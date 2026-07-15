# Phase 3 — 주문 승인/거절 + 생산 큐 (핵심 비즈니스 로직)

> 작성 시점 기준으로 `docs/Phase0.md`~`docs/Phase2.md`는 다른 작업자가 동시에 작성 중이라
> 아직 확정되지 않았을 수 있다. 이 문서는 `docs/PLAN.md`의 Phase 0~2 설명(도메인 모델
> `Sample`/`Order`/`ProductionJob`, JSON 기반 Repository, `RESERVED` 주문 생성)을 그대로
> 전제로 삼는다. 만약 실제 Phase0~2 문서/구현이 여기서 가정한 시그니처와 다르게 확정되면
> 본 Phase 착수 전에 이 문서의 "인터페이스/데이터 스키마" 절을 갱신해야 한다.
> 실제 소스 트리(`Model/`, `View/`, `Controller/`)는 이 작성 시점에 전혀 존재하지 않는다
> (Phase 0도 아직 구현 전). 따라서 아래 계획은 PRD/PLAN/CLAUDE.md에 근거한 설계 제안이며,
> Phase 0~2 구현 결과에 따라 클래스명/필드명 등 세부 사항은 조정될 수 있다.

## 1. 목표 요약 (PLAN.md 인용)

**목표**: PRD 7.4, 7.6 — 이 프로젝트에서 가장 까다로운 규칙이 몰려 있는 구간이므로 별도
Phase로 분리한다.

- Model: 승인 시 "승인 시점 가용 재고" 계산 로직 구현
  (CLAUDE.md의 "승인 시점 생산량 확정 규칙" — 승인 순서대로 재고를 소진하며 계산, 물리적
  `stock`은 즉시 차감하지 않고 실제 생산완료/출고 시점에만 변경)
  - 가용 재고 충분 → `CONFIRMED`
  - 부족 → `shortage`/`actualQuantity`(=ceil(부족분/수율))/`estimatedTime` 확정 →
    `ProductionJob` 생성(FIFO 큐) → 주문 `PRODUCING`
  - 거절 → `REJECTED`
- Model: 생산 라인 진행 처리 — 큐에서 작업을 꺼내 확정된 값 그대로 생산(재계산 없음),
  완료 시 재고(`stock`) 증가 및 해당 주문 `PRODUCING` → `CONFIRMED`
- Controller/View: `RESERVED` 목록 표시 → 승인/거절 선택, 생산 라인 현황/대기열 표시
- 이 Phase는 **PRD 7.4의 예시 1·2 시나리오를 그대로 테스트 케이스로 작성**해 회귀를 방지한다.

**완료 기준**: 예시 1(재고 0, 두 건 순차 주문 시 둘 다 100개 생산 확정)과 예시 2(재고 90,
뒤 주문이 물리적 재고를 가용으로 보지 않음) 시나리오가 테스트로 검증된다.

---

## 2. 핵심 비즈니스 로직 주의사항 (PRD 7.4 원문 그대로)

이 Phase의 최우선 순위는 아래 규칙을 정확히 구현하는 것이다. 구현 중 어떤 리팩터링을 하더라도
이 규칙이 깨지면 안 된다.

### 2.1 승인 시점 생산량 확정 규칙

- 실 생산량과 생산 시간은 **승인하는 시점의 재고만 고려하여 계산 후 확정**하며, 이후 생산 큐에서
  실제 생산이 시작될 때 재고를 다시 계산하지 않는다. (앞서 승인된 주문이 아직 생산 중이더라도,
  해당 주문이 예정한 미래 재고 증가분은 현재 승인 대상 주문의 계산에 반영하지 않는다 — 승인
  시점의 실제 재고만 사용)
- 재고가 남지만 그 재고가 **이미 앞선(먼저 승인된) 주문의 출고분으로 예정되어 있다면**, 뒤에
  승인되는 주문은 그만큼을 재고로 잡을 수 없다. 즉 승인 순서대로 재고를 소진해가며 부족분을
  계산해야 한다. (뒤 주문 입장에서 "이미 남에게 배정된 재고"는 없는 것으로 취급)

