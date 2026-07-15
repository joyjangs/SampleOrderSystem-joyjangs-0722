# Phase 5 — 모니터링 시스템

> 이 문서는 `feature/phase3-approval-production` 브랜치의 Phase 0~4 실제 구현(코드 확인 완료)을
> 전제로 재작성됐다. 이전 초안은 Phase 0~4 실제 코드를 아직 볼 수 없는 상태에서 작성돼 다음
> 지점이 실제와 어긋나 있었다. 이번 갱신에서 모두 바로잡았다.
>
> **이전 초안과 실제 코드가 달랐던 지점 요약**
> - Repository 조회 메서드는 `findAll()`이 아니라 **`FindAll()`**(PascalCase)이다
>   (`Model::SampleRepository`/`Model::OrderRepository`/`Model::ProductionJobRepository`는 모두
>   `Model::JsonFileRepository<T>`를 상속). 인터페이스 이름은 "가정"이 아니라 실제 확정된
>   시그니처로 아래 전부 치환했다.
> - `OrderStatus` enum 값은 `RESERVED`/`CONFIRMED`/... 같은 대문자가 아니라
>   **`Reserved`/`Rejected`/`Producing`/`Confirmed`/`Release`**(PascalCase) C++ enum 값이다.
>   문자열 `"RESERVED"` 등은 `Model::ToString(OrderStatus)`가 반환하는 표시용 값일 뿐이다.
> - **`ProductionJobRepository`는 이미 "생산 큐에 남아 있는 작업만" 담고 있다.**
>   `Model::ProductionLine::CompleteFrontJob()`(`Model/ProductionLine.cpp` 35~49행)이 작업 완료
>   시 `repository_.Remove(completed->GetOrderId())`로 즉시 제거하기 때문에, 완료된
>   `ProductionJob`은 저장소에 남지 않는다. 따라서 이전 초안이 제안한 "PRODUCING 상태 주문에
>   연결된 ProductionJob 수를 별도로 세는" 방식은 불필요한 과설계다 —
>   **`productionJobRepository.FindAll().size()`가 곧 생산 라인 대기 건수**다(현재 진행 중 1건 +
>   대기열 포함, PRD 7.6과 정확히 일치).
> - `Model::Order`/`Model::Sample`에는 여전히 로우레벨 setter(`SetStatus`/`SetUpdatedAt`,
>   `SetStock`)만 있고 상태 전이/재고 계산 전용 메서드가 없다 — Phase 3/4와 동일하게, 새 상태
>   전이가 없는 이번 Phase는 **읽기 전용 조회만** 하므로 이 setter들을 아예 쓰지 않는다.
> - `Model::CalculateAvailableStock(sample, orders)`(`Model/AvailableStockCalculator.h`)가 이미
>   존재하며, 이것이 바로 CLAUDE.md가 "모니터링과 혼동 금지"라고 못박은 **"승인 계산용 가용
>   재고"**다. 모니터링은 이 함수를 절대 호출하지 않고 `Sample::GetStock()`(물리적 재고)만
>   사용한다 — 이름이 비슷해 헷갈리기 쉬우므로 코드 리뷰 시 반드시 확인해야 하는 지점이다.
> - `Controller::MainMenuController`에는 **`MainMenuOption::Monitoring = 4`가 이미 enum으로
>   정의돼 있지만, `main.cpp`의 서브메뉴 라우팅 맵에는 아직 등록되어 있지 않다**
>   (`kAllMenuOptions`에는 포함되어 "선택 시 미구현 안내"만 뜬다). 이번 Phase가 이 자리를
>   채운다.
> - **`View::MainMenuView`는 현재 요약 정보를 표시할 방법 자체가 없다** — 메서드가
>   `ShowMainMenu()`/`ShowNotImplemented()`/`ShowInvalidSelection()`/`ShowExitMessage()`
>   4개뿐이고 전부 인자를 받지 않는다. 다른 View들(`ReleaseView`, `OrderApprovalView`,
>   `ProductionLineView`)과 달리 **`virtual`이 아니다** — `Tests/ControllerTests.cpp`가
>   `Controller::MainMenuController`를 GMock View 없이 실제 `View::MainMenuView` 인스턴스로
>   테스트하기 때문이다. 이 컨벤션 차이는 이번 Phase가 존중해야 할 기존 설계다(아래 3.3/3.4절
>   참고).
> - `Controller::MainMenuController`의 생성자는 현재 `(View::MainMenuView&, map<...>)` 2개
>   인자만 받고 **Model을 전혀 참조하지 않는다**. 메인 메뉴 요약 정보를 보여주려면 이 생성자에
>   `Model::MonitoringService&`를 추가해야 하며, 이는 `Tests/ControllerTests.cpp`의 기존 3개
>   테스트 케이스가 깨지는 **하위 호환성 없는 변경**이다(3.4절/7절에서 다룸).
> - `Controller::ISubMenuController`(`virtual void Run() = 0;`)와
>   `Controller::MainMenuController`의 map 기반 라우팅(`{ToMenuOptionKey(option), controller}`)은
>   Phase 3/4와 동일하게 그대로 재사용 가능 — 이 부분은 이전 초안의 가정이 맞았다.

## 1. 목표 요약 (PLAN.md 인용)

