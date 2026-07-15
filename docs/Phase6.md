# Phase 6 — 더미 데이터 생성기

> 이 문서는 Phase 0~4의 실제 구현(코드 확인 완료, 브랜치 `docs/phase6-plan-sync`가 가리키는
> `origin/master` 기준 — `Phase4: 주문 출고 처리 구현` 머지 커밋까지 반영된 상태)을 전제로
> 삼는다. 이전 초안(`docs/Phase6.md`, 커밋 `352c765 Add remaining Phase docs and reconcile
> cross-phase inconsistencies`)은 Phase 0~4 코드가 아직 없던 시점에 PLAN.md 서술만으로 작성된
> 추정 문서였다. 본 개정판은 실제 `Model/Sample.h`, `Model/Order.h`, `Model/OrderStatus.h`,
> `Model/OrderService.h`, `Model/JsonFileRepository.h`, `Controller/SampleController.cpp` 등을
> 직접 읽고 다음 사실을 확정해 반영한다.
>
> 핵심 확인 사항 요약:
> - `Model::Sample`은 `TryCreate(id, name, avgProductionTime, yieldRate, stock = 0)`이라는
>   self-validating 팩토리를 이미 제공하며, **`stock` 파라미터를 직접 받을 수 있다**(기본값만
>   0일 뿐, 호출자가 원하는 값을 넘길 수 있음). 다만 `Controller::SampleController::HandleRegister`는
>   이 파라미터를 쓰지 않고 항상 기본값(0)으로만 호출한다 — 이는 "콘솔 등록 메뉴에는 초기 재고
>   입력 UI가 없다"는 Phase 1의 UX 결정일 뿐, `TryCreate` 자체의 제약은 아니다. 따라서 더미
>   생성기는 `SampleController`를 거치지 않고 `Model::Sample::TryCreate`를 직접 호출해 임의의
>   초기 재고를 가진 더미 시료를 만들 수 있다.
> - `Sample`에는 자동 채번 기능이 없다(`IsValidId`는 공백 여부만 검사). ID 유일성은
>   `Controller::SampleController::HandleRegister`가 `repository_.FindById(id).has_value()`로
>   직접 검사하는 방식이며, 더미 생성기도 동일한 패턴(자체적으로 후보 ID를 만들고
>   `SampleRepository::FindById`로 충돌 검사)을 따라야 한다.
> - `Order`는 `Order::CreateReserved(...)`라는 self-validating 팩토리를 가지며, 상태는 항상
>   `OrderStatus::Reserved`로 고정 생성된다. 그리고 이 팩토리를 포함해 "등록된 시료만 주문
>   가능" 검증, 채번(`OrderIdGenerator`, `ORD-YYYYMMDD-NNNN`), Repository 저장까지를 한 번에
>   수행하는 **완성된 유스케이스 서비스 `Model::OrderService::PlaceOrder(sampleId,
>   customerName, quantity)`**가 이미 존재한다(`Model/OrderService.h/.cpp`). 더미 주문 생성은
>   이 서비스를 그대로 호출하는 것으로 충분하며, `OrderIdGenerator`를 별도로 인스턴스화하거나
>   `Order::CreateReserved`를 직접 호출할 필요가 없다.
> - `OrderStatus` enum은 `{Reserved, Rejected, Producing, Confirmed, Release}` (PascalCase
>   C++ enum, JSON 직렬화 문자열은 `ToString`/`OrderStatusFromString`을 통해
>   `"RESERVED"`/`"REJECTED"`/`"PRODUCING"`/`"CONFIRMED"`/`"RELEASE"`로 왕복 변환된다).
> - Repository 계층(`Model::IRepository<TEntity, TId>`, `Model::JsonFileRepository<TEntity>`)의
>   실제 메서드는 `FindAll() const`, `FindById(const TId&) const`, `Add(const TEntity&)`,
>   `Update(const TEntity&)`, `Remove(const TId&)`, `Save()`, `Load()` (모두 PascalCase). `Add`/
>   `Update`/`Remove`는 내부에서 즉시 `Save()`를 호출해 파일에 flush한다 — 더미 생성기가 별도로
>   `Save()`를 호출할 필요가 없다.
> - Phase 5(모니터링)는 이 브랜치에는 아직 머지되어 있지 않지만(별도 브랜치에서 구현 완료,
>   병합 예정), `docs/DATA_SCHEMA.md`(이미 이 브랜치에 존재, Phase 1 커밋에서 추가됨)에 Phase 5의
>   확정된 집계 규칙이 요약돼 있다. 본 문서는 그 내용을 근거로 Phase 5와의 연동 지점을
>   기술한다(4.4절 참고). 실제 머지 후 클래스/메서드명이 `docs/DATA_SCHEMA.md`의 서술과 다르면
>   그 시점에 본 문서 4.4절을 갱신한다.

## 1. 목표 요약 (PLAN.md 인용)

> **목표**: CLAUDE.md 핵심 구현 요소 — 테스트/개발 편의를 위한 더미 데이터.
>
> - Sample/Order 더미 데이터를 생성해 JSON(DB)에 추가하는 기능
> - 기존 Repository 구조를 그대로 재사용해 생성된 데이터가 앱에서 바로 조회 가능하게 함
>
> **완료 기준**: 더미 데이터 생성 후 메인 메뉴 요약/모니터링 화면에 반영된다.

관련 CLAUDE.md 조항: "더미 데이터 생성기: 테스트/개발 편의를 위해 Sample/Order 더미 데이터를
생성해 DB(JSON)에 추가하는 기능을 둔다." / "위 세 항목(MVC 스켈레톤, 영속성, 모니터링, 더미
데이터)은 선행 PoC(별도 Repository)에서 검증된 구조/기법을 그대로 활용해 구현한다." — 다만 실제
PoC 코드(`poc_dummydatagenerator`)는 이 작업 환경에서 참조할 수 없으므로, 본 문서는 그 자리에
**이 프로젝트에 이미 확립된 코드 컨벤션**(self-validating 팩토리 재사용, Model 서비스 계층
패턴, `ISubMenuController` 기반 Controller, `virtual ... const` View)을 그대로 따르는 것으로
대체한다.