```
예시1) 현재 재고 0, A시료 수율 0.5
  주문1: A 50개 주문 → 부족분 50 → 실 생산량 = ceil(50/0.5) = 100
         (주문분 50개 제외한 나머지 50개는 생산 완료 후 재고로 편입)
  주문2: 주문1 생산 중 A 50개 추가 주문 → 승인 시점 재고는 여전히 0 →
         부족분 50 → 실 생산량 = ceil(50/0.5) = 100 (주문1의 미래 생산량은 고려하지 않음)
         → 생산 큐 등록, 큐에서 나와 실제 생산 시작 시에도 재고를 다시 계산하지 않고
           승인 시점에 확정된 생산량(100개) 그대로 생산

예시2) 현재 재고 90, A시료 수율 1.0
  주문1: A 100개 주문 → 부족분 = 100 - 90 = 10 → 실 생산량 10
  주문2: A 10개 주문 → 물리적 재고는 90개로 남아있지만, 그 90개는 이미 주문1의 출고분으로
         예정되어 있으므로 주문2 입장에서 가용 재고는 0 → 실 생산량 10
         (모니터링 화면에는 물리적 재고 90개가 그대로 표시되어야 함 — 승인 계산용 "가용
         재고"와 모니터링에 보여주는 "실제 재고"는 서로 다른 개념)
```

- 구현 시사점: 승인 로직은 시료의 물리적 `stock` 값 자체를 바로 차감하는 것이 아니라, 승인
  순서대로 처리하면서 "아직 배정되지 않은 가용 재고"를 추적해 계산에만 사용해야 한다. 물리적
  `stock`은 실제 생산 완료/출고 시점에만 변경한다.

### 2.2 계산 공식 (PRD 7.6)

```
부족분(shortage) = max(주문 수량 - 승인 시점 가용 재고, 0)
실 생산량(actualQuantity) = ceil(shortage / yieldRate)
총 생산 시간(estimatedTime) = avgProductionTime × actualQuantity
```

- 위 계산은 주문 승인 시점에 1회 확정되며, 생산 큐에서 대기 후 실제 생산이 시작될 때 재계산하지
  않고 확정된 값(`ProductionJob`에 저장된 `shortage`/`actualQuantity`/`estimatedTime`)을 그대로
  사용한다.
- 생산 완료 시 물리적 `stock`을 `actualQuantity`만큼 증가시키고, 해당 주문을 `PRODUCING` →
  `CONFIRMED`로 전환한다.

### 2.3 모니터링과 가용 재고의 분리

- 모니터링(Phase 5에서 구현)에는 항상 **물리적 `stock`**을 표시한다. 승인 계산에만 쓰이는 "가용
  재고"는 모니터링 대상이 아니다. Phase 3에서 가용 재고를 별도 상태로 도입할 때, 이 값이
  실수로 모니터링/조회 화면에 물리적 재고 대신 노출되지 않도록 View/Controller 계층에서
  명확히 구분해야 한다.

---

## 3. 확정된 설계: "가용 재고" 모델링 (파생 계산 방식)

가용 재고(availableStock)는 **`Sample`에 별도 필드를 추가하지 않고, 승인 로직에서 매번
파생 계산**하는 방식으로 확정했다. 구현 중 이 결정을 재검토할 필요는 없다.

### 3.1 검토했던 대안 (참고, 채택하지 않음)

과거 검토 단계에서 `Sample`에 `committedStock`(또는 `allocatedStock`) 필드를 추가해 물리적
`stock`과 함께 "이미 배정된 수량"을 이중으로 보관하는 방식도 후보에 있었으나, `Sample`이
재고 배정이라는 주문 도메인 관심사까지 떠안게 되어 단일 책임 원칙에 어긋나고, 두 값의 동기화
(생산 완료/출고 시 갱신)를 별도로 관리해야 하는 부담이 있어 채택하지 않았다.

### 3.2 확정안: 파생 가용 재고 계산

가용 재고를 저장된 상태로 유지하지 않고, 항상 **"물리적 stock − 아직 출고되지 않은
CONFIRMED/PRODUCING 주문의 확정 수량"**으로 그때그때 계산한다.

- 가용 재고(availableStock) 계산:
  ```
  availableStock(sampleId) = sample.stock
      - Σ(quantity of orders where sampleId matches and status ∈ {CONFIRMED, PRODUCING})
  ```
  - `CONFIRMED`/`PRODUCING` 주문은 이미 승인 시점에 재고(또는 향후 생산량)가 배정된 주문이므로
    "이미 배정된 만큼"을 물리적 재고에서 제외한 나머지만 다음 주문이 가용으로 볼 수 있다.
  - `RESERVED`/`REJECTED` 주문은 아직 배정이 확정되지 않았거나 무효이므로 계산에서 제외한다.
  - `RELEASE`된 주문은 이미 물리적 `stock`에서 출고 시 차감(Phase 4에서 처리)되므로 별도로
    빼지 않는다. (Phase 4 완료 기준: 출고 시 `stock -= quantity` 확정 필요 — 아래 "의존 관계"
    참고)
