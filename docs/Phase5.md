# Phase 5 — 모니터링 시스템

## 1. 목표 요약 (PLAN.md 인용)

> **목표**: PRD 7.5 / CLAUDE.md 데이터 모니터링 요구사항.
>
> - 상태별(`RESERVED`/`CONFIRMED`/`PRODUCING`/`RELEASE`) 주문 건수 집계 (`REJECTED` 제외)
> - 시료별 재고 현황 및 여유/부족/고갈 상태 표기 (물리적 `stock` 기준 — 승인 계산용 가용
>   재고와 혼동하지 않음)
> - 메인 메뉴 요약 정보(등록 시료 수, 총 재고, 전체 주문 수, 생산 라인 대기 건수) 연동
>
> **완료 기준**: 모니터링 화면이 실제 Repository 데이터를 반영해 정확히 표시된다.

관련 PRD 조항: 7.1(메인 메뉴), 7.5(모니터링), 10(UI 참고 — 모니터링 화면 예시).
관련 CLAUDE.md 조항: "핵심 구현 요소 — 데이터 모니터링 시스템", "상태별 주문 건수(REJECTED
제외)와 시료별 재고 현황(여유/부족/고갈)을 콘솔에서 실시간 조회할 수 있어야 한다."

---

## 2. 의존 관계

### 2.1 이 Phase가 전제로 삼는 이전 Phase 산출물

- **Phase 0**: `Sample`, `Order`(status enum 포함), `ProductionJob` 도메인 모델과 각각의
  파일 기반 Repository(로드/저장/CRUD). 본 Phase는 이 Repository들을 **읽기 전용**으로만
  사용하며 새 영속 스키마를 추가하지 않는다.
- **Phase 1**: `Sample` 목록 조회 인터페이스(예: `SampleRepository::findAll()` 또는 동등한
  시그니처) — 시료별 재고 현황 집계에 사용.
- **Phase 2**: `Order` 생성/조회 인터페이스 — 상태별 건수 집계에 사용.
- **Phase 3**: `Order.status` 값이 `RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`로 정확히
  전이되어 있어야 하고, `ProductionJob` 큐 데이터(대기/진행 중 작업)가 조회 가능해야 한다.
  본 Phase는 "승인 시점 가용 재고" 계산 로직에 관여하지 않으며, Phase 3에서 확정한
  `ProductionJob.shortage`/`actualQuantity`/`estimatedTime` 값을 그대로 읽어서 화면에 반영할
  뿐이다.
- **Phase 4**: `Order.status == RELEASE` 전이가 정상 동작해야 상태별 건수 집계에 `RELEASE`가
  정확히 반영된다.

이 시점까지 `docs/Phase0~4.md`가 다른 작업으로 동시에 작성 중일 수 있으므로, 위 인터페이스
이름은 PLAN.md 서술을 근거로 한 **가정**이다. 실제 Phase 0~4 문서/구현에서 Repository
메서드 시그니처가 다르게 확정되면 본 Phase의 Model 설계는 그 시그니처에 맞춰 조정한다
(핵심은 "읽기 전용 집계"라는 책임 자체는 변하지 않는다는 점).

### 2.2 이후 Phase가 이 Phase에 기대하는 산출물

- **Phase 6(더미 데이터 생성기)**: 더미 데이터 생성 후 "메인 메뉴 요약 정보"와 "모니터링
  화면"에 즉시 반영되어야 하므로(PLAN.md Phase 6 완료 기준), 본 Phase가 만드는 집계 로직은
  Repository의 최신 상태를 매번 다시 읽어 계산하는 방식이어야 한다(캐시된 값을 별도로
  들고 있다가 갱신을 누락하는 방식 금지).
- **Phase 7(통합 마무리)**: 전체 시스템 테스트에서 모니터링 화면이 실제 주문/생산/재고
  변화를 정확히 반영하는지 검증하는 기준점이 된다.

---

## 3. 작업 항목 체크리스트

### 3.1 Model 계층

- [ ] `OrderStatusSummary` (또는 동등한 이름) 값 객체 정의
  - 필드: `reservedCount`, `confirmedCount`, `producingCount`, `releaseCount` (모두 `size_t`
    또는 `int`)
  - `REJECTED` 상태는 이 객체에 포함하지 않는다 (필드 자체를 두지 않음으로써 실수로 집계에
    끼워 넣는 것을 구조적으로 방지)
- [ ] `StockStatus` enum 정의: `SUFFICIENT`(여유) / `SHORTAGE`(부족) / `DEPLETED`(고갈)
- [ ] `SampleStockView` (또는 동등한 이름) 값 객체 정의
  - 필드: `sampleId`, `sampleName`, `stock`(물리적 재고), `pendingOrderQuantity`(비교 대상
    주문 수량 합), `status`(`StockStatus`)