관련 PRD 조항: 9.4 "Dummy 데이터 생성 Tool"(선행 PoC 대상 항목을 실제 제품 코드에 통합), 10장
(메인 메뉴에 없는 개발자 편의 메뉴는 자유롭게 결정 가능).

---

## 2. 전제 (Phase 0~5 실제/예정 산출물)

### 2.1 Phase 0~4 (이 브랜치에 실제로 존재 — 코드 확인 완료)

- **`Model::Sample`** (`Model/Sample.h`)
  - 생성자: `Sample(std::string id, std::string name, double avgProductionTime, double yieldRate,
    int stock)` — 검증 없이 그대로 저장(내부/역직렬화용).
  - `static std::optional<Sample> TryCreate(std::string id, std::string name, double
    avgProductionTime, double yieldRate, int stock = 0)` — `IsValidId`(공백 아님)/`IsValidName`
    (공백 아님)/`IsValidAvgProductionTime`(`> 0`)/`IsValidYieldRate`(`0.0 < x <= 1.0`)를 모두
    통과해야 값을 반환하고, 실패 시 `std::nullopt`(예외 없음).
  - `GetId()`/`GetName()`/`GetAvgProductionTime()`/`GetYieldRate()`/`GetStock()` 접근자만 존재.
    자동 채번 기능 없음.
- **`Model::SampleRepository : JsonFileRepository<Sample>`** (`Model/SampleRepository.h`) —
  `FindAll()`, `FindById(id)`, `Add(sample)`, `Update`, `Remove`, `Save`, `Load`
  (기본 경로 `data/samples.json`), 그리고 `SearchByName(keyword)`(Phase 1 전용, 더미 생성기는
  사용하지 않음).
- **`Model::Order`** (`Model/Order.h`) / **`Model::OrderStatus`** (`Model/OrderStatus.h`)
  - `enum class OrderStatus { Reserved, Rejected, Producing, Confirmed, Release }`.
  - `static std::optional<Order> CreateReserved(std::string orderId, std::string sampleId,
    std::string customerName, int quantity, std::string createdAt)` — `IsValidQuantity`(`>= 1`)/
    `IsValidCustomerName`(공백 아님)을 통과해야 하며 `status_`는 항상 `Reserved`로 고정.
  - `GetOrderId()`/`GetSampleId()`/`GetCustomerName()`/`GetQuantity()`/`GetStatus()`/
    `GetCreatedAt()`/`GetUpdatedAt()` 접근자.
- **`Model::OrderRepository : JsonFileRepository<Order>`** (`Model/OrderRepository.h`) —
  Sample과 동일한 CRUD API(기본 경로 `data/orders.json`).
- **`Model::OrderIdGenerator`** (`Model/OrderIdGenerator.h`) — `GenerateNextId(const
  std::vector<Order>& existingOrders, const std::string& today) const`로 `ORD-YYYYMMDD-NNNN`
  형식(당일 최대 순번 + 1, 4자리 zero-padding) 채번. 상태를 갖지 않는 값 객체이므로 매번 새로
  만들어 호출해도 무방하다.
- **`Model::OrderService`** (`Model/OrderService.h/.cpp`) — **더미 주문 생성이 그대로 재사용할
  완성된 유스케이스**:
  ```cpp
  struct PlaceOrderResult {
      bool success = false;
      std::optional<Order> order;   // success == true일 때만 유효
      std::string failureReason;    // success == false일 때만 유효
  };
  class OrderService {
  public:
      OrderService(SampleRepository& sampleRepository, OrderRepository& orderRepository);
      PlaceOrderResult PlaceOrder(const std::string& sampleId, const std::string& customerName,
                                   int quantity);
  };
  ```
  `PlaceOrder` 내부에서 시료 존재 검증 → 수량/고객명 검증 → `OrderIdGenerator`로 채번 →
  `Order::CreateReserved` → `orderRepository_.Add(*order)`까지 모두 수행한다(`Sample.stock`은
  건드리지 않음). 더미 생성기는 이 메서드 하나만 호출하면 되고, 채번기/팩토리를 직접 다루지
  않는다.
- **`Common::CurrentDateYyyymmdd()`/`Common::CurrentTimestampIso8601()`** (`Common/Clock.h`) —
  `OrderService`가 내부적으로 이미 사용 중이므로 더미 생성기가 직접 호출할 필요는 없다(참고
  정보로만 기재).
- **`Model::IRepository<TEntity, TId>`/`Model::JsonFileRepository<TEntity>`** — `Add`/`Update`/
  `Remove` 호출 시 즉시 `Save()`가 내부에서 실행되어 파일에 flush됨(`docs/DATA_SCHEMA.md` 1절).
  더미 생성기가 별도의 파일 I/O나 `Save()` 명시적 호출을 만들 필요가 없다.