- 이 방식은 별도 필드/클래스 없이 `Order` 목록만으로 가용 재고를 언제든 재계산할 수 있어
  영속화 문제(재시작 후 상태 복원)가 자연히 해결된다. 단, **승인은 반드시 주문이 들어온 순서
  (createdAt 오름차순 혹은 채번 순서)대로 순차 처리**해야 하며, 한 번에 여러 주문을 일괄
  승인하는 기능을 만들 경우 이 순서를 강제해야 한다(주문 처리 UI에서 임의 순서로 승인 시
  본 규칙이 깨질 수 있음에 유의).
- 이 프로젝트 규모(콘솔 앱, 소규모 더미 데이터)에서는 매 승인마다 전체 주문을 스캔하는 방식의
  성능 비용은 문제가 되지 않는다고 보고, 별도의 캐싱/인덱싱은 도입하지 않는다.

### 3.3 단일 주문 승인 알고리즘 (확정)

```
ApproveOrder(order, sample, allOrders):
    if order.status != RESERVED: 오류
    available = sample.stock - Σ(o.quantity for o in allOrders
                                   if o.sampleId == sample.id
                                   and o.status in {CONFIRMED, PRODUCING})
    shortage = max(order.quantity - available, 0)
    if shortage == 0:
        order.status = CONFIRMED
        order.updatedAt = now
    else:
        actualQuantity = ceil(shortage / sample.yieldRate)
        estimatedTime = sample.avgProductionTime * actualQuantity
        job = ProductionJob{ orderId, sampleId, shortage, actualQuantity,
                              estimatedTime, progress = 0 }
        생산 큐(FIFO)에 job push
        order.status = PRODUCING
        order.updatedAt = now
```

- `RejectOrder(order)`: `order.status == RESERVED` 검증 후 `REJECTED`로 전환, `updatedAt` 갱신.

---

## 4. 작업 항목 체크리스트

### 4.1 Model

- [ ] `Order`에 상태 전이 메서드 추가 (Phase 0/2에서 정의된 `Order` 클래스를 확장)
  - `void Reject()` — `RESERVED`인 경우만 허용, `REJECTED`로 전이 + `updatedAt` 갱신
  - `void ConfirmDirectly()` — `RESERVED` → `CONFIRMED` (재고 충분 케이스)
  - `void MoveToProducing()` — `RESERVED` → `PRODUCING` (생산 큐 등록 케이스)
  - `void CompleteProduction()` — `PRODUCING` → `CONFIRMED` (생산 완료 케이스, Phase 3 범위)
  - 위 메서드들은 잘못된 상태에서 호출 시 예외/오류를 반환해 상태 흐름도(PRD 6.3)를 벗어나지
    않게 한다.
- [ ] `ProductionJob` 클래스 정의 (Phase 0에서 헤더만 선언됐다면 이 Phase에서 실제 로직 채움)
  - 필드: `orderId`, `sampleId`, `shortage`, `actualQuantity`, `estimatedTime`, `startedAt`
    (생산 시작 시각, 아래 "progress 갱신 방식 확정" 참고)
  - **progress 갱신 방식 확정**: `progress`는 별도 백그라운드 스레드/타이머로 자동 갱신되지
    않고, 저장된 상태 값도 아니다. "생산 라인 조회" 화면에 진입하거나 그 화면에서 새로고침을
    트리거하는 시점마다, 현재 시각과 해당 작업의 `startedAt`(생산 시작 시각)을 비교해 그때
    그때 다시 계산해서 보여준다.
    ```
    progress = min(1.0, (now - startedAt) / estimatedTime)
    ```
    - `startedAt`은 해당 job이 큐의 맨 앞으로 나와 실제로 생산을 시작한 시각이며, 아직 대기
      중(큐에서 순서를 기다리는 중)인 job은 `startedAt`이 없다(`std::optional` 또는 null).
    - 이 계산은 순수 함수 `double ComputeProgress(const ProductionJob&, TimePoint now)`로
      구현해 Model 계층에 둔다(뷰/컨트롤러에 로직이 섞이지 않도록).
  - `bool IsComplete(TimePoint now) const` (또는 순수 함수) — `ComputeProgress(...) >= 1.0`
    여부로 완료 판단
- [ ] `AvailableStockCalculator` (또는 이에 준하는 순수 함수/서비스)
  - `int CalculateAvailableStock(const Sample& sample, const std::vector<Order>& orders)`
  - CONFIRMED/PRODUCING 상태 주문의 quantity 합을 물리적 stock에서 제외해 반환
  - 순수 함수로 구현해 단위 테스트 용이하게 함 (부작용 없음)