> **목표**: PRD 7.5 / CLAUDE.md 데이터 모니터링 요구사항.
>
> - 상태별(`RESERVED`/`CONFIRMED`/`PRODUCING`/`RELEASE`) 주문 건수 집계 (`REJECTED` 제외)
> - 시료별 재고 현황 및 여유/부족/고갈 상태 표기 (물리적 `stock` 기준 — 승인 계산용 가용
>   재고와 혼동하지 않음)
> - 메인 메뉴 요약 정보(등록 시료 수, 총 재고, 전체 주문 수, 생산 라인 대기 건수) 연동
>
> **완료 기준**: 모니터링 화면이 실제 Repository 데이터를 반영해 정확히 표시된다.

PRD 7.1(메인 메뉴), 7.5(모니터링) 원문은 이전 초안이 이미 정확히 인용했으므로 그대로 유지한다
(아래 3.4/6절에서 판정 규칙과 함께 다시 인용).

Phase 5는 상태 전이(`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASE` 확정, 승인 시점
생산량 확정 규칙, 출고 처리)를 재구현하지 않는다. Phase 2~4가 만든 데이터를 **읽기 전용으로
집계·표시**하는 것이 유일한 책임이다.

---

## 2. 전제 (Phase 0~4 실제 코드 확인 결과)

- `Model::SampleRepository : Model::JsonFileRepository<Sample>` — `FindAll()`(전체 목록),
  `FindById(id)`, `SearchByName(keyword)`, `Add`/`Update`/`Remove`/`Save`/`Load` 제공. 기본 경로
  `data/samples.json`.
- `Model::OrderRepository : Model::JsonFileRepository<Order>` — `FindAll()`/`FindById`/`Add`/
  `Update`/`Remove`/`Save`/`Load`. 기본 경로 `data/orders.json`. 별도의 `FindByStatus`는 없음 —
  Phase 3/4의 `OrderApprovalController`/`ReleaseController`도 `FindAll()` 후 컨트롤러 쪽에서
  직접 상태로 필터링하는 방식을 쓴다.