- **`Controller::ISubMenuController`**(`virtual void Run() = 0;`)와
  **`Controller::MainMenuController`**(`std::map<std::string,
  std::reference_wrapper<ISubMenuController>>` 기반 라우팅, `MainMenuOption` enum + `std::string
  ToMenuOptionKey(MainMenuOption)`) — 새 서브메뉴 추가 시 `MainMenuController` 자체를 수정할
  필요 없이 `main.cpp`의 맵에 항목만 추가하면 되는 구조가 이미 확립돼 있다.
  - 현재(`Model/MainMenuController.h`) `enum class MainMenuOption { Exit = 0, SampleManagement =
    1, OrderPlacement = 2, OrderApproval = 3, Monitoring = 4, ProductionLine = 5, Release = 6 }`.
    PRD 7.1의 정식 메뉴 6개(1~6)가 이미 모두 배정되어 있으므로, 더미 데이터 생성 메뉴는 새 값
    `DummyDataGeneration = 7`을 추가해야 한다(1~6과 절대 충돌하지 않도록).
  - `main.cpp`은 현재 `Monitoring`("4") 키를 아직 맵에 등록하지 않는다(Phase 5 미병합 상태와
    일치). Phase 6 구현 시점에는 Phase 5가 먼저 병합되어 `"4"` 항목이 채워져 있을 것으로
    전제한다.
- **View 컨벤션** (`View/SampleView.h`, `View/OrderView.h` 등): 모든 메서드가 `virtual ...
  const`(콘솔 I/O만 담당, Model 상태를 직접 조작하지 않음), 가상 소멸자 보유. GMock으로
  Controller를 목 View 기반 단위 테스트하기 위한 장치.
- **`Common::ReadInt()`** (`Common/ConsoleInput.h`) — `std::cin`에서 정수 하나를 읽고 파싱
  실패 시 0을 반환(실패 상태 clear). 더미 데이터 생성 개수/시드 입력에 재사용 가능.

### 2.2 Phase 5 (이 브랜치에는 미병합 — `docs/DATA_SCHEMA.md` 기준 확정 사항만 인용)

Phase 5는 별도 브랜치에서 구현이 끝난 상태이며 곧 병합될 예정이다. 이 브랜치에는 아직
`Model/MonitoringService.h` 등 실 코드가 없으므로, 본 절은 **이미 이 브랜치에 커밋되어 있는
`docs/DATA_SCHEMA.md`(Phase 1에서 추가, "Phase5.md 3.4절/5절 확정"이라고 명시)** 의 서술을
근거로 삼는다.

- 모니터링/메인 메뉴 요약은 **캐시를 두지 않고 Repository의 `FindAll()`을 매번 다시 읽어
  계산**하는 방식으로 확정되어 있다(`docs/DATA_SCHEMA.md` 3.4절, "읽기 전용, 재계산 없음").
  → 더미 데이터 생성기가 별도의 "모니터링 갱신 트리거"나 옵저버 통지 코드를 추가할 필요가
  전혀 없다. `SampleRepository`/`OrderRepository`에 `Add`로 데이터를 넣기만 하면, 다음번
  모니터링 화면 진입 시 그 즉시 반영된다(모니터링 쪽이 스스로 다시 읽기 때문).
- 모니터링 집계에서 `REJECTED`는 제외되고, 재고 여유/부족/고갈 판정은 **물리적 `stock`**만
  사용하며 `pendingOrderQuantity`는 **`PRODUCING` 상태 주문의 `quantity` 합만** 사용한다
  (`RESERVED`/`CONFIRMED`/`RELEASE`/`REJECTED` 제외). 더미 주문은 항상 `RESERVED`로만 생성되므로
  (3장 참고), 더미 데이터 생성 직후에는 재고 판정에 전혀 영향을 주지 않고 오직
  "여유/부족/고갈" 판정의 분모가 되는 `stock` 자체(더미 시료의 초기 재고)와 "전체 주문 수"만
  늘어난다.
- 메인 메뉴 요약(`MainMenuSummary`로 추정되는 값 객체)은 등록 시료 수/총 재고/전체 주문 수
  (`REJECTED` 제외)/생산 라인 대기 건수(`PRODUCING` 주문에 연결된 `ProductionJob` 수)를 담는다.
  더미 시료·더미 주문(`RESERVED`)이 추가되면 "등록 시료 수"/"총 재고"/"전체 주문 수"는 즉시
  증가하고, "생산 라인 대기 건수"는 더미 주문이 `RESERVED`로만 생성되므로 **변하지 않는다**
  (`PRODUCING` 전이는 승인 절차를 거쳐야만 발생하며 이는 이 Phase의 범위 밖).