- [ ] `OrderApprovalService` (Controller가 아닌 Model 계층의 도메인 서비스)
  - `ApprovalResult Approve(Order& order, const Sample& sample, const std::vector<Order>& allOrders)`
    - 내부에서 `AvailableStockCalculator` 호출 → shortage 계산 → 실 생산량/생산 시간 계산
    - 재고 충분 시 `order.ConfirmDirectly()`만 호출하고 `ProductionJob`을 만들지 않음
    - 재고 부족 시 `order.MoveToProducing()` + 생성된 `ProductionJob`을 반환(호출자가 큐에 push)
    - **주의**: 이 서비스는 물리적 `Sample::stock`을 변경하지 않는다 (읽기 전용 참조만 사용)
  - `void Reject(Order& order)` — `order.Reject()` 위임 (검증 포함)
- [ ] `ProductionLine` (FIFO 큐 관리, Model 계층)
  - 내부 자료구조: `std::deque<ProductionJob>` 혹은 Repository가 관리하는 리스트(JSON 영속)
  - `void Enqueue(ProductionJob job)` — 큐가 비어 있던 상태에서 처음 push되는 job은 즉시
    맨 앞(진행 중)이 되므로 이 시점에 `startedAt = now`를 설정한다. 이미 다른 job이 진행
    중이면 `startedAt`은 설정하지 않고(대기 상태) 순서만 유지한다.
  - `const ProductionJob* Peek() const` — 현재 진행 중(맨 앞) 작업 조회
  - `std::vector<ProductionJob> ListQueue() const` — 대기열 전체(진행 중 포함) 조회, FIFO 순서
    유지
  - `double PeekProgress(TimePoint now) const` — 맨 앞 job에 대해 `ComputeProgress(...)`를
    호출해 그 시점 기준 진행률을 반환 ("생산 라인 조회/새로고침" 시 이 함수를 사용)
  - `void CompleteCurrentJob(SampleRepository&, OrderRepository&)` (또는 완료된 job을 반환하고
    호출자가 Repository 갱신을 책임지는 방식, 5.4절 참고)
    - 맨 앞 job을 꺼내 완료 처리: 해당 `Sample.stock += job.actualQuantity` (물리적 재고 갱신
      은 여기서 최초로 발생), 해당 `orderId`의 Order를 `CompleteProduction()` 호출로
      `PRODUCING → CONFIRMED` 전이, 큐에서 제거
    - 재계산 없음 — `job.actualQuantity`를 그대로 사용
    - 다음 job이 큐에 남아 있다면 그 job의 `startedAt = now`를 설정해 진행을 이어간다
  - **완료 트리거 방식 확정**: 실시간 타이머로 자동 완료 처리하지 않는다. 사용자가 "생산 라인"
    화면에서 진행률이 100%(또는 경과 시간이 `estimatedTime` 이상)임을 확인한 뒤 "생산 완료
    처리" 메뉴를 선택해야 `CompleteCurrentJob`이 호출된다(수동 트리거). `progress` 자체는
    위에서 정의한 대로 조회 시점마다 파생 계산되므로, 진행 중 별도로 갱신할 값은 없다.
- [ ] Repository 확장 (Phase 0에서 정의된 Repository 인터페이스에 맞춰)
  - `ProductionJobRepository`(혹은 `ProductionLineRepository`)가 큐 상태를 JSON으로
    저장/로드해 재시작 후에도 대기열이 복원되도록 함 (CLAUDE.md 영속성 요구사항)
  - `OrderRepository::Update(order)` — 상태 변경 반영 후 저장

### 4.2 Controller

- [ ] `OrderApprovalController` (또는 기존 주문 컨트롤러 확장)
  - `RESERVED` 주문 목록 조회 → 사용자에게 번호로 표시
  - 사용자가 특정 주문 선택 → 승인/거절 입력 받음
  - 승인 선택 시: 해당 시료 조회 → `OrderApprovalService::Approve` 호출 → 결과(즉시 CONFIRMED
    인지, PRODUCING으로 큐 등록됐는지)를 View에 전달
  - 거절 선택 시: `OrderApprovalService::Reject` 호출 → View에 결과 전달
  - 승인 순서 강제: 반드시 `RESERVED` 목록을 접수 순서(주문번호/createdAt 오름차순)로 노출하고,
    한 번에 하나씩 처리하도록 유도한다. 임의 순서 승인이 가능한 UI를 만들 경우 2.1의 규칙이
    깨질 수 있음을 재차 강조.
