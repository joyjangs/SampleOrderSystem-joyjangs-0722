# DATA_SCHEMA — 데이터 스키마 및 영속성 규칙

> 본 문서는 `docs/PRD.md` 8장(데이터 모델)과 `docs/Phase0~7.md`에서 확정된 세부 사항을 통합해,
> 실제 JSON 파일 스키마와 "저장하는 값 vs 매번 계산하는 값"의 경계를 하나의 문서로 못박기
> 위해 작성한다. Model 클래스(`Model/Sample.h`, `Model/Order.h`, `Model/OrderStatus.h`,
> `Model/ProductionJob.h`)의 실제 필드와 대조해 최신 상태를 유지한다.

## 1. 영속성 방식 개요

- 외부 라이브러리 없이 자체 구현한 `Common::JsonValue`(재귀 하강 파서/직렬화기, `Common/Json.h`
  · `Common/Json.cpp`)를 사용한다 (Phase0.md 2.3절).
- 엔티티별 Repository(`SampleRepository`/`OrderRepository`/`ProductionJobRepository`)는
  공통 템플릿 `Model::JsonFileRepository<TEntity>`(`Model/JsonFileRepository.h`)를 기반으로
  구현되며, **모든 CRUD 연산(`Add`/`Update`/`Remove`)이 즉시 파일에 flush**된다(`Save()`를
  내부에서 자동 호출). 명시적 `Save()` 별도 호출 정책이 아니다.
- 각 Repository는 다음 파일을 로드/저장한다.

  | 엔티티 | 파일 경로 |
  |---|---|
  | `Sample` | `data/samples.json` |
  | `Order` | `data/orders.json` |
  | `ProductionJob` | `data/production_jobs.json` |

- 파일이 없으면 빈 배열(`[]`)로 시작한다(`Load()`가 `std::filesystem::exists` 확인 후 없으면
  빈 컬렉션 유지). 현재 저장소의 세 파일은 초기 상태(`[]`)로 커밋되어 있다.
- 저장 형식은 각 엔티티 배열을 담은 JSON 배열이며, pretty-print 여부/들여쓰기는
  `Common::JsonValue::SaveFile` 구현을 따른다(문서 레벨에서 별도로 규정하지 않음).

## 2. 엔티티 스키마

### 2.1 Sample (`data/samples.json`)

`Model/Sample.h` 실제 필드와 1:1 대응.

| 필드 | JSON 키 | 타입 | 제약 (Phase1.md 3.1) |
|---|---|---|---|
| id | `id` | string | 공백 아님(trim 후 non-empty), 시스템 내 유일. **사용자가 직접 입력**하며 자동 채번 없음 |
| name | `name` | string | 공백 아님 |
| avgProductionTime | `avgProductionTime` | number(double) | `> 0` (분/개 단위) |
| yieldRate | `yieldRate` | number(double) | `0.0 < yieldRate <= 1.0` — **0은 등록 시점에 거부**(`ceil(shortage/yieldRate)`의 0-division 방지, Phase1.md 5절) |
| stock | `stock` | number(int) | `>= 0`. **물리적 재고만 의미** — 등록 시 기본값 0 (Phase1.md 2.1절 기본안) |

```jsonc
// data/samples.json
[
  { "id": "S-001", "name": "실리콘 웨이퍼-8인치", "avgProductionTime": 10.0, "yieldRate": 0.9, "stock": 50 }
]
```

**중요 — `stock`의 이중적 의미 금지**: `Sample`에는 "가용 재고"(승인 계산용)를 저장하는
필드를 절대 추가하지 않는다. 물리적 `stock`은 오직 아래 두 시점에만 변경된다.

1. 생산 완료 시: `stock += actualQuantity` (Phase 3 담당)
2. 출고(`RELEASE`) 처리 시: `stock -= order.quantity` (Phase 4 담당)

주문 승인(Phase 3)·주문 접수(Phase 2)는 `stock`을 직접 건드리지 않는다.

### 2.2 Order (`data/orders.json`)

`Model/Order.h` 및 `Model/OrderStatus.h` 실제 필드와 1:1 대응.