> TODO: 확인 필요 — Phase 5가 실제로 병합된 뒤 `MonitoringService`/`MainMenuSummary` 등의
> 정확한 클래스/필드명이 `docs/DATA_SCHEMA.md`의 서술과 다르게 확정되면(예: 다른 이름으로
> 리네임), 본 문서의 2.2절/4.4절 표현을 그 실제 이름으로 교체한다. 핵심 결론("더미 생성기는
> Repository에 `Add`만 하면 되고, 모니터링 쪽이 캐시 없이 재계산한다")은 이름이 바뀌어도
> 변하지 않는다.

---

## 3. 작업 항목 체크리스트

### 3.1 Model 계층

- [ ] `Model/DummyDataOptions.h` 신설 — 4장의 `DummyDataOptions` 구조체 정의(값 객체, 로직 없음).
- [ ] `Model/DummyDataGenerator.h/.cpp` 신설
  - 생성자: `DummyDataGenerator(SampleRepository& sampleRepository, OrderRepository&
    orderRepository)` — `OrderService`/`OrderApprovalService`/`OrderReleaseService`와 동일한
    "Repository 참조 주입" 컨벤션.
  - 공개 메서드: `DummyDataResult Generate(const DummyDataOptions& options)`.
  - 내부에서 **두 번째 도메인 로직 구현체가 되지 않도록** 다음을 반드시 지킨다.
    - 더미 시료 생성: 후보 ID를 스스로 만들고(3.1.1절) `Model::Sample::TryCreate(...)`를 직접
      호출해 검증을 통과한 값만 `SampleRepository::Add`로 저장. `TryCreate`가 실패
      (`std::nullopt`)하면 그 시료는 건너뛰고 `failureReasons`에 사유를 남긴다(생성기 스스로
      옵션 범위를 벌려서 잘못된 값을 만들지 않는 한 정상적으로는 실패하지 않아야 하지만, 방어
      차원의 처리).
    - 더미 주문 생성: `Model::OrderService::PlaceOrder(sampleId, customerName, quantity)`를
      그대로 호출. `PlaceOrderResult::success == false`이면 `failureReasons`에
      `result.failureReason`을 그대로 누적하고 다음 주문 생성으로 넘어간다(예외로 전체 생성이
      중단되지 않게 함).
  - `DummyDataGenerator` 자신의 책임은 "무작위/규칙적인 입력값을 만들어 기존 생성 API
    (`Sample::TryCreate`, `OrderService::PlaceOrder`)에 넘기는 것"으로 한정한다. 수율/생산시간
    범위 검증, 채번, "등록된 시료만 주문 가능" 검증 등 도메인 규칙 자체를 재구현하지 않는다.
- [ ] **(3.1.1) 더미 시료 ID/이름/수치 생성 규칙**
  - ID: `S-{n}` (n은 1부터 시작하는 순번, 예: `S-001`) 형태로 후보를 순차 생성한다.
    1. 후보 `S-{n}`에 대해 `SampleRepository::FindById(candidateId)`를 호출해 존재 여부를
       확인한다(전용 "다음 ID" API를 새로 만들지 않고 기존 조회 API만 사용).
    2. 이미 존재하면 순번을 증가시켜 다음 후보로 넘어간다.
    3. 존재하지 않는 ID를 찾으면 그 값을 `Sample::TryCreate`의 `id` 인자로 사용한다.
    4. 무한 루프 방지를 위해 시도 횟수 상한(예: `options.sampleCount`의 10배, 최소 100)을 두고,
       상한 초과 시 해당 시료 생성을 실패로 처리하고 `failureReasons`에 사유를 남긴다.
  - 이름: `더미시료-{n}` (n은 위 ID 채번에 성공한 순서상의 일련번호, ID의 `{n}`과 반드시 같은
    값일 필요는 없다 — 이름은 표시용이므로 중복이 허용된다).
  - 평균 생산시간(`avgProductionTime`): `[options.minAvgProductionTime,
    options.maxAvgProductionTime]` 범위의 난수(`> 0` 이어야 하므로 하한 기본값도 0보다 크게
    설정, 4장 참고).
  - 수율(`yieldRate`): `[options.minYieldRate, options.maxYieldRate]` 범위의 난수(`0.0 <
    yieldRate <= 1.0` 범위를 벗어나지 않도록 옵션 자체의 기본값을 그 범위 안에 둔다).
  - 초기 재고(`stock`): `[options.minInitialStock, options.maxInitialStock]` 범위의 난수(0
    포함 — "고갈" 상태 모니터링 테스트가 가능하도록). `Sample::TryCreate`의 5번째 인자로 그대로
    전달한다(2절에서 확인했듯 `TryCreate`는 `stock`을 인자로 받으므로, Phase 1의 콘솔 등록
    메뉴가 이 인자를 안 쓴다는 사실과 무관하게 더미 생성기는 직접 지정 가능).
- [ ] **(3.1.2) 더미 주문 생성 규칙**
  - 대상 시료: `SampleRepository::FindAll()` 결과(방금 생성한 더미 시료 포함) 중에서 무작위로
    선택한다. 결과가 비어 있으면(등록된 시료가 하나도 없음) 주문 생성을 전부 실패 처리하고
    `failureReasons`에 사유("등록된 시료가 없어 더미 주문을 생성할 수 없습니다" 등)를 1건
    남긴다.
  - 고객명: 미리 정의한 후보 문자열 목록(`고객A`, `고객B`, ... 또는 `Customer-{n}`)에서 순환
    선택.
  - 수량: `[options.minOrderQuantity, options.maxOrderQuantity]` 범위의 난수(`>= 1`).
  - 상태: 별도로 지정하지 않는다 — `OrderService::PlaceOrder`가 항상 `Reserved`로 생성한다
    (Phase 2 규칙 그대로).
  - 더미 생성기가 `CONFIRMED`/`PRODUCING`/`RELEASE` 등 다른 상태의 주문을 직접 만들어 넣거나,
    Phase 3/4의 승인·거절·생산·출고 로직을 재실행해 상태를 전이시키는 기능은 이 Phase의 범위에
    **포함하지 않는다**. 다양한 상태의 더미 주문이 필요하면 담당자가 콘솔의 주문 승인(3)/생산
    라인(5)/출고 처리(6) 메뉴를 수동으로 이용해야 한다.
- [ ] 난수 시드(`options.seed`)를 받아 `std::mt19937`(또는 동등한 PRNG) 시드로 사용, 동일 시드
  로 재현 가능한 더미 데이터 생성 지원(테스트 용이성 목적). `seed == 0`이면 실행 시각 기반
  자동 시드(`std::random_device` 등)를 사용한다.

### 3.2 Controller 계층

- [ ] `Controller/DummyDataController.h/.cpp` 신설, `Controller::ISubMenuController` 구현
  (`void Run() override`).
  - 생성자: `DummyDataController(Model::DummyDataGenerator& generator, View::DummyDataView&
    view)`.
  - `Run()` 흐름:
    1. `view_.PromptOptions(options)` (또는 각 필드를 개별 프롬프트로 나눠 읽고 `Controller`가
       `DummyDataOptions`를 조립 — View는 입출력만, 조립은 Controller 책임) — 시료 개수/주문
       개수/시드를 입력받는다. 미입력(빈 입력 등)은 4장의 기본값을 사용한다.
    2. 개수 입력값 검증: 음수는 재입력 요청(`view_.ShowInvalidCount()` 후 재프롬프트 또는
       실패 처리 — 세부 UX는 구현 시 `SampleView::ReadRegistrationInput`류의 "재입력 루프"
       패턴을 참고), 0은 "해당 종류는 생성하지 않음"으로 허용한다.
    3. `Model::DummyDataResult result = generator_.Generate(options);` 호출.
    4. `view_.ShowResult(result)`로 생성된 시료 ID 목록/주문번호 목록(또는 개수 요약), 실패
       사유를 출력한다.
  - Controller 자체에는 난수 생성/도메인 검증 로직이 없어야 한다(Model 호출 결과를 그대로
    View에 넘김 — CLAUDE.md MVC 원칙).

### 3.3 View 계층

- [ ] `View/DummyDataView.h/.cpp` 신설, 기존 View 컨벤션(`virtual ... const`, 가상 소멸자,
  Model 값 객체만 인자로 받고 Repository/Service를 직접 참조하지 않음)을 따른다.
  - `virtual void PromptOptions(Model::DummyDataOptions& options) const;` (또는 개수/시드를
    개별 메서드로 나눠 `PromptSampleCount()`/`PromptOrderCount()`/`PromptSeed()`로 분리 — 구현
    시 `code-review`에서 함수 라인 수/SRP 관점으로 판단)
  - `virtual void ShowResult(const Model::DummyDataResult& result) const;` — 생성된 시료 ID
    목록, 생성된 주문번호 목록(또는 개수 요약), 실패 사유 목록을 출력.
  - `virtual void ShowInvalidCount() const;`
- [ ] 메인 메뉴에 "더미 데이터 생성" 항목 추가.
  - `View/MainMenuView.cpp`의 `ShowMainMenu()` 출력 목록에 `"7. 더미 데이터 생성 (테스트용)"`
    한 줄 추가(PRD 7.1 정식 메뉴 1~6과 구분되는 개발자 편의 기능임을 문구로 명시).
  - `Controller/MainMenuController.h`의 `MainMenuOption` enum에 `DummyDataGeneration = 7` 추가.

### 3.4 `main.cpp` 배선

- [ ] `Model::DummyDataGenerator dummyDataGenerator(sampleRepository, orderRepository);`,
  `View::DummyDataView dummyDataView;`, `Controller::DummyDataController
  dummyDataController(dummyDataGenerator, dummyDataView);` 생성.
- [ ] `MainMenuController`에 넘기는 맵에 `{ToMenuOptionKey(MainMenuOption::DummyDataGeneration),
  dummyDataController}` 항목 추가(Phase 4가 `Release`에 대해 한 것과 동일한 패턴).
- [ ] Phase 5가 이 시점까지 병합되어 있다면 `Monitoring`("4") 항목도 함께 맵에 등록돼 있어야
  한다(이 Phase의 책임은 아니지만, DoD의 "모니터링 화면에 반영" 확인을 위해 실제 실행
  경로에서 필요).

---

## 4. 인터페이스/데이터 스키마

이 Phase는 `Sample`/`Order`/`ProductionJob`의 스키마를 변경하지 않는다(PRD 8장/
`docs/DATA_SCHEMA.md` 그대로 사용). 새로 추가되는 것은 아래 옵션/생성기/Controller/View
인터페이스뿐이다.

### 4.1 `Model/DummyDataOptions.h` (신설)

```cpp
#pragma once

namespace Model {

struct DummyDataOptions {
    int sampleCount = 5;           // 생성할 더미 시료 개수
    int orderCount = 10;           // 생성할 더미 주문 개수
    unsigned int seed = 0;         // 0이면 실행 시각 기반 자동 시드 사용

    double minYieldRate = 0.5;     // 0.0 < minYieldRate <= maxYieldRate <= 1.0
    double maxYieldRate = 1.0;

    double minAvgProductionTime = 1.0;   // > 0 (Sample::IsValidAvgProductionTime 요건)
    double maxAvgProductionTime = 120.0;

    int minInitialStock = 0;       // >= 0
    int maxInitialStock = 100;

    int minOrderQuantity = 1;      // >= 1 (Order::IsValidQuantity 요건)
    int maxOrderQuantity = 50;
};

}  // namespace Model
```

### 4.2 `Model/DummyDataGenerator.h` (신설)

```cpp
#pragma once

#include <string>
#include <vector>

#include "Model/DummyDataOptions.h"
#include "Model/OrderRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

struct DummyDataResult {
    std::vector<std::string> createdSampleIds;
    std::vector<std::string> createdOrderIds;
    std::vector<std::string> failureReasons;
};

// Test/dev convenience use-case: generates dummy Sample/Order data through
// the existing self-validating factories (Sample::TryCreate,
// OrderService::PlaceOrder) so it can never bypass Phase 1/2's validation
// or duplicate their domain logic. Samples always get a self-picked "S-{n}"
// id (Sample has no auto id generator); Orders always come out RESERVED via
// OrderService (Order has one already, OrderIdGenerator included).
class DummyDataGenerator {
public:
    DummyDataGenerator(SampleRepository& sampleRepository, OrderRepository& orderRepository);

    DummyDataResult Generate(const DummyDataOptions& options);

private:
    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
};

}  // namespace Model
```

> `Model::OrderService`는 `DummyDataGenerator`의 생성자 인자로 받지 않고, `Generate()` 내부에서
> `OrderService(sampleRepository_, orderRepository_)`를 지역 변수로 만들어 호출한다(상태를
> 갖지 않는 얇은 유스케이스 객체이므로 매번 새로 만들어도 비용이 없다 — `main.cpp`에 이미
> 존재하는 `orderService` 인스턴스를 별도로 주입받게 하면 생성자 인자가 하나 더 늘고 Phase 6가
> Phase 2의 배선(main.cpp에서 `orderService`가 이미 `orderController`용으로 만들어짐)에
> 종속되므로, 더 단순한 쪽을 택한다). TODO: 확인 필요 — `code-review`에서 "지역 생성 vs 생성자
> 주입" 중 어느 쪽이 이 코드베이스의 기존 관례(예: `OrderApprovalController`가
> `ProductionLine`을 참조로 주입받는 방식)에 더 부합하는지 재검토한다.