- [ ] `MainMenuSummary` 값 객체 정의
  - 필드: `registeredSampleCount`, `totalStock`, `totalOrderCount`, `productionQueueCount`
- [ ] `MonitoringService` 클래스 신설 (Model 계층, 순수 집계 로직 — 콘솔 I/O 없음)
  - 생성자: `SampleRepository&`, `OrderRepository&`, `ProductionJobRepository&` 참조 주입
    (Phase 0에서 확정된 실제 Repository 타입명으로 교체)
  - `OrderStatusSummary GetOrderStatusSummary() const`
    — 전체 주문을 순회하며 상태별 카운트, `REJECTED`는 건너뜀
  - `std::vector<SampleStockView> GetSampleStockViews() const`
    — 전체 시료를 순회하며 시료별 물리적 `stock`과 판정 상태를 계산
  - `MainMenuSummary GetMainMenuSummary() const`
    — 등록 시료 수, 총 재고 합, 전체 주문 수(`REJECTED` 제외 — 3.4절 참고), 생산 라인 대기
      건수를 계산
  - `StockStatus JudgeStockStatus(int stock, int pendingOrderQuantity) const` (private 헬퍼)
    — 3.4절의 판정 규칙을 그대로 구현
- [ ] 위 로직은 View/Controller에 절대 섞지 않는다 (CLAUDE.md 아키텍처 원칙 — "View나
  Controller에 도메인 로직이 섞이지 않도록 한다")

### 3.2 Controller 계층

- [ ] `MonitoringController` 클래스 신설
  - 메인 메뉴에서 "모니터링" 메뉴 진입 시 호출되는 진입점 (`Show()` 또는 `Run()`)
  - `MonitoringService`를 호출해 `OrderStatusSummary`, `vector<SampleStockView>`를 얻고
    `MonitoringView`에 전달해 출력만 위임
  - Controller 자체에는 집계 계산식이 없어야 한다 (Model 호출 결과를 그대로 View에 넘김)
- [ ] 메인 메뉴 Controller(기존 또는 신설 `MainMenuController`)에서 화면 진입/갱신 시마다
  `MonitoringService::GetMainMenuSummary()`를 호출해 요약 정보를 `MainMenuView`에 전달하도록
  연결 (Phase 0에서 이미 메인 메뉴 골격이 있다면 이 Phase에서 요약 정보 연동 부분만 추가)

### 3.3 View 계층

- [ ] `MonitoringView` 클래스 신설
  - `PrintOrderStatusSummary(const OrderStatusSummary&)` — 상태별 건수를 표 형태로 출력
    (`RESERVED`/`CONFIRMED`/`PRODUCING`/`RELEASE` 4개 항목만, `REJECTED` 언급 없음)
  - `PrintSampleStockStatus(const std::vector<SampleStockView>&)` — 시료 ID/이름/현재
    재고/여유·부족·고갈 상태를 표 형태로 출력
- [ ] `MainMenuView`(기존)에 `PrintSummary(const MainMenuSummary&)` 메서드 추가 또는 확장
  — 등록 시료 수 / 총 재고 / 전체 주문 수 / 생산 라인 대기 건수를 메인 메뉴 상단에 출력

### 3.4 핵심 판정/집계 규칙 정의 (설계 확정 필요 항목)

이 절은 PRD가 "자유롭게 결정 가능"이라 명시한 표시 수준을 제외하고, 구현에 필요한 최소
판정 기준을 명확히 하기 위해 이번 Phase에서 확정한다.

- **여유/부족/고갈 판정**은 물리적 `stock` 값을 기준으로 하며, Phase 3의 "승인 시점 가용
  재고"(아직 배정되지 않은 재고를 추적하는 계산용 값)와는 **완전히 별개의 값**이다.
  모니터링 화면은 오직 `Sample.stock`(실제 재고)만 사용한다. PRD 7.4 예시2에서 명시한 대로,
  물리적 재고가 이미 앞선 주문 출고분으로 예정되어 있어도 모니터링에는 물리적 재고 그대로
  표시한다.
- **판정 우선순위** (PRD 7.5 문구 순서 그대로 우선순위로 채택):
  1. `stock == 0` → `DEPLETED`(고갈)
  2. `stock > 0` 이고 `stock < pendingOrderQuantity` → `SHORTAGE`(부족)
  3. 그 외(`stock > 0` 이고 `stock >= pendingOrderQuantity`, 또는 비교 대상 주문이 없는 경우)
     → `SUFFICIENT`(여유)
- **`pendingOrderQuantity`(주문 대비 비교 수량)의 정의 (확정)**: 해당 시료를 대상으로 하는
  주문 중 상태가 `PRODUCING`인 주문의 `quantity` 합계만을 `pendingOrderQuantity`로 삼는다.
  ```
  pendingOrderQuantity = Σ(해당 시료에 대해 PRODUCING 상태인 주문의 quantity)
  ```
  - `RESERVED` 주문은 아직 승인 여부가 결정되지 않았으므로 재고 압박 요인에 포함하지 않는다
    (승인/거절 판단이 나기 전까지는 실제로 재고를 소진할지 알 수 없음).
  - `CONFIRMED` 주문은 이미 물리적 재고 충분이 확인되어 출고만 남은 상태이므로 제외한다
    (재고가 실제로 그 주문에 충분함이 이미 보장된 상태).
  - `RELEASE`/`REJECTED` 주문은 더 이상 재고에 대한 압박 요인이 아니므로 제외한다.
  - 이 정의는 확정된 설계이며 추가 확인이 필요하지 않다.

- **`totalOrderCount`(메인 메뉴 "전체 주문 수")의 정의**: CLAUDE.md/PRD가 "REJECTED는
  모니터링 집계에서 제외"라고 명시한 원칙을 메인 메뉴 요약에도 동일하게 적용해, `REJECTED`를
  제외한 주문 건수(`RESERVED`+`CONFIRMED`+`PRODUCING`+`RELEASE`)로 정의한다.

  > **TODO: 확인 필요** — PRD 7.1은 "전체 주문 수"라고만 되어 있어 `REJECTED` 포함 여부가
  > 명시적이지 않다. 일관성을 위해 제외로 채택했으나, 사용자 의도와 다를 수 있어 확인이
  > 필요하다.

- **`productionQueueCount`(메인 메뉴 "생산 라인 대기 건수")의 정의**: 주문 상태가 `PRODUCING`인
  주문에 연결된 `ProductionJob` 항목 수(= 아직 `CONFIRMED`로 전환되지 않은, 생산 큐에
  남아 있는 작업 수)로 정의한다. "현재 처리 중인 1건 + 대기열"을 모두 포함하는 값이며, Phase 6의
  생산 라인 화면(진행 중 1건 vs 대기열 N건 구분 표시)과는 별개로, 메인 메뉴에서는 총합만
  보여준다.

---

## 4. 인터페이스/데이터 스키마

본 Phase는 JSON 스키마를 새로 추가하지 않는다 (읽기 전용 집계). 다만 Model 계층에 아래
값 객체/서비스 시그니처를 신설한다.

```cpp
// Model/MonitoringTypes.h (파일명은 실제 프로젝트 컨벤션에 맞춰 조정)

enum class StockStatus {
    SUFFICIENT, // 여유
    SHORTAGE,   // 부족
    DEPLETED    // 고갈
};

struct OrderStatusSummary {
    int reservedCount = 0;
    int confirmedCount = 0;
    int producingCount = 0;
    int releaseCount = 0;
    // REJECTED는 의도적으로 필드 자체가 없음
};

struct SampleStockView {
    std::string sampleId;
    std::string sampleName;
    int stock = 0;                 // 물리적 재고 (Sample.stock 그대로)
    int pendingOrderQuantity = 0;   // PRODUCING 상태 주문의 quantity 합 (3.4절 확정 정의)
    StockStatus status = StockStatus::SUFFICIENT;
};

struct MainMenuSummary {
    int registeredSampleCount = 0;
    int totalStock = 0;
    int totalOrderCount = 0;        // REJECTED 제외
    int productionQueueCount = 0;   // PRODUCING 상태 주문에 연결된 ProductionJob 수
};
```

```cpp
// Model/MonitoringService.h

class MonitoringService {
public:
    MonitoringService(SampleRepository& sampleRepo,
                       OrderRepository& orderRepo,
                       ProductionJobRepository& jobRepo);

    OrderStatusSummary GetOrderStatusSummary() const;
    std::vector<SampleStockView> GetSampleStockViews() const;
    MainMenuSummary GetMainMenuSummary() const;

private:
    StockStatus JudgeStockStatus(int stock, int pendingOrderQuantity) const;

    SampleRepository& sampleRepo_;
    OrderRepository& orderRepo_;
    ProductionJobRepository& jobRepo_;
};
```

```cpp
// View/MonitoringView.h

class MonitoringView {
public:
    void PrintOrderStatusSummary(const OrderStatusSummary& summary) const;
    void PrintSampleStockStatus(const std::vector<SampleStockView>& views) const;
};
```

```cpp
// Controller/MonitoringController.h

class MonitoringController {
public:
    MonitoringController(MonitoringService& service, MonitoringView& view);
    void Show() const; // 모니터링 메뉴 진입 시 호출
private:
    MonitoringService& service_;
    MonitoringView& view_;
};
```

> Repository 타입명(`SampleRepository`/`OrderRepository`/`ProductionJobRepository`)과 실제
> 조회 메서드 시그니처(`FindAll()` 등)는 Phase 0 확정 결과를 따른다. 여기서는 개념적
> 이름만 제시하며, 실제 구현 시 Phase 0 산출물의 정확한 이름으로 치환한다.

---

## 5. 핵심 비즈니스 로직 주의사항

- **REJECTED 제외 원칙**: `OrderStatusSummary`에는 `REJECTED`를 담을 필드 자체를 두지 않는다.
  집계 루프에서 `if (order.status == OrderStatus::REJECTED) continue;` 형태로 명시적으로
  건너뛰어, "필드는 있는데 항상 0으로만 채워지는" 형태로 방치하지 않는다.
- **물리적 재고 vs 가용 재고 혼동 금지 (CLAUDE.md 필수 반영 사항)**: 모니터링 화면(상태별
  집계, 시료별 재고 현황, 메인 메뉴 요약의 "총 재고")은 예외 없이 `Sample.stock`(물리적
  재고)만 사용한다. Phase 3에서 도입되는 "승인 시점 가용 재고"(주문 승인 계산 전용, 아직
  배정되지 않은 재고를 추적하는 값)는 `MonitoringService`가 절대 참조하지 않는다. PRD 7.4
  예시2("모니터링 화면에는 물리적 재고 90개가 그대로 표시되어야 함")를 그대로 만족해야 한다.
- **읽기 전용, 재계산 없음**: `MonitoringService`는 상태를 변경하지 않는다(주문 상태 전이,
  재고 증감 등은 Phase 2~4의 책임). 매 호출 시 Repository에서 최신 데이터를 다시 읽어
  계산하며, 자체적으로 값을 캐시해 두었다가 stale한 값을 보여주지 않는다.
- **판정 우선순위 고정**: `stock == 0`이면 `pendingOrderQuantity` 값과 무관하게 무조건
  `DEPLETED`로 판정한다(PRD 7.5 "고갈: 재고 수량이 0"을 최우선 규칙으로 채택).

---

## 6. 테스트 계획 (tester 에이전트 작성 대상)

### 6.1 `OrderStatusSummary` 집계 단위 테스트

- 주문이 하나도 없을 때 모든 카운트가 0인지
- `RESERVED`/`CONFIRMED`/`PRODUCING`/`RELEASE` 각각 여러 건이 섞여 있을 때 정확히 분류되는지
- `REJECTED` 주문이 다수 포함돼도 `OrderStatusSummary`의 어떤 카운트에도 영향을 주지 않는지
  (합계 검증: 전체 주문 수 - REJECTED 수 == 4개 필드 합)

### 6.2 `SampleStockView` / `StockStatus` 판정 단위 테스트

- `stock == 0` → 비교 대상 주문 수량과 무관하게 `DEPLETED` (경계값: pendingOrderQuantity가
  0이어도 `DEPLETED`)
- `stock > 0` 이고 `stock < pendingOrderQuantity` → `SHORTAGE`
- `stock > 0` 이고 `stock == pendingOrderQuantity` → `SUFFICIENT` (경계값, "충분"의 등호 포함
  여부 검증)
- `stock > 0` 이고 비교 대상 주문이 아예 없음(`pendingOrderQuantity == 0`) → `SUFFICIENT`
- `pendingOrderQuantity` 계산 시 `RESERVED` 상태 주문은 합산에서 제외되는지 (예: 동일 시료에
  `RESERVED` 수량 100 + `PRODUCING` 수량 10이 있을 때 `pendingOrderQuantity == 10`이어야 함,
  `RESERVED` 100이 더해져 110이 되면 안 됨)
- `CONFIRMED`/`RELEASE`/`REJECTED` 상태 주문도 `pendingOrderQuantity` 합산에서 제외되는지
- 여러 시료가 섞여 있을 때 시료별로 독립적으로 판정되는지

### 6.3 물리적 재고 vs 가용 재고 회귀 테스트 (PRD 7.4 예시2 기반, code-review 시 필수 확인)

- Given: 시료 A, `stock = 90`, 수율 1.0. 주문1(수량 100, 승인 완료, 부족분 10 → `PRODUCING`),
  주문2(수량 10, 승인 완료, Phase 3 규칙상 가용 재고 0 취급 → `PRODUCING`)
- When: `MonitoringService::GetSampleStockViews()` 호출
- Then: 시료 A의 `SampleStockView.stock == 90` (Phase 3의 가용 재고 계산 결과가 아닌 물리적
  재고 그대로 노출되는지 확인)

### 6.4 `MainMenuSummary` 단위 테스트

- 등록 시료 수 = Repository의 시료 개수와 일치
- 총 재고 = 모든 시료의 `stock` 합
- 전체 주문 수 = `REJECTED`를 제외한 주문 개수
- 생산 라인 대기 건수 = `PRODUCING` 상태 주문에 연결된 `ProductionJob` 개수와 일치
  (Phase 3에서 이미 `CONFIRMED`로 전환된 완료 작업은 제외되는지 확인)

### 6.5 Controller/View 계층 테스트 (GMock 활용)

- `MonitoringController::Show()` 호출 시 `MonitoringService`의 조회 메서드가 정확히 호출되고,
  반환값이 `MonitoringView`의 출력 메서드로 그대로 전달되는지 Mock으로 검증
  (Controller에 집계 로직이 새어 들어가지 않았는지 확인하는 목적)
- `MonitoringView` 출력 포맷 테스트는 표준출력 캡처 방식으로 상태별 4개 항목/여유·부족·고갈
  라벨이 모두 출력되는지 확인 (Phase 0에서 확립된 콘솔 출력 테스트 방식을 재사용)

### 6.6 통합(시스템) 테스트

- 실제 JSON 파일 기반 Repository에 시료/주문/생산 큐 더미 데이터를 적재한 뒤 애플리케이션을
  구동해 모니터링 메뉴 진입 → 화면 출력값이 파일 데이터와 일치하는지 end-to-end로 확인
- 주문 승인/거절/생산완료/출고 등 상태 변화를 일으킨 뒤 모니터링 화면을 재조회해 값이 즉시
  갱신되는지 확인 (캐시 없이 매번 재계산됨을 증명)

---

## 7. 완료 조건 (Definition of Done)

- [ ] `MonitoringService`, `MonitoringController`, `MonitoringView`, 관련 값 객체
  (`OrderStatusSummary`, `SampleStockView`, `MainMenuSummary`, `StockStatus`)가 구현되고
  MVC 계층 분리 원칙을 어기지 않는다 (Model에 집계 로직, Controller는 흐름 제어만, View는
  출력만).
- [ ] 메인 메뉴 진입/갱신 시 `MainMenuSummary`가 실제 Repository 데이터를 반영해 출력된다.
- [ ] 모니터링 메뉴에서 상태별 주문 건수(4개 상태, `REJECTED` 제외)와 시료별 재고 현황
  (여유/부족/고갈)이 정확히 출력된다.
- [ ] 모니터링 화면이 물리적 `stock`만 사용하고 Phase 3의 승인 계산용 가용 재고를 절대
  참조하지 않음이 테스트로 증명된다 (6.3 회귀 테스트 통과).
- [ ] 6장에 정의된 단위/통합 테스트가 `tester` 에이전트에 의해 작성되고 전부 통과한다.
- [ ] `code-review` 에이전트 검토에서 Clean Code/SOLID/함수 라인 수/디자인 패턴 사용에 대한
  주요 지적 사항이 없거나, 있었다면 반영 완료됐다.
- [ ] `supervisor`가 본 문서(`docs/Phase5.md`) 및 CLAUDE.md/PRD.md와의 부합 여부를 확인해
  이상 없음을 보고한다.
- [ ] 3.4절의 남은 `TODO: 확인 필요` 항목(전체 주문 수 REJECTED 포함 여부)에 대해 사용자
  확답을 받아 문서에 반영했다. (`pendingOrderQuantity` 산정 범위는 확정 완료.)

---

## 8. 미확정/확인 필요 사항 요약

1. **메인 메뉴 "전체 주문 수"에 `REJECTED` 포함 여부** — 모니터링 집계(7.5)의 "REJECTED 제외"
   원칙을 메인 메뉴 요약에도 동일 적용하는 것으로 기본값을 채택했으나, PRD 7.1에는 명시적
   언급이 없어 확인이 필요하다.

> `pendingOrderQuantity`(재고 판정용 "주문 대비" 비교 수량) 산정 범위는 확정됐다 (3.4절
> 참고): `PRODUCING` 상태 주문의 `quantity` 합만 사용하며, `RESERVED`는 제외한다.