| 필드 | JSON 키 | 타입 | 비고 |
|---|---|---|---|
| orderId | `orderId` | string | 포맷 `ORD-YYYYMMDD-NNNN` (NNNN은 당일 발급 순번, 4자리 zero-padding, Phase2.md 3.1절). `OrderIdGenerator`가 자동 채번 |
| sampleId | `sampleId` | string | `SampleRepository`에 존재하는 ID만 허용(등록되지 않은 시료는 주문 자체가 생성되지 않음) |
| customerName | `customerName` | string | 공백 아님 |
| quantity | `quantity` | number(int) | `>= 1` |
| status | `status` | string(enum) | `RESERVED` \| `REJECTED` \| `PRODUCING` \| `CONFIRMED` \| `RELEASE` — `Model::OrderStatus`와 `ToString`/`OrderStatusFromString`으로 왕복 변환 |
| createdAt | `createdAt` | string | ISO 8601 (`YYYY-MM-DDTHH:MM:SS`), 생성 시 1회 고정 |
| updatedAt | `updatedAt` | string | 동일 포맷, 상태 전이마다 갱신 |

```jsonc
// data/orders.json
[
  {
    "orderId": "ORD-20260416-0043",
    "sampleId": "S-001",
    "customerName": "삼성전자 파운드리",
    "quantity": 200,
    "status": "PRODUCING",
    "createdAt": "2026-04-16T10:00:00",
    "updatedAt": "2026-04-16T10:05:00"
  }
]
```

**상태 전이 가드** (허용되는 전이만 존재, `Order` 클래스 내부 메서드로 캡슐화 — Phase3/4.md):

```
RESERVED --Reject()--------------> REJECTED
RESERVED --ConfirmDirectly()-----> CONFIRMED     (승인 시점 가용 재고 충분)
RESERVED --MoveToProducing()-----> PRODUCING     (승인 시점 가용 재고 부족 → 생산 큐 등록)
PRODUCING --CompleteProduction()-> CONFIRMED     (생산 완료)
CONFIRMED --Release()------------> RELEASE       (출고 처리)
```

그 외 전이(예: `RELEASE`에서 재출고, `REJECTED`에서 재승인)는 모두 거부된다.

### 2.3 ProductionJob (`data/production_jobs.json`)

`Model/ProductionJob.h` 실제 필드와 1:1 대응. 주문당 최대 1건(`orderId`가 키, Phase0.md 3.1절
— "주문당 1개의 ProductionJob"을 전제로 채택).

| 필드 | JSON 키 | 타입 | 비고 |
|---|---|---|---|
| orderId | `orderId` | string | 연관 `Order.orderId` (Repository 키) |
| sampleId | `sampleId` | string | 생산 대상 시료 ID |
| shortage | `shortage` | number(int) | **승인 시점에 1회 확정**, 이후 재계산 금지 |
| actualQuantity | `actualQuantity` | number(int) | `ceil(shortage / yieldRate)`, 승인 시점 확정 |
| estimatedTime | `estimatedTime` | number(double) | `avgProductionTime × actualQuantity`, 승인 시점 확정 |
| startedAt | `startedAt` | string \| null | 생산이 실제로 시작된 시각(ISO 8601). **큐 대기 중(아직 맨 앞이 아님)이면 `null`** |

```jsonc
// data/production_jobs.json
[
  {
    "orderId": "ORD-20260416-0043",
    "sampleId": "S-001",
    "shortage": 170,
    "actualQuantity": 185,
    "estimatedTime": 148.0,
    "startedAt": "2026-04-16T10:05:00"
  },
  {
    "orderId": "ORD-20260416-0044",
    "sampleId": "S-002",
    "shortage": 80,
    "actualQuantity": 103,
    "estimatedTime": 30.9,
    "startedAt": null
  }
]
```

**`progress`는 영속 필드가 아니다.** JSON에 저장하지 않고, "생산 라인 조회/새로고침" 시점마다
아래 순수 함수로 파생 계산한다(Phase3.md 4.1절/5.2절).

```
progress = (startedAt == null) ? 0.0 : min(1.0, (now - startedAt) / estimatedTime)
```