- [ ] `ProductionLineController`
  - 생산 라인 현황(진행 중 작업) 조회 → View 전달. 조회 시 항상 현재 시각(`now`)을 기준으로
    `ProductionLine::PeekProgress(now)`를 호출해 그 시점 진행률을 계산한 뒤 View에 넘긴다
    (저장된 progress 값을 그대로 읽어오지 않는다)
  - 대기열(큐) 목록 조회 → View 전달
  - "새로고침" 입력 처리 — 생산 라인 화면 진입 직후 및 화면에서 "새로고침" 메뉴 선택 시
    동일한 조회 로직(현재 시각 기준 `PeekProgress` 재계산)을 다시 수행해 최신 진행률을
    반영한다. 별도 백그라운드 갱신 없이 이 재조회 시점에만 값이 갱신된다.
  - "생산 완료 처리"(또는 진행 트리거) 입력 처리 → `ProductionLine::CompleteCurrentJob` 호출 →
    관련 `Sample`/`Order` Repository 갱신 반영

### 4.3 View

- [ ] `RESERVED` 주문 목록 출력 (주문번호/시료ID/고객명/수량/접수일시)
- [ ] 승인/거절 결과 출력
  - 승인 + 재고 충분: "즉시 출고 대기(CONFIRMED) 처리되었습니다" 등
  - 승인 + 재고 부족: "부족분 N개, 실 생산량 M개로 생산 큐에 등록되었습니다(예상 생산시간: T)"
  - 거절: "주문이 거절 처리되었습니다"
- [ ] 생산 라인 화면
  - 현재 생산 중인 작업(주문번호/시료ID/actualQuantity/progress/추정완료 등) 출력. progress는
    화면 진입/새로고침 시점마다 Controller가 다시 계산해 넘긴 값을 그대로 표시(View는 계산
    로직을 갖지 않는다)
  - FIFO 대기열 목록 출력 (순서 보장)
  - "새로고침" 메뉴 항목 제공 — 선택 시 Controller에 재조회를 요청해 화면을 다시 그린다

---

## 5. 인터페이스/데이터 스키마

### 5.1 `Order` 확장 (Phase 0/2 스키마에 상태 전이 메서드 추가, 필드 변경 없음)

```cpp
class Order {
public:
    // Phase 0/2에서 정의된 기존 필드: orderId, sampleId, customerName,
    // quantity, status, createdAt, updatedAt

    void Reject();              // RESERVED -> REJECTED
    void ConfirmDirectly();     // RESERVED -> CONFIRMED
    void MoveToProducing();     // RESERVED -> PRODUCING
    void CompleteProduction();  // PRODUCING -> CONFIRMED
};
```

### 5.2 `ProductionJob` (PRD 8.3 기반, `startedAt` 필드 추가 — progress 갱신 방식 확정에 따름)

`progress`는 저장 필드가 아니라 조회 시점마다 파생 계산되는 값으로 확정했다. 대신 계산의
기준이 되는 "생산 시작 시각"(`startedAt`)을 저장한다.

```cpp
struct ProductionJob {
    std::string orderId;
    std::string sampleId;
    int shortage;          // 승인 시점 확정
    int actualQuantity;    // ceil(shortage / yieldRate), 승인 시점 확정
    double estimatedTime;  // avgProductionTime * actualQuantity, 승인 시점 확정
    std::optional<std::chrono::system_clock::time_point> startedAt;
    // 큐의 맨 앞으로 나와 실제 생산이 시작된 시각. 아직 대기 중인 job은 nullopt.
};

// progress는 저장하지 않고 조회 시점마다 이 순수 함수로 계산한다.
double ComputeProgress(const ProductionJob& job,
                        std::chrono::system_clock::time_point now);
// startedAt이 nullopt(아직 대기 중)이면 0.0 반환,
// 그 외에는 min(1.0, (now - startedAt) / estimatedTime) 반환.
```

JSON 스키마 예시 (PRD 8.3의 `progress` 필드는 `startedAt`으로 대체 — 조회 시점 재계산 방식이므로
progress 값 자체를 영속화할 필요가 없다):
```json
{
  "orderId": "ORD-20260416-0043",
  "sampleId": "S-001",
  "shortage": 50,
  "actualQuantity": 100,
  "estimatedTime": 500.0,
  "startedAt": "2026-07-15T09:00:00"
}
```
- 아직 대기 중(큐에서 순서를 기다리는 중)인 job은 `"startedAt": null`로 직렬화한다.