### 4.3 `Controller/DummyDataController.h` / `View/DummyDataView.h` (신설)

```cpp
// Controller/DummyDataController.h
#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/DummyDataGenerator.h"
#include "View/DummyDataView.h"

namespace Controller {

class DummyDataController : public ISubMenuController {
public:
    DummyDataController(Model::DummyDataGenerator& generator, View::DummyDataView& view);

    void Run() override;

private:
    Model::DummyDataGenerator& generator_;
    View::DummyDataView& view_;
};

}  // namespace Controller
```

```cpp
// View/DummyDataView.h
#pragma once

#include "Model/DummyDataGenerator.h"
#include "Model/DummyDataOptions.h"

namespace View {

class DummyDataView {
public:
    virtual ~DummyDataView() = default;

    virtual void PromptOptions(Model::DummyDataOptions& options) const;
    virtual void ShowResult(const Model::DummyDataResult& result) const;
    virtual void ShowInvalidCount() const;
};

}  // namespace View
```

### 4.4 Phase 5(모니터링, 미병합)와의 연동 지점

- 이 Phase는 `docs/DATA_SCHEMA.md`가 서술하는 Phase 5의 확정 사항(모니터링/메인 메뉴 요약이
  캐시 없이 Repository를 매번 다시 읽어 계산)에 의존하되, `MonitoringService` 등 Phase 5의 실제
  클래스를 **호출하지 않는다**(2.2절 참고). `DummyDataGenerator`/`DummyDataController`는 Phase
  5의 어떤 타입도 `#include`하지 않는다 — 두 Phase 간 결합은 오직 "동일한
  `SampleRepository`/`OrderRepository` 인스턴스를 공유한다"는 사실 하나뿐이다.