- `Model::ProductionJobRepository : Model::JsonFileRepository<ProductionJob>` — 주문 1건당
  `ProductionJob` 0~1개(주석: "Keyed by orderId — one order maps to at most one production
  job"). 완료된 작업은 `Model::ProductionLine::CompleteFrontJob()`이 `Remove()`로 즉시
  삭제하므로, **`FindAll()` 결과 자체가 이미 "현재 생산 중 + 대기 중"인 작업만 담고 있다.**
- `Model::Sample`(`Model/Sample.h`) — `GetId()`/`GetName()`/`GetStock()`/`GetYieldRate()`/
  `GetAvgProductionTime()`만 조회하면 충분. `SetStock` 등 변경 메서드는 이번 Phase에서 호출하지
  않는다(읽기 전용).
- `Model::Order`(`Model/Order.h`, `Model/OrderStatus.h`) — `GetSampleId()`/`GetQuantity()`/
  `GetStatus()`(`OrderStatus` enum: `Reserved`/`Rejected`/`Producing`/`Confirmed`/`Release`)
  조회만 사용. `Model::ToString(OrderStatus)`가 대문자 표시 문자열(`"RESERVED"` 등)로 변환한다 —
  View 출력 시 이 함수를 재사용할 수 있다.
- `Model::CalculateAvailableStock(const Sample&, const std::vector<Order>&)`
  (`Model/AvailableStockCalculator.h`) — Phase 3의 "승인 시점 가용 재고" 계산 전용 함수(물리적
  `stock`에서 `CONFIRMED`/`PRODUCING` 주문 수량 합을 뺀 값, 0으로 클램프). **`MonitoringService`는
  이 함수를 절대 호출하지 않는다** — CLAUDE.md/PRD 7.4 예시2가 요구하는 "모니터링에는 물리적
  재고만 표시"를 만족하려면 이 함수와 완전히 분리된 경로로 `Sample::GetStock()`만 읽어야 한다.
- `Controller::ISubMenuController`(`virtual void Run() = 0;`), `Controller::MainMenuController`
  (`std::map<std::string, std::reference_wrapper<ISubMenuController>>` 기반 라우팅,
  `MainMenuOption::Monitoring = 4`가 이미 enum에 정의돼 있고 `ToMenuOptionKey`로 문자열 키
  변환) — Phase 3/4와 동일한 확정 컨벤션. 다만 `main.cpp`의 서브메뉴 맵에는 아직
  `MainMenuOption::Monitoring` 항목이 없다(Phase 5가 채워야 할 자리, `Controller/
  MainMenuController.cpp`의 `kAllMenuOptions` 배열에는 이미 포함돼 있어 미등록 상태에서는
  "아직 구현되지 않은 기능입니다" 안내만 뜬다).
- `View` 계층 컨벤션(`OrderApprovalView`/`ReleaseView`/`ProductionLineView`) — 모든 메서드가
  `virtual ... const`, 가상 소멸자, Model 값 타입(`Order`, `Sample` 등)을 인자로만 받고 Model
  Repository를 직접 참조하지 않음. **단, `View::MainMenuView`는 이 컨벤션에서 벗어난 예외다** —
  메서드가 `virtual`이 아니며(가상 소멸자도 없음), `Tests/ControllerTests.cpp`가
  `Controller::MainMenuController`를 실제 `View::MainMenuView` 인스턴스로만 테스트하고 View를
  목(mock)으로 대체하지 않는다. 이번 Phase가 `MainMenuView`에 요약 출력 메서드를 추가할 때도 이
  기존 컨벤션(비-virtual)을 그대로 따른다 — 다른 View들처럼 새삼 virtual로 바꾸지 않는다(불필요한
  변경/과설계 방지).
- `main.cpp`의 DI 배선 방식: Repository들을 만들고 `Load()` 호출 → 각 서비스/뷰/컨트롤러를
  생성자에서 참조로 엮음 → `MainMenuController`에 `{ToMenuOptionKey(...), controller}` 맵 항목
  추가. 현재 맵에는 `SampleManagement`/`OrderPlacement`/`OrderApproval`/`ProductionLine`/
  `Release` 5개만 등록돼 있고 **`Monitoring`이 빠져 있다.**

---

## 3. 작업 항목 체크리스트

### 3.1 Model 계층

- [ ] `Model/MonitoringTypes.h` 신설(값 객체 전용, 로직 없음):
  - `enum class StockStatus { Sufficient, Shortage, Depleted };` (PascalCase — `OrderStatus`
    enum 컨벤션과 통일. 화면 표시용 한글 라벨 "여유"/"부족"/"고갈"은 View가 매핑한다.)
  - `struct OrderStatusSummary { int reservedCount = 0; int confirmedCount = 0;
    int producingCount = 0; int releaseCount = 0; };` — `REJECTED`용 필드는 아예 두지 않는다.
  - `struct SampleStockView { std::string sampleId; std::string sampleName; int stock = 0;
    int pendingOrderQuantity = 0; StockStatus status = StockStatus::Sufficient; };`
  - `struct MainMenuSummary { int registeredSampleCount = 0; int totalStock = 0;
    int totalOrderCount = 0; int productionQueueCount = 0; };`
- [ ] `Model::MonitoringService` 신설(`Model/MonitoringService.h/.cpp`, 순수 집계 — 콘솔 I/O
  없음, 상태 변경 없음):
  - 생성자: `MonitoringService(SampleRepository& sampleRepository, OrderRepository& orderRepository,
    ProductionJobRepository& productionJobRepository)` — 세 Repository 모두 참조로 주입
    (`OrderApprovalService`/`OrderReleaseService`와 동일한 DI 컨벤션).
  - `OrderStatusSummary GetOrderStatusSummary() const` — `orderRepository_.FindAll()`을 순회하며
    `switch (order.GetStatus())`로 4개 필드 중 하나만 증가시키고, `OrderStatus::Rejected`는
    명시적으로 `continue`(또는 `case`에서 아무 것도 하지 않음)로 건너뛴다.
  - `std::vector<SampleStockView> GetSampleStockViews() const` — `sampleRepository_.FindAll()`을
    순회하며 시료별로 `PendingOrderQuantityFor(sampleId, allOrders)`를 구해 `JudgeStockStatus`로
    상태를 매긴다. `allOrders`는 루프 시작 전 `orderRepository_.FindAll()`로 한 번만 읽어 재사용
    (시료 수만큼 반복 조회하지 않도록).
  - `MainMenuSummary GetMainMenuSummary() const`:
    - `registeredSampleCount` = `sampleRepository_.FindAll().size()`
    - `totalStock` = `Σ sample.GetStock()` (전체 시료)
    - `totalOrderCount` = `REJECTED`를 제외한 주문 수(3.4절 참고, 확답 대기 중인 TODO)
    - `productionQueueCount` = `productionJobRepository_.FindAll().size()` (완료된 작업은
      이미 Repository에서 제거되어 있으므로 별도 필터링 불필요 — 2절 참고)
  - `private: StockStatus JudgeStockStatus(int stock, int pendingOrderQuantity) const;` — 3.4절
    판정 규칙 그대로 구현.
  - `private: int PendingOrderQuantityFor(const std::string& sampleId,
    const std::vector<Order>& allOrders) const;` — 해당 `sampleId`이면서
    `status == OrderStatus::Producing`인 주문의 `quantity` 합산(3.4절 확정 정의).
- [ ] 이 로직은 View/Controller에 절대 섞지 않는다(CLAUDE.md 아키텍처 원칙).

### 3.2 Controller 계층

- [ ] `Controller::MonitoringController : Controller::ISubMenuController` 신설
  (`Controller/MonitoringController.h/.cpp`):
  - 생성자: `MonitoringController(Model::MonitoringService& service, View::MonitoringView& view)`.
  - `void Run() override`:
    1. `view_.ShowOrderStatusSummary(service_.GetOrderStatusSummary())`
    2. `view_.ShowSampleStockStatus(service_.GetSampleStockViews())`
  - 집계 계산식이 Controller에 새어 들어가지 않도록 한다(Model 호출 결과를 그대로 View에 위임).
  - `ProductionLineController`(반복 메뉴, "새로고침"/"완료 처리" 선택지 있음)와 달리 모니터링은
    단순 조회 화면이라 서브메뉴 루프가 필요 없다(`SampleController`의 목록 조회 흐름과 동급의
    단순 구조로 충분 — 과설계 방지).
- [ ] `Controller::MainMenuController` 생성자에 `Model::MonitoringService&` 파라미터 추가:
  ```cpp
  MainMenuController(View::MainMenuView& view, Model::MonitoringService& monitoringService,
                      std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers);
  ```
  - `Run()` 루프에서 기존 `view_.ShowMainMenu()` 호출 **직전**에
    `view_.ShowSummary(monitoringService_.GetMainMenuSummary())`를 추가한다.
  - `HandleInput(const std::string&)`은 요약 출력과 무관하므로 변경하지 않는다(기존 라우팅
    로직 그대로 유지).
  - **주의(하위 호환성 없는 변경)**: 이 생성자 시그니처 변경으로 `Tests/ControllerTests.cpp`의
    기존 3개 테스트(`HandleInputWithZeroRequestsExit` 등)가 컴파일되지 않는다. `code-review`/
    `tester` 단계에서 반드시 함께 갱신해야 한다(7.4절 참고). 대안으로 "메인 메뉴 요약은
    `MainMenuController`가 아니라 `main.cpp`가 루프 밖에서 직접 출력"하는 방식도 검토했으나,
    `MainMenuController::Run()`이 메뉴 출력 루프 자체를 소유하고 있어(3.1의 `while
    (!isExitRequested_) { view_.ShowMainMenu(); ... }`) 요약을 "매번 메뉴 직전에" 갱신하려면
    루프 내부에서 호출하는 것이 유일하게 자연스러운 지점이다 — `main.cpp`가 별도로 요약을
    반복 출력하게 하면 두 군데(main.cpp/MainMenuController)에 화면 흐름 제어가 나뉘어 오히려
    책임이 흩어진다.
- [ ] `main.cpp`에 `Model::MonitoringService monitoringService(sampleRepository, orderRepository,
  productionJobRepository);`, `View::MonitoringView monitoringView;`,
  `Controller::MonitoringController monitoringController(monitoringService, monitoringView);` 생성
  → `MainMenuController` 생성자 호출에 `monitoringService` 인자 추가 → 서브메뉴 맵에
  `{Controller::ToMenuOptionKey(Controller::MainMenuOption::Monitoring), monitoringController}`
  등록(기존 `MainMenuOption::Monitoring = 4`를 그대로 사용, 새 enum 값 불필요).

### 3.3 View 계층

- [ ] `View::MonitoringView` 신설(`View/MonitoringView.h/.cpp`), 다른 서브메뉴 View
  (`ReleaseView`/`OrderApprovalView`)와 동일한 컨벤션(모든 메서드 `virtual ... const`, 가상
  소멸자, Model 값 타입만 인자로 받음 — `Model::MonitoringTypes.h`의 값 객체를 포함):
  - `virtual void ShowOrderStatusSummary(const Model::OrderStatusSummary& summary) const;` —
    `RESERVED`/`CONFIRMED`/`PRODUCING`/`RELEASE` 4개 항목만 표로 출력(`REJECTED` 언급 없음).
  - `virtual void ShowSampleStockStatus(const std::vector<Model::SampleStockView>& views) const;`
    — 시료 ID/이름/현재 재고/여유·부족·고갈 상태를 표로 출력. `StockStatus` → 한글 라벨
    ("여유"/"부족"/"고갈") 매핑은 이 View 내부(또는 이 View만 쓰는 익명 네임스페이스 헬퍼)에
    둔다(Model이 UI 문자열을 갖지 않도록).
- [ ] `View::MainMenuView`에 `void ShowSummary(const Model::MainMenuSummary& summary) const;`
  메서드 추가(등록 시료 수 / 총 재고 / 전체 주문 수 / 생산 라인 대기 건수를 메인 메뉴 상단에
  출력). 기존 4개 메서드처럼 **non-virtual로 추가**한다(2절에서 확인한 기존 컨벤션 유지, 다른
  View들처럼 virtual로 바꾸지 않음 — `Tests/ControllerTests.cpp`가 실제 `MainMenuView`
  인스턴스를 그대로 쓰는 방식과 충돌하지 않도록).
  - `Model/MonitoringTypes.h`를 `View/MainMenuView.h`에 `#include`해야 한다(View가 Model의 값
    타입에 의존하는 것은 `ReleaseView`가 `Model::Order`를 인자로 받는 것과 동일한 기존 패턴이라
    MVC 의존 방향을 어기지 않는다 — Model이 View를 모르는 단방향 의존은 유지된다).

### 3.4 핵심 판정/집계 규칙 정의 (설계 확정 필요 항목)

- **여유/부족/고갈 판정은 물리적 `Sample::GetStock()` 값 기준**이며, Phase 3의
  `Model::CalculateAvailableStock`(승인 계산 전용, 아직 배정되지 않은 재고를 추적)와는 **완전히
  별개**다. `MonitoringService`는 `CalculateAvailableStock`을 어디에서도 호출하지 않는다. PRD
  7.4 예시2("모니터링 화면에는 물리적 재고 90개가 그대로 표시되어야 함")를 그대로 만족해야 한다.
- **판정 우선순위**(PRD 7.5 문구 순서 그대로 채택):
  1. `stock == 0` → `Depleted`(고갈) — `pendingOrderQuantity` 값과 무관하게 최우선.
  2. `stock > 0` 이고 `stock < pendingOrderQuantity` → `Shortage`(부족)
  3. 그 외(`stock > 0` 이고 `stock >= pendingOrderQuantity`, 또는 비교 대상 주문이 없는 경우)
     → `Sufficient`(여유)
- **`pendingOrderQuantity`(주문 대비 비교 수량) 정의(확정)**: 해당 시료를 대상으로 하는 주문 중
  상태가 `Producing`인 주문의 `quantity` 합계만 사용한다.
  ```
  pendingOrderQuantity = Σ(해당 시료에 대해 Producing 상태인 주문의 quantity)
  ```
  - `Reserved` 주문은 승인/거절이 아직 결정되지 않아 재고 압박 요인으로 보지 않는다.
  - `Confirmed` 주문은 승인 시점에 이미 물리적 재고 충분이 보장된 상태(생산 완료를 거쳤든 즉시
    확정이든)이므로 제외한다.
  - `Release`/`Rejected` 주문은 더 이상 재고 압박 요인이 아니므로 제외한다.
  - 이 정의는 확정된 설계이며 추가 확인이 필요하지 않다.
- **`totalOrderCount`(메인 메뉴 "전체 주문 수") 정의**: CLAUDE.md/PRD가 "REJECTED는 모니터링
  집계에서 제외"라고 명시한 원칙을 메인 메뉴 요약에도 동일 적용해 `REJECTED`를 제외한 주문
  건수(`Reserved`+`Confirmed`+`Producing`+`Release`)로 정의한다.
  > **TODO: 확인 필요** — PRD 7.1은 "전체 주문 수"라고만 되어 있어 `REJECTED` 포함 여부가
  > 명시적이지 않다. 이전 초안과 동일하게 "제외"를 기본값으로 채택했으나 사용자 확인이 필요하다.
- **`productionQueueCount`(메인 메뉴 "생산 라인 대기 건수") 정의(재확정, 이전 초안보다 단순화)**:
  `productionJobRepository.FindAll().size()`. 완료된 작업은 `ProductionLine::CompleteFrontJob()`
  이 이미 `Remove()`로 지워버리므로, 저장소에 남아 있는 항목 수가 곧 "현재 처리 중인 1건(있다면)
  + 대기열"의 총합이다(PRD 7.6). 이전 초안이 제안한 "PRODUCING 상태 주문에 연결된 개수를 별도로
  세는" 접근은 불필요하다 — `Order.status == Producing`인 주문 수와 `ProductionJob` 개수는
  정상 흐름에서 항상 1:1(주문 승인 시 큐 등록, 생산 완료 시 큐 제거와 주문 상태 전환이 같은
  시점에 함께 일어남)이므로 더 단순한 쪽(Repository 크기)을 신뢰 가능한 단일 소스로 채택한다.

---

## 4. 인터페이스/데이터 스키마

본 Phase는 JSON 영속 스키마를 추가/변경하지 않는다(읽기 전용 집계). Model 계층에 아래 값
객체/서비스를 신설한다.

```cpp
// Model/MonitoringTypes.h (신설)
#pragma once

#include <string>

namespace Model {

enum class StockStatus {
    Sufficient,  // 여유
    Shortage,    // 부족
    Depleted     // 고갈
};

struct OrderStatusSummary {
    int reservedCount = 0;
    int confirmedCount = 0;
    int producingCount = 0;
    int releaseCount = 0;
    // Rejected는 의도적으로 필드 자체가 없음 (REJECTED는 모니터링 집계 제외, PRD 7.5)
};

struct SampleStockView {
    std::string sampleId;
    std::string sampleName;
    int stock = 0;                 // 물리적 재고 (Sample::GetStock() 그대로)
    int pendingOrderQuantity = 0;   // Producing 상태 주문의 quantity 합 (3.4절 확정 정의)
    StockStatus status = StockStatus::Sufficient;
};

struct MainMenuSummary {
    int registeredSampleCount = 0;
    int totalStock = 0;
    int totalOrderCount = 0;        // Rejected 제외 (TODO: 확인 필요, 3.4절)
    int productionQueueCount = 0;   // productionJobRepository.FindAll().size()
};

}  // namespace Model
```

```cpp
// Model/MonitoringService.h (신설)
#pragma once

#include <string>
#include <vector>

#include "Model/MonitoringTypes.h"
#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/ProductionJobRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

// Read-only aggregation over Sample/Order/ProductionJob repositories for the
// monitoring screen and main-menu summary (PRD 7.5 / CLAUDE.md). Never
// mutates state and never calls CalculateAvailableStock() — that function is
// Phase 3's approval-time "available stock" concept, which monitoring must
// not surface (PRD 7.4 example 2: monitoring always shows physical stock).
class MonitoringService {
public:
    MonitoringService(SampleRepository& sampleRepository, OrderRepository& orderRepository,
                       ProductionJobRepository& productionJobRepository);

    OrderStatusSummary GetOrderStatusSummary() const;
    std::vector<SampleStockView> GetSampleStockViews() const;
    MainMenuSummary GetMainMenuSummary() const;

private:
    StockStatus JudgeStockStatus(int stock, int pendingOrderQuantity) const;
    int PendingOrderQuantityFor(const std::string& sampleId, const std::vector<Order>& allOrders) const;

    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
    ProductionJobRepository& productionJobRepository_;
};

}  // namespace Model
```

```cpp
// View/MonitoringView.h (신설)
#pragma once

#include <vector>

#include "Model/MonitoringTypes.h"

namespace View {

// Console output only — no domain logic, no calls into Model. Methods are
// virtual (with a virtual destructor) so GMock-based MonitoringController
// tests can mock this class, matching ReleaseView/OrderApprovalView's
// convention.
class MonitoringView {
public:
    virtual ~MonitoringView() = default;

    virtual void ShowOrderStatusSummary(const Model::OrderStatusSummary& summary) const;
    virtual void ShowSampleStockStatus(const std::vector<Model::SampleStockView>& views) const;
};

}  // namespace View
```

```cpp
// Controller/MonitoringController.h (신설)
#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/MonitoringService.h"
#include "View/MonitoringView.h"

namespace Controller {

// PRD 7.5 monitoring screen: pure read-and-display, no state transitions.
class MonitoringController : public ISubMenuController {
public:
    MonitoringController(Model::MonitoringService& service, View::MonitoringView& view);

    void Run() override;

private:
    Model::MonitoringService& service_;
    View::MonitoringView& view_;
};

}  // namespace Controller
```

```cpp
// View/MainMenuView.h (기존 파일 확장 — 새 메서드만 추가, 기존 4개는 그대로)
#pragma once

#include "Model/MonitoringTypes.h"

namespace View {

class MainMenuView {
public:
    void ShowMainMenu() const;
    void ShowSummary(const Model::MainMenuSummary& summary) const;  // 신규
    void ShowNotImplemented() const;
    void ShowInvalidSelection() const;
    void ShowExitMessage() const;
};

}  // namespace View
```

```cpp
// Controller/MainMenuController.h (기존 파일 수정 — 생성자 시그니처 변경, 하위 호환성 없음)
MainMenuController(View::MainMenuView& view, Model::MonitoringService& monitoringService,
                    std::map<std::string, std::reference_wrapper<ISubMenuController>> subMenuControllers);
```

JSON 저장 스키마 변경 없음 — `data/samples.json`/`data/orders.json`/`data/production_jobs.json`
모두 이번 Phase에서 필드가 추가되거나 값이 바뀌지 않는다(순수 읽기).

---

## 5. 의존 관계

**이 Phase가 전제로 삼는 이전 Phase 산출물**

- Phase 0: `Model::Sample`/`Model::Order`/`Model::ProductionJob` 도메인 모델과
  `Model::JsonFileRepository<T>` 기반 `SampleRepository`/`OrderRepository`/
  `ProductionJobRepository`(`FindAll`/`FindById`/`Add`/`Update`/`Remove`/`Save`/`Load`).
- Phase 1: `SampleRepository::FindAll()` — 시료별 재고 현황 집계에 사용.
- Phase 2: `Order` 생성(`OrderService::PlaceOrder`) — `Reserved` 상태 주문 집계 대상.
- Phase 3: `Order.status`가 `Reserved`/`Rejected`/`Producing`/`Confirmed`로 정확히 전이돼 있어야
  하고, `ProductionJobRepository::FindAll()`이 생산 큐 잔여 작업만 반환하는 불변식(완료 시
  `Remove()`)이 유지돼야 한다. 이 Phase는 Phase 3의 "승인 시점 가용 재고" 계산
  (`CalculateAvailableStock`)에 절대 관여하지 않고, `Sample.stock`/`Order.status`만 읽는다.
- Phase 4: `Order.status == Release` 전이(`OrderReleaseService::Release`)가 정상 동작해야 상태별
  건수 집계에 `Release`가 정확히 반영되고, 출고 시 `Sample.stock`이 감소한 값이 "총 재고"에
  정확히 반영된다.

**이후 Phase가 이 Phase에 기대하는 산출물**

- Phase 6(더미 데이터 생성기): 더미 데이터 생성 후 메인 메뉴 요약/모니터링 화면에 즉시
  반영되어야 하므로(PLAN.md Phase 6 완료 기준), `MonitoringService`의 모든 조회 메서드는 매
  호출 시 Repository를 다시 읽어 계산해야 한다(캐시 금지).
- Phase 7(통합 마무리): 전체 시스템 테스트에서 모니터링 화면이 실제 주문/생산/재고 변화를
  정확히 반영하는지 검증하는 기준점이 된다.

---

## 6. 핵심 비즈니스 로직 주의사항

- **REJECTED 제외 원칙**: `OrderStatusSummary`에는 `Rejected`를 담을 필드 자체를 두지 않는다.
  집계 루프에서 `switch`/`if`로 명시적으로 건너뛰어 "필드는 있는데 항상 0으로만 채워지는" 형태로
  방치하지 않는다.
- **물리적 재고 vs 가용 재고 혼동 금지(CLAUDE.md 필수 반영 사항)**: 모니터링 화면(상태별 집계,
  시료별 재고 현황, 메인 메뉴 요약의 "총 재고")은 예외 없이 `Sample::GetStock()`(물리적 재고)만
  사용한다. `Model::CalculateAvailableStock`(Phase 3, 승인 계산 전용)은 `MonitoringService`가
  어디에서도 호출하지 않는다. PRD 7.4 예시2("모니터링 화면에는 물리적 재고 90개가 그대로
  표시되어야 함")를 그대로 만족해야 한다.
- **읽기 전용, 재계산 없음**: `MonitoringService`는 상태를 변경하지 않는다(주문 상태 전이, 재고
  증감은 Phase 2~4의 책임). 매 호출 시 Repository에서 최신 데이터를 다시 읽어 계산하며, 자체
  캐시를 두지 않는다.
- **판정 우선순위 고정**: `stock == 0`이면 `pendingOrderQuantity`와 무관하게 무조건 `Depleted`로
  판정한다(PRD 7.5 "고갈: 재고 수량이 0"을 최우선 규칙으로 채택).
- **생산 큐 대기 건수는 Repository 크기 그대로**: `ProductionJobRepository::FindAll().size()`가
  완료된 작업을 이미 제외한 값이므로, `Order.status == Producing`인 주문을 별도로 세거나 두
  값을 대조하는 로직을 추가하지 않는다(과설계 방지, 2절 참고). 다만 `code-review`/`tester` 단계
  에서 "정상 흐름이라면 두 값이 항상 같아야 한다"는 불변식 자체는 회귀 테스트로 한 번 확인해
  둘 가치가 있다(7.3절 테스트 10번).

---

## 7. 테스트 계획 (tester 에이전트 작성 대상)

> 실행 방식(Phase 0~4에서 확정): `Tests/*.cpp`에 GoogleTest/GMock 테스트를 추가하되, 이 저장소는
> 파일을 자동으로 글롭(glob)하지 않고 `SampleOrderSystem.vcxproj`의 Debug 전용 `<ItemGroup>`에
> **파일명을 명시적으로 나열**하는 방식이다(`vcxproj`의 190~198행, `Tests\OrderManagementSystemTests.cpp`
> 까지 개별 등록돼 있음). 새 테스트 파일(예: `Tests/MonitoringServiceTests.cpp`,
> `Tests/MonitoringControllerTests.cpp`)을 추가하면 **`vcxproj`에도 반드시 `<ClCompile
> Include="Tests\...">` 항목을 함께 추가**해야 빌드에 포함된다. `SampleOrderSystem.exe --test`로
> 실행하면 콘솔 앱 대신 `RUN_ALL_TESTS()`가 실행된다(Release 빌드는 테스트 소스 제외).

### 7.1 `MonitoringService::GetOrderStatusSummary` 단위 테스트

1. 주문이 하나도 없을 때 4개 필드 모두 0.
2. `Reserved`/`Confirmed`/`Producing`/`Release` 각각 여러 건이 섞여 있을 때 정확히 분류.
3. `Rejected` 주문이 다수 포함돼도 어떤 카운트에도 영향을 주지 않음(합계 검증: 전체 주문 수 -
   `Rejected` 수 == 4개 필드 합).

### 7.2 `MonitoringService::GetSampleStockViews` / `StockStatus` 판정 단위 테스트

4. `stock == 0` → 비교 대상 주문 수량과 무관하게 `Depleted`(경계값: `pendingOrderQuantity == 0`
   이어도 `Depleted`).
5. `stock > 0` 이고 `stock < pendingOrderQuantity` → `Shortage`.
6. `stock > 0` 이고 `stock == pendingOrderQuantity` → `Sufficient`(경계값, "충분"의 등호 포함
   여부 검증).
7. `stock > 0` 이고 비교 대상 주문이 아예 없음(`pendingOrderQuantity == 0`) → `Sufficient`.
8. `pendingOrderQuantity` 계산 시 `Reserved` 상태 주문은 합산에서 제외되는지(예: 동일 시료에
   `Reserved` 수량 100 + `Producing` 수량 10 → `pendingOrderQuantity == 10`이어야 함, 110이
   되면 안 됨).
9. `Confirmed`/`Release`/`Rejected` 상태 주문도 `pendingOrderQuantity` 합산에서 제외되는지.
10. 여러 시료가 섞여 있을 때 시료별로 독립적으로 판정되는지.

### 7.3 물리적 재고 vs 가용 재고 회귀 테스트 (PRD 7.4 예시2 기반, code-review 시 필수 확인)

11. Given: 시료 A, `stock = 90`, 수율 1.0. 주문1(수량 100, 부족분 10, `Producing`), 주문2(수량
    10, Phase 3 규칙상 가용 재고 0 취급, `Producing`).
    When: `GetSampleStockViews()` 호출.
    Then: 시료 A의 `SampleStockView.stock == 90`(`CalculateAvailableStock`이 계산했을 값이 아닌
    물리적 재고 그대로).
12. (생산 큐 대기 건수 불변식 회귀 테스트) 정상 흐름에서 `Order.status == Producing`인 주문
    개수와 `ProductionJobRepository::FindAll().size()`가 항상 같은지 확인 — 승인 시 큐 등록,
    생산 완료 시 큐 제거와 주문 상태 전환이 같은 시점에 함께 일어난다는 전제(2절/6절)가 실제로
    지켜지는지 증명한다.

### 7.4 `MonitoringService::GetMainMenuSummary` 단위 테스트

13. 등록 시료 수 = Repository의 시료 개수와 일치.
14. 총 재고 = 모든 시료의 `GetStock()` 합.
15. 전체 주문 수 = `Rejected`를 제외한 주문 개수.
16. 생산 라인 대기 건수 = `productionJobRepository.FindAll().size()`와 일치(완료된 작업은 이미
    제거되어 포함되지 않음을 확인).

### 7.5 Controller/View 계층 테스트(GMock 활용)

17. `MonitoringController::Run()` 호출 시 `MonitoringService::GetOrderStatusSummary`/
    `GetSampleStockViews`가 각 1회 호출되고, 반환값이 `MonitoringView`의 출력 메서드로 그대로
    전달되는지 Mock으로 검증(Controller에 집계 로직이 새어 들어가지 않았는지 확인 목적).
18. `MonitoringView` 출력 포맷 테스트는 표준출력 캡처 방식(`Tests/TestSupport/
    ConsoleRedirectGuard.h` 재사용)으로 상태별 4개 항목/여유·부족·고갈 라벨이 모두 출력되는지
    확인.
19. **`MainMenuController` 생성자 시그니처 변경에 따른 기존 테스트 갱신(필수)**:
    `Tests/ControllerTests.cpp`의 `HandleInputWithZeroRequestsExit`/
    `HandleInputDelegatesToTheRegisteredController`/
    `HandleInputWithUnregisteredKnownMenuNumberDoesNotDelegate` 3개 테스트가 새 생성자 시그니처
    (`Model::MonitoringService&` 추가)에 맞춰 수정돼야 컴파일된다. `Tests/TestSupport/
    TempFileFixture.h`(기존 `RepositoryTests.cpp`/`OrderManagementSystemTests.cpp`에서 이미
    사용 중인 방식)로 임시 JSON 경로의 `SampleRepository`/`OrderRepository`/
    `ProductionJobRepository`를 만들어 `MonitoringService`를 구성한 뒤 넘겨준다.
20. (신규) `MainMenuController::Run()`을 호출했을 때(또는 `Run()` 내부 로직을 단위 테스트하기
    쉽게 별도 private 헬퍼로 뺀 경우 그 헬퍼를) `View::MainMenuView::ShowSummary`가 매 루프
    반복마다 최신 `MainMenuSummary`로 호출되는지 확인. `MainMenuView`가 non-virtual이라 GMock
    목이 불가능하므로, 표준출력 캡처 방식으로 요약 수치가 출력에 포함되는지 검증하거나, 필요
    시 `MonitoringService` 자체를 실제 임시 Repository로 구성해 값이 맞는지 확인하는 통합
    테스트로 대체한다(3.2절에서 확정한 non-virtual 유지 결정에 따른 제약).

### 7.6 통합(시스템) 테스트

21. 실제 JSON 파일 기반 Repository에 시료/주문/생산 큐 데이터를 적재한 뒤 애플리케이션을 구동해
    모니터링 메뉴(`4`) 진입 → 화면 출력값이 파일 데이터와 일치하는지 end-to-end로 확인.
22. 주문 승인/거절/생산완료/출고 등 상태 변화를 일으킨 뒤 모니터링 화면을 재조회해 값이 즉시
    갱신되는지 확인(캐시 없이 매번 재계산됨을 증명).
23. `main.cpp`에 `MainMenuOption::Monitoring`("4") 라우팅이 실제로 연결되어 있는지(메뉴에서 "4"
    입력 시 더 이상 "아직 구현되지 않은 기능입니다"가 뜨지 않는지) 확인.

---

## 8. 완료 조건 (Definition of Done)

- [ ] `Model::MonitoringService`, `Controller::MonitoringController`, `View::MonitoringView`,
  관련 값 객체(`OrderStatusSummary`, `SampleStockView`, `MainMenuSummary`, `StockStatus`)가
  구현되고 MVC 계층 분리 원칙을 어기지 않는다(Model에 집계 로직, Controller는 흐름 제어만,
  View는 출력만).
- [ ] `MonitoringService`가 `Model::CalculateAvailableStock`을 어디에서도 호출하지 않음이
  코드 리뷰로 확인된다.
- [ ] `Controller::MainMenuController` 생성자에 `Model::MonitoringService&`가 추가되고, `Run()`
  루프가 매 반복 `View::MainMenuView::ShowSummary`로 실제 Repository 데이터를 반영한 요약을
  출력한다.
- [ ] `Tests/ControllerTests.cpp`의 기존 3개 테스트가 새 생성자 시그니처에 맞춰 갱신되어 여전히
  통과한다(7.5절 19번).
- [ ] `main.cpp`에 `MainMenuOption::Monitoring`("4") 라우팅이 실제로 연결된다.
- [ ] 모니터링 메뉴에서 상태별 주문 건수(4개 상태, `REJECTED` 제외)와 시료별 재고 현황(여유/
  부족/고갈)이 정확히 출력된다.
- [ ] 모니터링 화면이 물리적 `stock`만 사용하고 Phase 3의 `CalculateAvailableStock`(가용 재고)을
  절대 참조하지 않음이 테스트로 증명된다(7.3절 회귀 테스트 11번 통과).
- [ ] 생산 라인 대기 건수가 `ProductionJobRepository::FindAll().size()`로 정확히 계산되고, 완료된
  작업이 제외됨이 테스트로 확인된다(7.4절 16번).
- [ ] 새로 추가한 테스트 파일이 `SampleOrderSystem.vcxproj`의 Debug `<ItemGroup>`에 등록돼 빌드에
  포함된다.
- [ ] 7절에 정의된 단위/통합 테스트가 `tester` 에이전트에 의해 작성되고 전부 통과한다.
- [ ] `code-review` 에이전트 검토에서 Clean Code/SOLID/함수 라인 수/디자인 패턴 사용에 대한 주요
  지적 사항이 없거나, 있었다면 반영 완료됐다.
- [ ] `supervisor`가 본 문서(`docs/Phase5.md`) 및 CLAUDE.md/PRD.md와의 부합 여부를 확인해 이상
  없음을 보고한다.
- [ ] 9절의 남은 `TODO: 확인 필요` 항목(전체 주문 수 REJECTED 포함 여부)에 대해 사용자 확답을
  받아 문서에 반영했다(`pendingOrderQuantity`/`productionQueueCount` 산정 범위는 확정 완료).

---

## 9. 확인이 필요한 지점 (TODO 요약)

1. **메인 메뉴 "전체 주문 수"에 `REJECTED` 포함 여부** — 모니터링 집계(PRD 7.5)의 "REJECTED
   제외" 원칙을 메인 메뉴 요약에도 동일 적용하는 것으로 기본값을 채택했으나, PRD 7.1에는 명시적
   언급이 없어 확인이 필요하다(이전 초안과 동일한 미해결 사항, 이번 코드 재검증으로도 해소되지
   않음).
2. **`MainMenuView`를 non-virtual로 유지하는 것이 이번 Phase 이후에도 계속 적절한지** — 지금은
   `MainMenuController`가 `ShowSummary` 호출 여부를 GMock으로 직접 검증할 방법이 없어 통합/출력
   캡처 테스트로 대체해야 한다(7.5절 20번). 향후 `MainMenuView`도 다른 View들처럼 virtual +
   가상 소멸자로 통일할지는 `code-review` 단계에서 재논의가 필요하다(이번 Phase 범위에서는
   기존 컨벤션을 그대로 존중하는 쪽으로 결정).

> `pendingOrderQuantity`(재고 판정용 "주문 대비" 비교 수량)와 `productionQueueCount`(생산 라인
> 대기 건수) 산정 범위는 모두 확정됐다(3.4절 참고).