### 5.3 도메인 서비스

```cpp
struct ApprovalResult {
    bool wasQueued;               // true면 PRODUCING(큐 등록), false면 CONFIRMED
    std::optional<ProductionJob> job; // wasQueued == true일 때만 값 존재
};

class OrderApprovalService {
public:
    ApprovalResult Approve(Order& order, const Sample& sample,
                           const std::vector<Order>& allOrders);
    void Reject(Order& order);
};

int CalculateAvailableStock(const Sample& sample,
                             const std::vector<Order>& orders);
```

### 5.4 `ProductionLine`

```cpp
class ProductionLine {
public:
    // 큐가 비어 있던 상태에서 push되면 즉시 startedAt = now로 설정(생산 시작)
    void Enqueue(ProductionJob job);
    const ProductionJob* Peek() const;
    const std::vector<ProductionJob>& ListQueue() const;
    // "생산 라인 조회/새로고침" 시점마다 호출 — 맨 앞 job의 진행률을 그 시점 기준으로 계산
    double PeekProgress(std::chrono::system_clock::time_point now) const;
    // 완료 처리는 Sample/Order 갱신까지 포함하므로 Repository를 인자로 받거나,
    // 완료된 job을 반환하고 Controller가 Repository 갱신을 책임지는 방식도 가능
    // (아래는 후자를 제안 — Model이 Repository를 직접 알 필요 없게 하기 위함)
    // 다음 job이 있으면 그 job의 startedAt = now로 설정해 생산을 이어감
    std::optional<ProductionJob> CompleteFrontJob();
};
```

- `ProductionLine::CompleteFrontJob()`이 완료된 job을 반환하면, 이를 호출한 Controller가
  `Sample.stock += job.actualQuantity` 반영 및 `Order`를 조회해 `CompleteProduction()` 호출 →
  각 Repository에 저장하는 흐름을 제안. (Model이 Repository에 의존하지 않도록 해 계층 의존
  방향을 지킴 — CLAUDE.md MVC 원칙)

---

## 6. 의존 관계

### 6.1 이 Phase가 전제로 삼는 이전 Phase 산출물

- Phase 0: `Sample`, `Order`(status enum 포함), `ProductionJob` 도메인 모델 정의 및 JSON
  Repository(로드/저장/CRUD) — 특히 `OrderRepository::FindAll()`(혹은 이에 준하는 전체 조회)이
  가용 재고 계산에 필요
- Phase 1: `Sample` 등록/조회 — 승인 로직이 시료의 `stock`/`yieldRate`/`avgProductionTime`을
  참조
- Phase 2: `Order` 생성 (`RESERVED` 상태, 주문번호 채번, `createdAt` 기록) — 승인 대상 목록의
  출처. **승인 순서는 Phase 2에서 부여한 접수 순서(주문번호/`createdAt`)를 그대로 사용**한다는
  점이 이 Phase의 전제

### 6.2 이후 Phase가 이 Phase에 기대하는 산출물

- Phase 4(출고 처리): `CONFIRMED` 주문을 대상으로 출고 처리 → `RELEASE`. 이때 물리적 `stock`을
  차감하는 시점/방식이 확정되어 있어야 함. **본 문서의 가용 재고 계산식
  (`availableStock = stock - Σ(CONFIRMED/PRODUCING 주문 수량)`)은 CONFIRMED 주문이 출고
  전까지 재고에서 계속 "배정된 것"으로 잡혀 있다는 전제**이므로, Phase 4에서 출고 시
  물리적 `stock -= quantity`를 확정하는 로직이 반드시 필요함을 다음 Phase 계획에 명시해야 한다.
  **TODO: 확인 필요** — Phase 4 문서 작성 시 이 전제를 그대로 承繼(승계)할지 재확인 필요.
- Phase 5(모니터링): 상태별 주문 건수 집계 시 `PRODUCING`/`CONFIRMED`를 포함하고 `REJECTED`는
  제외 — 본 Phase가 만든 상태 전이 결과를 그대로 사용. 재고 현황 표기는 물리적 `stock`만
  사용(가용 재고 노출 금지).
- Phase 6(더미 데이터 생성기): 다양한 상태(RESERVED/CONFIRMED/PRODUCING 등)의 더미 `Order`를
  생성할 때, 이 Phase의 승인 로직을 거치지 않고 직접 상태를 주입하는 경우가 있을 수 있음 —
  이 경우 가용 재고 계산 결과가 실제 승인 시나리오와 다를 수 있음을 더미 데이터 생성기 문서에서
  주의사항으로 남겨야 함.