- 따라서 "더미 데이터 생성 후 모니터링 화면에 즉시 반영"이라는 완료 기준은 이 Phase가 별도
  코드를 작성해서가 아니라, Phase 5가 이미 그렇게(캐시 없이) 설계돼 있기 때문에 자동으로
  성립한다. 이 Phase의 책임은 오직 "Repository에 실제로 데이터가 들어가는 것"까지다.

---

## 5. 의존 관계

- **선행 의존**
  - Phase 0(`docs/Phase0.md`): `Sample`/`Order`/`OrderStatus` 도메인 모델, `IRepository`/
    `JsonFileRepository<T>` 기반 CRUD, 파일 경로(`data/samples.json`, `data/orders.json`).
  - Phase 1(`docs/Phase1.md`): `Sample::TryCreate`의 검증 규칙(수율 `0.0 < x <= 1.0`,
    평균 생산시간 `> 0`), 시료 ID가 자동 채번 없이 "직접 입력 + 중복 검사" 방식이라는 확정
    사실.
  - Phase 2(`docs/Phase2.md`): `OrderService::PlaceOrder`(검증 + 채번 + `RESERVED` 생성 +
    저장을 한 번에 수행하는 유스케이스), `OrderIdGenerator`의 `ORD-YYYYMMDD-NNNN` 채번 규칙.
  - Phase 5(모니터링, 이 브랜치에는 미병합이지만 별도 구현 완료 예정): "캐시 없이 매번
    재계산"이라는 설계가 전제되어야 이 Phase의 완료 기준(모니터링 화면에 즉시 반영)이
    자동으로 성립한다. `docs/DATA_SCHEMA.md`의 관련 서술을 근거로 삼는다.
- **후속 기대**
  - Phase 7(통합 마무리): 전체 시스템 테스트에서 더미 데이터 생성기로 대량의 Sample/Order를
    만들어 놓고 승인/생산/출고/모니터링 흐름을 검증하는 시나리오에 활용한다. 이 Phase의
    산출물(더미 시료/주문)은 프로그램 재시작 후에도 JSON에 남아 있어야 한다(Phase 0 영속성
    요구사항 상속, `Add`가 즉시 `Save()`하므로 별도 구현 없이 자동 만족).

---

## 6. 핵심 비즈니스 로직 주의사항

이 Phase 자체는 PRD의 생산량 계산/상태 전이 같은 새로운 비즈니스 규칙을 추가하지 않는다. 다만
기존 규칙을 우회하지 않도록 다음을 반드시 지킨다.

- **시료 검증 규칙 재사용(확정)**: 더미 시료도 `Sample::TryCreate`의 검증(수율 `0.0 < x <=
  1.0`, 평균 생산시간 `> 0`, ID/이름 공백 아님)을 그대로 통과해야 한다. 옵션 자체가 잘못
  구성되어(예: `minYieldRate > maxYieldRate`, `minAvgProductionTime <= 0`) 검증에 걸리는
  값이 나올 경우, `Generate()`는 예외를 던지지 않고 `failureReasons`에 사유를 남기며 계속
  진행한다(`TryCreate`가 `std::nullopt`를 반환하는 것과 동일한 계약).
- **"등록된 시료만 주문 가능" 규칙 재사용(확정)**: `OrderService::PlaceOrder`가 이미 이 검증을
  수행하므로 더미 생성기가 별도로 재검증할 필요는 없다. 다만 대상 시료를 무작위로 고르는
  시점 자체가 `SampleRepository::FindAll()`이 반환하는, 실제로 존재하는 시료 목록이어야 한다
  (존재하지 않는 ID를 임의로 지어내 `PlaceOrder`에 넘기면 항상 실패로 끝나므로 무의미).