큐(FIFO)는 `ProductionJobRepository`가 저장하는 리스트 순서 자체로 표현하며, 별도의 순번
필드를 두지 않는다. 큐가 비어 있던 상태에서 처음 `Enqueue`되는 job은 즉시 `startedAt = now`가
설정되고, 이후 `Enqueue`되는 job은 `startedAt = null`(대기)로 유지된다.

## 3. 저장하지 않는 값 (파생 계산 전용)

아래 값들은 어떤 JSON 파일에도 필드로 존재하지 않는다. Repository에 저장된 원본 데이터로부터
조회 시점마다 다시 계산해야 하며, 캐시하거나 영속 필드로 승격시키지 않는다.

### 3.1 승인 시점 가용 재고 (`availableStock`)

Phase3.md 3.2절에서 확정한 계산식. `Sample`에 별도 필드를 두지 않고, 주문 승인 로직 내부에서만
사용한다.

```
availableStock(sampleId) = sample.stock
    - Σ(order.quantity for order in allOrders
         where order.sampleId == sampleId
         and order.status ∈ {CONFIRMED, PRODUCING})
```

- `RESERVED`/`REJECTED` 주문은 제외 (아직 배정 확정 전 또는 무효).
- `RELEASE` 주문은 제외 (이미 물리적 `stock`에서 차감 완료).
- 승인은 반드시 주문 접수 순서(`createdAt`/채번 순서)대로 순차 처리해야 이 계산식이 PRD 7.4의
  두 예시(재고 0에서 순차 100개씩 확정 / 재고 90을 두 주문이 나눠 갖지 못하는 경우)를 만족한다.

### 3.2 재고 판정용 비교 수량 (`pendingOrderQuantity`, 모니터링 전용)

Phase5.md 3.4절에서 확정. 3.1의 `availableStock`과는 **다른 값**이며 혼동 금지.

```
pendingOrderQuantity(sampleId) = Σ(order.quantity for order in allOrders
                                     where order.sampleId == sampleId
                                     and order.status == PRODUCING)
```

- `RESERVED`(승인 전), `CONFIRMED`(재고 충분 확인됨), `RELEASE`/`REJECTED`(더 이상 압박 요인
  아님)는 모두 제외 — 오직 `PRODUCING`만 집계.
- 여유/부족/고갈 판정(모니터링, PRD 7.5):

  ```
  stock == 0                          → DEPLETED(고갈)   ← 최우선 규칙
  stock > 0 and stock < pendingQty    → SHORTAGE(부족)
  그 외 (stock >= pendingQty)         → SUFFICIENT(여유)
  ```

- 모니터링 화면은 예외 없이 **물리적 `stock`**만 표시한다. 3.1의 `availableStock`을 모니터링에
  노출하지 않는다(PRD 7.4 예시2: "모니터링 화면에는 물리적 재고 90개가 그대로 표시되어야 함").

### 3.3 생산 진행률 (`progress`)

2.3절 참고. `ProductionJob.startedAt`과 `estimatedTime`으로부터 조회 시점(`now`)마다 계산.

### 3.4 모니터링 집계 값

`OrderStatusSummary`(상태별 건수), `MainMenuSummary`(등록 시료 수/총 재고/전체 주문 수/생산
라인 대기 건수)도 저장하지 않고 Repository의 `FindAll()` 결과를 매번 다시 순회해 계산한다
(Phase5.md 5절 "읽기 전용, 재계산 없음").

- `REJECTED`는 모든 주문 관련 집계(상태별 건수, 전체 주문 수)에서 제외한다.
- "생산 라인 대기 건수"는 `PRODUCING` 상태 주문에 연결된 `ProductionJob` 개수(진행 중 1건 +
  대기열 포함)로 정의한다.

## 4. 스키마 변경 이력 규칙

- `Sample`/`Order`/`ProductionJob`에 필드를 추가/변경할 때는 반드시 본 문서와 `docs/PRD.md`
  8장을 함께 갱신한다.
- 3장에 정리된 "파생 계산 전용" 값을 저장 필드로 승격하는 변경은 PRD 7.4/7.5의 핵심 규칙
  (승인 시점 확정, 모니터링·가용 재고 분리)과 충돌할 가능성이 크므로, 변경 전 Phase3.md/
  Phase5.md의 해당 절을 재검토한다.