---

## 7. 테스트 계획 (tester 에이전트 작성 대상)

### 7.1 `CalculateAvailableStock` 단위 테스트

1. 주문이 없을 때: `availableStock == sample.stock`
2. `RESERVED`/`REJECTED` 주문만 있을 때: 계산에서 제외되어 `availableStock == sample.stock`
3. `CONFIRMED`/`PRODUCING` 주문이 있을 때: 해당 수량만큼 차감됨
4. `RELEASE` 주문은 계산에서 제외됨 (이미 물리적 stock에서 차감됐다고 가정)
5. 여러 시료가 섞여 있을 때 대상 `sampleId`만 필터링되는지 확인

### 7.2 `OrderApprovalService::Approve` 단위 테스트

1. 재고 충분 케이스: 주문 수량 ≤ 가용 재고 → `order.status == CONFIRMED`, `ApprovalResult.wasQueued == false`
2. 재고 부족 케이스: 주문 수량 > 가용 재고 → `order.status == PRODUCING`, `ProductionJob` 생성,
   `shortage`/`actualQuantity`/`estimatedTime` 값 검증
3. 수율에 따른 ceil 반올림 검증 (예: shortage=3, yieldRate=0.4 → actualQuantity = ceil(7.5) = 8)
4. `RESERVED`가 아닌 주문에 대해 `Approve` 호출 시 오류/예외 발생 검증
5. **PRD 예시1 재현 (회귀 테스트, 가장 중요)**
   - 초기 상태: Sample(A, stock=0, yieldRate=0.5)
   - 주문1(A, 50개) 생성 → 승인 → `shortage=50`, `actualQuantity=100`, `order1.status==PRODUCING`
   - (주문1이 아직 PRODUCING인 상태에서, 즉 `Sample.stock`은 여전히 0인 채로) 주문2(A, 50개)
     생성 → 승인 → `shortage=50`, `actualQuantity=100`, `order2.status==PRODUCING`
   - 검증 포인트: 주문2의 계산에 주문1의 "미래 생산량 100개"가 전혀 반영되지 않아야 함
     (반영됐다면 주문2의 shortage가 0이 되는 버그)
6. **PRD 예시2 재현 (회귀 테스트, 가장 중요)**
   - 초기 상태: Sample(A, stock=90, yieldRate=1.0)
   - 주문1(A, 100개) 생성 → 승인 → `shortage=10`, `actualQuantity=10`, `order1.status==PRODUCING`
   - 주문2(A, 10개) 생성 → 승인 → 가용 재고 계산 시 `availableStock = 90 - 100(주문1 수량,
     PRODUCING 상태) = -10 → 0으로 clamp` → `shortage=10`, `actualQuantity=10`,
     `order2.status==PRODUCING`
   - 검증 포인트: 물리적 `sample.stock`은 테스트 전 구간 내내 90 그대로 유지되어야 함
     (승인 로직이 `stock`을 직접 변경하지 않는지 확인)

### 7.3 `OrderApprovalService::Reject` 단위 테스트

1. `RESERVED` 주문 거절 → `status == REJECTED`, `updatedAt` 갱신 확인
2. `RESERVED`가 아닌 주문 거절 시도 → 오류/예외 발생

### 7.4 `ProductionLine` 단위 테스트

1. `Enqueue` 후 `Peek()`이 가장 먼저 등록된 job을 반환하는지 (FIFO)
2. 여러 job을 순서대로 enqueue한 뒤 `CompleteFrontJob()` 반복 호출 시 등록 순서대로 완료되는지
3. `CompleteFrontJob()` 호출 후 반환된 job의 `actualQuantity`가 승인 시점 확정값과 동일한지
   (재계산되지 않았는지)
4. 큐가 비어 있을 때 `Peek()`/`CompleteFrontJob()` 호출 시 안전하게 처리되는지(예:
   `nullptr`/`std::nullopt` 반환)
5. 빈 큐에 처음 `Enqueue`하면 해당 job의 `startedAt`이 즉시 설정되는지; 이미 진행 중인 job이
   있는 상태에서 추가로 `Enqueue`한 job은 `startedAt`이 설정되지 않는지(대기 상태 유지)
6. `CompleteFrontJob()` 호출 후 다음 job이 있다면 그 job의 `startedAt`이 새로 설정되는지

### 7.5 `ComputeProgress` 단위 테스트 (progress 조회 시점 재계산 확정 사항)