- **주문 상태는 항상 `Reserved`로만 생성(확정)**: `OrderService::PlaceOrder`가 내부적으로
  `Order::CreateReserved`만 호출하므로 이 규칙은 구조적으로 보장된다. 더미 생성기가
  `Order`/`OrderRepository`를 직접 조작해 다른 상태를 만들어 넣는 코드를 작성하지 않는다.
- **물리적 `stock` vs 승인 시점 가용 재고 혼동 금지 (PRD 7.4, `docs/DATA_SCHEMA.md` 3.1절)**:
  더미 시료의 `stock`은 물리적 재고이며, Phase 3의 "승인 시점 가용 재고"
  (`CalculateAvailableStock`, `Model/AvailableStockCalculator.h`) 계산과는 무관하다. 더미
  생성기는 그 계산 로직을 호출하지도, 그 결과에 영향을 주는 어떤 상태(`CONFIRMED`/
  `PRODUCING`)도 만들지 않는다(더미 주문은 항상 `RESERVED`이므로 애초에
  `CalculateAvailableStock`의 합산 대상에도 포함되지 않는다).
- **시료 ID 채번 방식(확정)**: `Sample`은 자동 채번 기능이 없으므로, 더미 생성기는 `S-{n}`
  형태의 후보 ID를 스스로 순차 생성하고 `SampleRepository::FindById`로 기존 시료와의 충돌
  여부를 직접 검사해 유일성을 확보한다(충돌 시 다음 후보로 이동). 이렇게 정한 ID만
  `Sample::TryCreate`에 전달한다.
- **주문 ID 채번 방식(확정)**: `Order`는 `OrderService::PlaceOrder`가 내부적으로
  `OrderIdGenerator`를 이미 호출하므로, 더미 생성기는 채번 로직을 전혀 직접 다루지 않는다.

---

## 7. 테스트 계획 (`tester` 에이전트 작성 대상)

> 실행 방식(Phase 0~4에서 확정): 별도 테스트 프로젝트 없이 `Tests/*.cpp`에 GoogleTest/GMock
> 테스트를 추가하면 `SampleOrderSystem.vcxproj`가 Debug 빌드에서만 조건부 컴파일한다.
> `SampleOrderSystem.exe --test`로 실행하면 콘솔 앱 대신 `RUN_ALL_TESTS()`가 실행된다. 새 테스트
> 파일은 `Tests/DummyDataGeneratorTests.cpp`(단위), `Tests/DummyDataControllerTests.cpp`
> (Controller/View GMock), 그리고 기존 `Tests/OrderManagementSystemTests.cpp`류의 시스템 테스트
> 파일에 케이스를 추가하는 형태를 따른다(기존 파일명 패턴과의 일관성).

### 7.1 `DummyDataGenerator::Generate` 단위 테스트

- 정상 케이스: `sampleCount=5, orderCount=10`으로 호출 시 `createdSampleIds.size()==5`,
  `createdOrderIds.size()==10`이고, 각 ID가 실제 `SampleRepository`/`OrderRepository`에서
  `FindById`로 조회 가능한지 확인.
- 생성된 시료의 `GetYieldRate()`가 항상 `[minYieldRate, maxYieldRate]` 범위 내인지 확인
  (경계값: `minYieldRate == maxYieldRate`로 고정값 생성되는 케이스 포함).
- 생성된 시료의 `GetAvgProductionTime()`/`GetStock()`이 지정 범위 내인지 확인(경계값:
  `minInitialStock == 0`으로 두어 실제로 `stock == 0`인 시료가 만들어지는지).
- 생성된 주문의 `GetStatus() == OrderStatus::Reserved`인지 전수 확인.
- 생성된 주문의 `GetSampleId()`가 (생성 시점 기준) `SampleRepository`에 존재하는 시료 ID 중
  하나인지 확인.
- 경계값: `sampleCount=0, orderCount=0` → 아무것도 생성되지 않고(`createdSampleIds`/
  `createdOrderIds` 모두 empty) `failureReasons`도 비어 있음(정상 종료, 에러 아님).
- 경계값: 등록된 시료가 하나도 없는 상태(`sampleCount=0`)에서 `orderCount>0` → 주문 생성이
  전부 실패 처리되고 `failureReasons`에 사유가 기록되며, 예외로 프로그램이 죽지 않음.
- 동일 시드로 두 번 `Generate` 호출(매번 Repository를 초기 상태로 리셋한 뒤) 시 생성되는
  시료 ID(`S-{n}` 순차 채번)가 완전히 동일하게 재현되는지 확인. 주문번호
  (`ORD-YYYYMMDD-NNNN`)는 실행 날짜에 좌우되므로 문자열 자체의 재현은 검증 대상에서 제외하고,
  수량/참조 시료 ID 등 "값의 재현"만 확인한다.
- 시료 ID 충돌 회피: `SampleRepository`에 이미 `S-001`~`S-003`이 존재하는 상태에서 `Generate`
  호출 시, 새로 생성되는 시료 ID가 기존 ID와 겹치지 않고 `S-004`부터 순차 채번되는지 확인
  (GMock으로 `SampleRepository::FindById`가 후보 ID마다 호출됨을 함께 검증). 경계값: 기존 ID가
  `S-001`, `S-003`처럼 듬성듬성 존재하는 경우에도 건너뛰고 정상적으로 유일한 ID를 찾는지 확인.
- 주문 ID 형식/유일성: 생성된 주문번호가 `ORD-YYYYMMDD-NNNN` 정규식에 맞고, 기존 주문번호와
  중복되지 않는지 확인.

### 7.2 `DummyDataController`/`DummyDataView` 통합(GMock) 테스트