1. `startedAt`이 설정되지 않은(대기 중) job → `ComputeProgress == 0.0`
2. `startedAt` 이후 경과 시간이 `estimatedTime`의 절반일 때 → `ComputeProgress ≈ 0.5`
3. 경과 시간이 `estimatedTime`을 초과할 때 → `ComputeProgress`가 1.0을 초과하지 않고 clamp되는지
4. 동일한 job에 대해 서로 다른 `now`를 두 번 넘겨 호출했을 때, 저장된 값이 바뀌지 않고
   매번 그 시점 기준으로 다시 계산되는지 (즉, 호출이 job 내부 상태를 변경하지 않는 순수
   함수인지 확인)

### 7.6 통합(시스템) 테스트

1. 시료 등록 → 주문 접수(RESERVED) → 승인(재고 충분) → CONFIRMED 확인 → 재시작 시뮬레이션
   (Repository 재로드) 후 상태 유지 확인
2. 시료 등록 → 주문 접수 → 승인(재고 부족) → PRODUCING + 생산 큐 등록 확인 → 재시작 후 큐
   상태(FIFO 순서 포함, `startedAt` 포함) 복원 확인
3. 생산 큐 완료 처리 → 물리적 `stock` 증가 확인 + 해당 주문 `CONFIRMED`로 전환 확인
4. 주문 거절 → REJECTED 확인 → (Phase 5 선행 구현 시) 모니터링 집계에서 제외되는지 확인,
   미구현 시 상태값만 확인
5. 생산 라인 화면 조회 → 일정 시간 경과 시뮬레이션(테스트에서 `now`를 다르게 주입) → 화면을
   다시 조회(새로고침)했을 때 progress가 이전 조회 시점보다 증가해 표시되는지 확인 (별도
   백그라운드 갱신 없이도 재조회 시점 기준으로 값이 달라짐을 검증)

---

## 8. 완료 조건 (Definition of Done)

- [ ] `RESERVED` 목록 표시 → 승인/거절 처리가 콘솔에서 실제로 동작
- [ ] 승인 시 가용 재고 계산이 CLAUDE.md/PRD 7.4 규칙(승인 순서대로 소진, 물리적 `stock` 즉시
      차감 금지)을 그대로 따름
- [ ] PRD 7.4 예시1·예시2 시나리오가 GoogleTest로 작성되어 통과함 (7.2절 5, 6번 테스트)
- [ ] 생산 큐(FIFO)가 등록/조회/완료 처리 되며, 완료 시에만 물리적 `stock`이 증가함
- [ ] `ProductionJob`의 `shortage`/`actualQuantity`/`estimatedTime`이 승인 시점에 확정된 뒤
      생산 완료까지 변경되지 않음이 테스트로 증명됨
- [ ] 주문/생산 큐 상태(`startedAt` 포함)가 JSON으로 영속화되어 재시작 후 복원됨(Phase 0
      Repository 재사용)
- [ ] `ProductionJob.progress`가 별도 값으로 저장/자동 갱신되지 않고, "생산 라인 조회/새로고침"
      시점마다 `startedAt`/`estimatedTime` 기준으로 재계산되어 표시됨이 테스트로 증명됨
- [ ] `code-review` 에이전트 검토 통과 (Clean Code/SOLID/함수 라인 수/디자인 패턴 남용 여부)
- [ ] `tester` 에이전트가 7장의 테스트를 작성해 실제 빌드/실행 후 전부 통과함을 보고
- [ ] `supervisor` 에이전트가 위 산출물이 CLAUDE.md/PRD.md/PLAN.md에 부합함을 확인

---

## 9. 확인이 필요한 애매한 지점 (TODO)

> 가용 재고 모델링 방식과 `ProductionJob.progress` 갱신 방식은 확정되어(3장, 4.1절 참고)
> 더 이상 TODO가 아니다. 아래는 여전히 남아 있는 애매한 지점이다.

1. **일괄 승인 UI 허용 여부**: 승인 순서를 반드시 지켜야 하므로, Controller/View가 한 번에 여러
   주문을 승인하는 기능을 제공할지(제공한다면 내부적으로 순서를 강제해야 함) 여부는 PRD에
   명시되어 있지 않음.
2. **Phase 4와의 재고 차감 시점 경계**: 가용 재고 계산식이 "CONFIRMED 주문은 출고 전까지 재고
   배정 상태"라는 전제를 갖는데, 이는 Phase 4(출고 처리) 설계와 맞물려야 한다. Phase 4 문서
   작성 시 이 전제를 명시적으로 이어받아야 한다.