- `view_.PromptOptions(...)`가 채운 옵션 그대로 `DummyDataGenerator::Generate`가 호출되는지
  (Mock `DummyDataGenerator` 서브클래스로 검증 — 다른 Service들과 동일하게 가상 메서드로
  선언되어 있어야 함, `code-review`에서 확인).
- `Generate` 결과가 그대로 `view_.ShowResult(result)`로 전달되는지(Controller에 별도 가공
  로직이 없는지 확인).
- 개수 입력값이 음수인 경우 `view_.ShowInvalidCount()`가 호출되고 `Generate`가 호출되지
  않는지.
- 개수 입력값이 0인 경우 정상적으로 `Generate`가 호출되는지("0은 생성 안 함"으로 허용).

### 7.3 시스템(통합) 테스트

- 실제 JSON 파일 기반 Repository로 더미 데이터 생성 → 프로세스 재시작을 흉내 낸
  `Load()` 재호출 → 생성된 시료/주문이 그대로 조회되는지 확인(Phase 0 영속성 요구사항
  재검증).
- (Phase 5 병합 후 실행 가능한 시나리오 — 병합 전까지는 Model 계층 값만으로 아래를 간접
  검증) 더미 데이터 생성 전/후 `SampleRepository::FindAll()`/`OrderRepository::FindAll()`
  집계값(등록 시료 수, 총 재고 합, 전체 주문 수)이 정확히 증가하는지 확인 — 이는 Phase 5의
  `MonitoringService`가 그대로 재사용할 동일한 원본 데이터이므로, 이 Phase에서는 Repository
  수준에서 값을 검증하고, Phase 5 병합 후에는 `MonitoringService`를 통한 end-to-end 검증을
  추가한다(TODO: Phase 5 병합 후 이 절을 갱신).
- 콘솔 메뉴 경로 E2E: 메인 메뉴 → "7" 입력 → 더미 데이터 생성 메뉴 진입 → 개수/시드 입력 →
  결과 출력까지 확인.

---

## 8. 완료 조건 (Definition of Done)

- [ ] `Model::DummyDataGenerator`, `Model::DummyDataOptions`, `Model::DummyDataResult`,
  `Controller::DummyDataController`, `View::DummyDataView`가 구현되고 빌드된다.
- [ ] 더미 데이터 생성 기능이 `SampleRepository::Add`/`OrderService::PlaceOrder`(내부적으로
  `OrderRepository::Add` 수행)만을 통해 데이터를 추가하며, 별도의 JSON 파일 직접 조작 코드가
  없다.
- [ ] 더미 시료가 `Sample::TryCreate`의 검증을 그대로 통과해 생성되며, ID는 더미 생성기가
  자체적으로 후보(`S-{n}`)를 만들고 `SampleRepository::FindById`로 충돌을 확인해 결정한다.
- [ ] 더미 주문이 `OrderService::PlaceOrder`를 그대로 통과해 생성되며, 항상 `Reserved` 상태로만
  생성된다(등록되지 않은 시료에 대한 더미 주문은 생성되지 않으며, 더미 생성기가 승인/생산/출고
  로직을 재실행해 다른 상태로 전이시키는 기능은 구현하지 않는다).
- [ ] 생성된 더미 데이터가 프로그램 재시작(`Load()` 재호출) 후에도 유지된다.
- [ ] `main.cpp`에 `MainMenuOption::DummyDataGeneration`("7") 라우팅이 실제로 연결된다.
- [ ] 7장의 단위/통합 테스트가 GoogleTest/GMock으로 작성되어 통과한다(`tester` 에이전트).
- [ ] `code-review` 에이전트가 Clean Code/함수 라인 수/SOLID/디자인 패턴 관점에서 지적한 사항이
  없거나 반영 완료됐다(특히 4.2절의 "OrderService 지역 생성 vs 주입" TODO에 대한 판단 포함).
- [ ] `supervisor` 에이전트가 본 문서(`docs/Phase6.md`)와 실제 구현/테스트 결과의 일치 여부를
  검증하고 이상 없음을 확인한다.
- [ ] Phase 5가 이 시점까지 병합되어 있다면, 더미 데이터 생성 직후 모니터링 메뉴/메인 메뉴
  요약에 값이 즉시 반영되는 것을 최소 1개의 시스템 테스트 또는 수동 확인으로 검증한다(Phase 5
  가 아직 병합 전이라면 이 항목은 Phase 5 병합 시점으로 이연하고, 대신 7.3절의 Repository
  수준 검증으로 대체한다).

---

## 9. 확인이 필요한 지점 (TODO 요약)

1. **`OrderService`를 `DummyDataGenerator`의 생성자로 주입할지, `Generate()` 내부에서 지역
   생성할지** (4.2절) — 이 코드베이스의 다른 Model 서비스들이 대체로 "생성자에 필요한 협력자를
   전부 주입받는" 패턴(`OrderApprovalService`가 `ProductionLine`을 참조로 받는 등)을 따르므로,
   더 일관된 방식이 무엇인지 `code-review`에서 재검토가 필요하다.
2. **더미 데이터 생성 메뉴 항목 번호("7")와 문구** — PRD 7.1에는 명시되지 않은 개발자 편의
   기능이므로 메인 메뉴에 정식 노출할지(현재안: "7. 더미 데이터 생성 (테스트용)"), 별도의 숨김
   메뉴로 둘지 사용자 확인이 필요하다.
3. **Phase 5의 실제 클래스/메서드명** — 2.2절/4.4절은 `docs/DATA_SCHEMA.md`의 서술을 근거로
   삼았으나, 실제 병합된 코드가 이름을 다르게 확정했다면 그 부분만 교체하면 되고 설계의 핵심
   (Repository 공유만으로 자동 반영)은 변하지 않는다.
