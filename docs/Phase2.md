# Phase 2 — 시료 주문 (예약)

## 1. 목표 요약 (PLAN.md 인용)

> **목표**: PRD 7.3 — 고객 주문 접수.
>
> - Model: `Order` 생성 (시료 ID/고객명/수량 입력 → `RESERVED` 상태로 생성, 주문번호 채번)
> - Controller/View: 주문 입력 화면 → 결과(주문번호, 상태) 출력
> - 등록되지 않은 시료 ID로는 주문할 수 없도록 검증 (PRD 7.2 "등록된 시료만 주문 가능")
>
> **완료 기준**: 주문 접수 흐름이 동작하고 `RESERVED` 주문이 영속화된다.

이 Phase의 범위는 **주문 접수(예약)** 까지다. 승인/거절, 생산 큐, 재고 계산 로직(PRD 7.4)은
Phase 3의 영역이므로 이 문서/구현에서 다루지 않는다. Phase 2에서 생성되는 주문은 항상
`RESERVED` 상태로 끝난다.

---

## 2. 전제 (Phase 0 / Phase 1 실제 산출물)

Phase 0/1이 실제로 구현/병합되었으므로, 아래는 더 이상 가정이 아니라 실제 소스 코드
(`Model/Order.h`, `Model/OrderStatus.h`, `Model/Repository.h`, `Model/JsonFileRepository.h`,
`Model/OrderRepository.h`, `Model/SampleRepository.h`, `Model/Sample.h`,
`Controller/ISubMenuController.h`, `Controller/MainMenuController.h`,
`Controller/SampleController.h`, `main.cpp`)를 그대로 옮긴 사실이다.

- **Phase 0 산출물 (확정)**
  - `Model::OrderStatus`는 C++ `enum class OrderStatus { Reserved, Rejected, Producing, Confirmed, Release }`
    (`Model/OrderStatus.h`). JSON에는 대문자 문자열("RESERVED" 등)로 저장하며, 변환은 멤버
    함수가 아니라 자유 함수 `Model::ToString(OrderStatus)` / `Model::OrderStatusFromString(const std::string&)`
    로 제공된다.
  - `Model::Order`(`Model/Order.h`)는 상태 전이 로직·검증 로직이 없는 순수 데이터 컨테이너다.
    생성자: `Order(std::string orderId, std::string sampleId, std::string customerName, int quantity, OrderStatus status, std::string createdAt, std::string updatedAt)`.
    Getter: `GetId()`(Repository 키 접근자, `GetOrderId()`와 동일 값 반환), `GetOrderId()`,
    `GetSampleId()`, `GetCustomerName()`, `GetQuantity()`, `GetStatus()`, `GetCreatedAt()`,
    `GetUpdatedAt()`. Setter: `SetStatus(OrderStatus)`, `SetUpdatedAt(std::string)`.
    `ToJson()`/`static FromJson(const Common::JsonValue&)` 제공. **`Order::CreateReserved(...)`
    같은 정적 팩토리는 아직 없다 — Phase 2가 새로 추가해야 한다** (4.2절 참고).
  - `Model::IRepository<TEntity, TId>`(`Model/Repository.h`): `std::vector<TEntity> FindAll() const`,
    `std::optional<TEntity> FindById(const TId&) const`, `void Add(const TEntity&)`,
    `void Update(const TEntity&)`, `void Remove(const TId&)`, `void Save()`, `void Load()`.
    **`Add`/`Update`는 `bool`이 아니라 `void`이며, `Add`는 중복 id를 거부하지 않고 단순
    `push_back` 후 즉시 `Save()`한다.**
  - `Model::JsonFileRepository<TEntity>`(`Model/JsonFileRepository.h`)가 위 인터페이스를
    구현하는 공용 CRUD 베이스이며, `Model::OrderRepository : JsonFileRepository<Order>`
    (기본 파일 경로 `data/orders.json`, `Model/OrderRepository.h`)가 이미 구현되어 있다.
  - `main → Controller → (Model, View)` 의존 방향, Model/View 상호 비의존 구조가 갖춰져 있다.
  - JSON 직렬화는 외부 라이브러리 없이 `Common::JsonValue`(자체 구현 파서/직렬화기)로 처리한다.
  - 테스트는 별도 실행 파일이 아니라 `SampleOrderSystem.exe --test`(Debug 빌드 전용)로
    실행한다. `Tests/*.cpp`는 `SampleOrderSystem.vcxproj`에 `Condition="'$(Configuration)'=='Debug'"`
    로만 컴파일/링크되고, `main.cpp`가 Debug 빌드에서 `--test` 인자를 받으면
    `::testing::InitGoogleMock` + `RUN_ALL_TESTS()`로 분기한다(별도 `SampleOrderSystemTests`
    프로젝트는 없다).
- **Phase 1 산출물 (확정)**
  - `Model::Sample`(`Model/Sample.h`)은 생성자 자체는 검증 없는 순수 데이터 컨테이너이고,
    검증은 정적 팩토리 `static std::optional<Sample> TryCreate(std::string id, std::string name, double avgProductionTime, double yieldRate, int stock = 0)`
    가 담당한다. **실패 시 예외를 던지지 않고 `std::nullopt`를 반환**한다 — 이는 Phase 1이
    확정한 관례이며, Phase 2도 도메인 검증 실패 표현 방식을 이 관례와 일관되게 맞춘다.
  - `Model::SampleRepository : JsonFileRepository<Sample>`(기본 파일 경로 `data/samples.json`,
    `Model/SampleRepository.h`)이 구현되어 있고, `FindById(id)`가 정확히
    `std::optional<Sample>`을 반환한다. Phase 2의 "등록된 시료인지 확인" 로직은 이 메서드를
    그대로 사용하면 된다. `SearchByName(keyword)`도 `virtual`로 제공되어 GMock으로 오버라이드
    가능하다.
  - `Controller::ISubMenuController`(`Controller/ISubMenuController.h`)는 `virtual void Run() = 0;`
    만 갖는 순수 인터페이스이며, `Controller::SampleController`가 이를 구현한다.
    `Controller::MainMenuController`(`Controller/MainMenuController.h`)는 이제 구체 클래스가
    아니라 `Controller::ISubMenuController&`(단일 서브컨트롤러, 생성자 인자명
    `sampleController`)에 의존한다(DIP, Phase 1 code-review 지적 반영).
  - `main.cpp`는 `SampleRepository`/`OrderRepository`/`ProductionJobRepository`를 만들어
    `Load()`하지만, 현재는 `SampleView`/`SampleController`만 생성해 `MainMenuController`에
    배선한다 — `OrderRepository`는 로드만 될 뿐 아직 어떤 View/Controller에도 연결돼 있지
    않다.

---

## 3. 작업 항목 체크리스트

### 3.1 Model

- [x] `Model::Order`에 정적 팩토리 `static Order CreateReserved(std::string orderId, std::string sampleId, std::string customerName, int quantity, std::string createdAt)`
      추가 — 생성 즉시 `status_ = OrderStatus::Reserved`, `updatedAt_ == createdAt`으로
      초기화. `Sample::TryCreate`와 달리 이 팩토리 자체는 수량 등 도메인 검증을 책임지지
      않는 "단순 생성" 헬퍼로 두고, 검증은 `OrderService::PlaceOrder`가 먼저 수행한 뒤에만
      호출하는 방식으로 설계한다(구현자가 검증까지 팩토리에 넣는 대안을 택할 수도 있으나,
      어느 쪽이든 "Model이 실패를 예외가 아닌 값으로 표현한다"는 Phase 1 관례를 유지해야
      한다).
- [x] 주문 수량(`quantity`)이 1 이상의 정수인지 검증하는 로직은 `OrderService::PlaceOrder`
      내부(또는 `Order`의 별도 `static bool IsValidQuantity(int)` 헬퍼)에 둔다 — `Sample`의
      `IsValidYieldRate` 등과 동일한 패턴. **상한은 두지 않는다(확정).**
- [x] 고객명(`customerName`)이 빈 문자열/공백만 있는 문자열이면 거부하는 검증도 함께 둔다
      (`Order`의 별도 `static bool IsValidCustomerName(const std::string&)` 헬퍼 권장 —
      `Sample::IsValidName`과 동일한 trim-후-비어있지 않음 패턴). **확정 — 빈 고객명은 허용하지
      않는다.**
- [x] `OrderIdGenerator` (혹은 유사 책임을 가진 작은 클래스/함수) 신설
  - [x] PRD 8.2 포맷 `ORD-YYYYMMDD-NNNN` 규칙에 맞춰 주문번호 채번
  - [x] 채번 시 당일 발급된 주문 수를 기준으로 순번(`NNNN`, 4자리 zero-padding)을 매김 —
        기존 주문 목록(`OrderRepository::FindAll()` 결과)에서 당일 최대 순번을 조회해 +1
        하는 방식으로 구현해 재시작 후에도 순번이 끊기지 않게 한다
  - [ ] (여전히 미해결 TODO) 날짜가 바뀌는 자정 경계, 동시성(단일 프로세스·단일 사용자 전제이므로
        동시성 이슈는 Non-goal로 간주하고 별도 락 처리는 하지 않음)
- [x] `OrderService`(Model 계층 유스케이스 조립 클래스) 신설 — 아래 책임을 캡슐화해 Controller가
      도메인 규칙을 직접 알 필요 없게 함
  - [x] `PlaceOrderResult PlaceOrder(const std::string& sampleId, const std::string& customerName, int quantity)`
    - [x] `SampleRepository::FindById(sampleId)`로 등록된 시료인지 조회 (PRD 7.2 "등록된 시료만
          주문 가능") — 반환값이 `std::nullopt`이면 미등록
    - [x] 미등록 시료면 주문을 생성하지 않고 실패를 반환 (`REJECTED`로 만들지 않음 — 애초에
          주문 자체가 성립하지 않는 입력 오류이므로 저장하지 않는다)
    - [x] 수량 유효성 검증 실패 시에도 마찬가지로 저장하지 않고 실패 반환
    - [x] 고객명이 빈 문자열/공백만 있는 문자열이면 마찬가지로 저장하지 않고 실패 반환
    - [x] 검증 통과 시 `OrderIdGenerator`로 채번, `Order::CreateReserved(...)`로 생성,
          `OrderRepository::Add(order)`로 영속화(내부에서 `Save()`까지 수행됨) 후 생성된
          `Order`를 반환
- [x] 위 로직에 대해 재고(`stock`)나 가용 재고 계산은 일절 관여하지 않음을 코드 주석/설계로
      명시 (Phase 3과 책임 경계를 코드 리뷰 시점에 명확히 하기 위함)

### 3.2 Controller

- [x] `OrderController`(신설, `Controller::ISubMenuController`를 구현) — 주문 접수 메뉴 흐름
      담당
  - [x] `void Run() override`: 실제로는 반복 메뉴 루프가 아니라 `HandlePlaceOrder()`를 한 번
        호출하고 반환하는 단일 액션으로 구현됨(PRD 7.3이 "주문 접수" 단일 흐름이라 판단, 계획
        문서의 "SampleController와 동일한 패턴" 가정과는 다르게 확정됨).
  - [x] `HandlePlaceOrder()`: View로부터 시료 ID/고객명/수량 입력을 받아 `OrderService::PlaceOrder`
        호출 → 성공 시 결과(주문번호, 상태)를 View에 전달해 출력, 실패 시 실패 사유를 View에
        전달해 출력
  - [x] Controller는 도메인 규칙(시료 존재 검증, 채번 규칙, 상태 전이)을 직접 구현하지 않고
        Model(`OrderService`)에 위임 — MVC 계층 분리 원칙 준수
- [x] `Controller::MainMenuController`를 확장해 두 번째 서브메뉴("2": 시료 주문)를 처리할 수
      있게 한다. 현재 생성자가 `ISubMenuController& sampleController` 하나만 받으므로, 다음 중
      하나를 택해 구현한다(둘 다 DIP를 유지하는 방법이며, 어느 쪽을 택할지는 구현자가 판단):
  - [ ] (A, 채택 안 됨) 생성자에 `ISubMenuController& orderController` 인자를 추가하고
        `HandleInput`에서 `"2"` 분기를 새로 추가하는 최소 확장안 — 매 Phase마다 생성자
        파라미터가 늘어나는 단점 때문에 채택하지 않음.
  - [x] (B, 채택됨) 생성자를 `std::map<std::string, std::reference_wrapper<ISubMenuController>>`
        로 일반화해, 이후 Phase가 생성자 시그니처를 바꾸지 않고 map 항목만 추가하도록 확정.
  - [x] 기존 `SampleController` 배선("1")을 그대로 유지했고, 관련 기존 테스트
        (`Tests/ControllerTests.cpp`)도 새 시그니처에 맞춰 갱신됨.
- [x] `main.cpp`에 `View::OrderView orderView;`, `Model::OrderService orderService(sampleRepository, orderRepository);`,
      `Controller::OrderController orderController(orderService, orderView);`를 추가하고,
      `MainMenuController` 생성 시 위에서 정한 방식(A/B)에 맞춰 `orderController`를 함께
      배선한다.

### 3.3 View

- [x] `OrderView`(신설)에 아래 출력 함수 추가
  - [x] `PromptSampleId()`, `PromptCustomerName()`, `PromptQuantity()` — 입력 프롬프트 및 raw
        입력 획득 (파싱/검증은 하지 않고 Controller/Model에 넘김, 혹은 형식 파싱 실패 시 재입력
        요청 정도의 최소 처리만 View에 둠 — 도메인 검증과 입력 형식 검증은 구분)
  - [x] `ShowOrderPlaced(orderId, status)` — 주문 성공 시 주문번호와 상태(`RESERVED`) 출력
        (PRD 10장 UI 참고: "확인 → 주문번호 및 상태(RESERVED) 출력")
  - [x] `ShowOrderRejectedInput(reason)` — 미등록 시료 ID, 잘못된 수량 등 입력 실패 사유 출력
        (주문 상태 `REJECTED`와 혼동되지 않도록 함수명/문구에 유의 — 이건 "입력 자체가 성립하지
        않아 주문이 생성되지 않은 경우"이며 PRD의 주문 거절(`REJECTED`, Phase 3 영역)과는 다른
        개념임을 화면 문구에서도 구분)

---

## 4. 인터페이스/데이터 스키마

### 4.1 Order (Phase 0에서 정의된 실제 코드, Phase 2는 재사용 — `Model/Order.h`, `Model/OrderStatus.h`)

```cpp
namespace Model {

enum class OrderStatus { Reserved, Rejected, Producing, Confirmed, Release };

std::string ToString(OrderStatus status);                       // "RESERVED" 등 대문자 문자열
OrderStatus OrderStatusFromString(const std::string& text);

class Order {
public:
    Order() = default;
    Order(std::string orderId, std::string sampleId, std::string customerName, int quantity,
          OrderStatus status, std::string createdAt, std::string updatedAt);

    const std::string& GetId() const;          // == GetOrderId(), Repository 키 접근자
    const std::string& GetOrderId() const;
    const std::string& GetSampleId() const;
    const std::string& GetCustomerName() const;
    int GetQuantity() const;
    OrderStatus GetStatus() const;
    const std::string& GetCreatedAt() const;
    const std::string& GetUpdatedAt() const;

    void SetStatus(OrderStatus status);
    void SetUpdatedAt(std::string updatedAt);

    Common::JsonValue ToJson() const;
    static Order FromJson(const Common::JsonValue& json);
};

}  // namespace Model
```

> 이 클래스에는 아직 상태 전이/검증 로직이 없다(Phase 0에서 의도적으로 그렇게 설계됨,
> `Order.h` 주석 "status transition rules ... belong to Phase 2/3/4" 참고). Phase 2가 이
> 파일에 `CreateReserved` 팩토리를 추가하는 첫 Phase다.

### 4.2 Phase 2에서 새로 추가되는 인터페이스

```cpp
// Model 계층 — Model/Order.h에 팩토리 추가
namespace Model {
class Order {
    // ... 기존 멤버에 추가 ...
    static Order CreateReserved(std::string orderId, std::string sampleId,
                                 std::string customerName, int quantity,
                                 std::string createdAt);
};
}

// Model 계층 — 신설 파일 Model/OrderIdGenerator.h
namespace Model {
class OrderIdGenerator {
public:
    // existingOrders: 오늘 날짜 기준 최대 순번을 판단하기 위해 조회된 기존 주문 목록
    // (OrderRepository::FindAll()의 결과를 그대로 전달)
    std::string GenerateNextId(const std::vector<Order>& existingOrders,
                                const std::string& today) const;
};
}

// Model 계층 — 신설 파일 Model/OrderService.h
namespace Model {
struct PlaceOrderResult {
    bool success = false;
    std::optional<Order> order;   // success == true일 때만 유효
    std::string failureReason;    // success == false일 때만 유효 (예: "등록되지 않은 시료 ID", "수량은 1 이상이어야 함", "고객명은 비어있을 수 없음")
};

class OrderService {
public:
    OrderService(SampleRepository& sampleRepo, OrderRepository& orderRepo);

    PlaceOrderResult PlaceOrder(const std::string& sampleId,
                                const std::string& customerName,
                                int quantity);
};
}
```

```cpp
// Controller 계층 — 신설 파일 Controller/OrderController.h
namespace Controller {
class OrderController : public ISubMenuController {
public:
    OrderController(Model::OrderService& orderService, View::OrderView& orderView);

    void Run() override;

private:
    void HandlePlaceOrder();
};
}
```

```cpp
// View 계층 — 신설 파일 View/OrderView.h
namespace View {
class OrderView {
public:
    std::string PromptSampleId();
    std::string PromptCustomerName();
    int PromptQuantity();

    void ShowOrderPlaced(const std::string& orderId, Model::OrderStatus status);
    void ShowOrderRejectedInput(const std::string& reason);
};
}
```

> `PlaceOrderResult`가 `Result`/예외 대신 성공 플래그 + `std::optional` 조합으로 실패를
> 표현하는 방식은 Phase 1이 `Sample::TryCreate`(실패 시 `std::nullopt`, 예외 없음)로 확정한
> 관례와 일관된다. 핵심은 "Model이 도메인 검증·채번·영속화를 담당하고 Controller/View는
> 흐름·출력만 담당한다"는 책임 분리이며, 이는 반드시 지켜져야 한다.

### 4.3 JSON 스키마

Phase 2는 Phase 0에서 정의된 Order JSON 스키마를 그대로 사용하며 필드를 추가하지 않는다.
(PRD 8.2 참고. `status` 필드는 `Model::ToString(OrderStatus)`가 생성하는 대문자 문자열이다.)

```json
{
  "orderId": "ORD-20260416-0043",
  "sampleId": "S-001",
  "customerName": "홍길동",
  "quantity": 50,
  "status": "RESERVED",
  "createdAt": "2026-07-15T10:00:00",
  "updatedAt": "2026-07-15T10:00:00"
}
```

---

## 5. 의존 관계

- **Phase 0이 이미 제공한 것**: `Model::Order`/`Model::OrderStatus` 정의(단, 상태 전이/
  검증/팩토리 없음 — Phase 2가 채움), `Model::OrderRepository`(`JsonFileRepository<Order>`
  상속, `FindAll`/`FindById`/`Add`/`Update`/`Remove`/`Save`/`Load` 모두 사용 가능, 기본 경로
  `data/orders.json`), `Common::JsonValue` 기반 JSON 직렬화, Debug 전용 `--test` 실행 경로.
- **Phase 1이 이미 제공한 것**: `Model::SampleRepository::FindById(id) -> std::optional<Sample>`을
  통한 "ID로 등록 여부 확인", `Controller::ISubMenuController` 인터페이스와 이를 구현하는
  `SampleController` 예시(신설 `OrderController`가 따를 참조 패턴), `Model::Sample::TryCreate`
  가 확정한 "검증 실패는 `std::nullopt`로 표현" 관례.
- **Phase 2가 `MainMenuController`/`main.cpp`에 가하는 변경**: 3.2절에서 정한 방식(A: 생성자
  파라미터 추가 / B: 맵 기반 일반화)에 따라 `MainMenuController` 생성자 시그니처가 바뀌므로,
  기존에 이를 생성하던 `main.cpp`와 `Tests/ControllerTests.cpp` 등 관련 테스트를 함께
  갱신해야 한다.
- **Phase 3이 이 Phase에 기대하는 것**: `RESERVED` 상태로 영속화된 `Order` 목록을
  `OrderRepository::FindAll()`로 그대로 조회할 수 있어야 한다 (Phase 3의 "RESERVED 목록
  표시 → 승인/거절" 흐름의 입력). 기존 `IRepository`/`JsonFileRepository` 시그니처를 그대로
  사용하면 되며, Phase 2가 별도 조회 메서드를 추가할 필요는 없다.
- **Phase 5(모니터링)가 기대하는 것**: `RESERVED` 주문이 정상적으로 영속화되어 있어야
  상태별 건수 집계에 반영된다.

---

## 6. 핵심 비즈니스 로직 주의사항

Phase 2 자체에는 PRD 7.4(승인 시점 생산량 확정 규칙) 같은 복잡한 계산은 없다. 다만 Phase 2
구현 시 다음을 반드시 지켜 Phase 3과의 경계를 흐리지 않는다.

- **재고를 절대 건드리지 않는다.** 주문 접수(`RESERVED` 생성)는 `Sample.stock`이나 "가용
  재고" 개념에 전혀 관여하지 않는다. 재고 소진/가용 재고 계산은 승인 시점(Phase 3)에만
  일어난다. Phase 2 구현 중 실수로 `Sample.stock`을 차감하는 코드를 넣지 않도록 주의한다.
- **등록된 시료만 주문 가능** (PRD 7.2): `sampleId`로 `SampleRepository::FindById(sampleId)`를
  호출했을 때 `std::nullopt`이면 주문 자체를 생성하지 않는다. 이 실패는 `REJECTED` 주문을
  만드는 것이 아니라, 애초에 `Order`가 영속화되지 않는 "입력 검증 실패"로 처리한다.
  (`REJECTED`는 정상적으로 접수된 `RESERVED` 주문에 대해 생산 담당자가 승인/거절을 판단한
  결과이며 Phase 3의 개념이다.)
- **주문번호 채번은 순서를 보장해야 한다.** Phase 3에서 "주문이 들어온 순서대로 재고를
  소진"하는 규칙(PRD 7.4)을 적용하려면, `createdAt`(및 채번 순서)이 실제 접수 순서를
  정확히 반영해야 한다. 따라서 채번/생성 시각 기록은 반드시 `Order`가 실제로 생성되는
  시점에 이뤄져야 하며, 재입력/재시도 등으로 순서가 뒤바뀌지 않도록 한다.
- **주문 수량 검증 (확정)**: 하한(1 이상의 정수)만 검증하며, 최대 수량 상한은 두지 않는다
  (사용자 확인 완료 — PRD에도 상한 명시가 없다).

---

## 7. 테스트 계획 (tester 에이전트 작성 대상)

> **테스트 실행 방식**: 별도 `SampleOrderSystemTests` 실행 파일은 없다. `Tests/*.cpp`에
> 테스트 소스를 추가하면 `SampleOrderSystem.vcxproj`가 Debug 빌드에서만 이를 컴파일/링크하고,
> `SampleOrderSystem.exe --test`를 실행하면 `RUN_ALL_TESTS()`가 수행된다(`main.cpp` 참고).
> tester 에이전트는 새 파일(예: `Tests/OrderServiceTests.cpp`, `Tests/OrderControllerTests.cpp`)을
> 이 프로젝트 트리에 추가하고 `SampleOrderSystem.exe --test`로 빌드/실행 결과를 보고한다.

### 7.1 Model 단위 테스트 (`OrderService`, `OrderIdGenerator`)

- 정상 케이스
  - 등록된 시료 ID + 유효한 고객명 + 수량(예: 50)으로 주문 시, `RESERVED` 상태의 `Order`가
    생성되고 `OrderRepository`에 저장된다.
  - 생성된 `Order`의 `createdAt == updatedAt`이다.
  - 같은 날짜에 이미 N건의 주문이 있는 상태에서 신규 주문 시 채번 순번이 `N+1`이 된다
    (`ORD-YYYYMMDD-000N` 형식, 4자리 zero-padding 확인).
  - 날짜가 다르면 순번이 0001부터 다시 시작한다.
- 경계값 / 실패 케이스
  - 미등록 시료 ID로 주문 시 `PlaceOrderResult.success == false`이고 `Order`가
    `OrderRepository`에 저장되지 않는다 (Repository mock으로 `Add`가 호출되지 않았음을
    검증 — GMock 활용).
  - 수량이 0 또는 음수일 때 실패하고 저장되지 않는다.
  - 고객명이 빈 문자열(또는 공백만 있는 문자열)일 때 실패하고 저장되지 않는다 (확정 — 사용자
    확인 완료, PRD에 형식 제약 명시는 없으나 빈 값은 거부하는 것으로 정책 확정).
- Repository/Sample 존재 여부 확인 시, `Tests/SampleControllerTests.cpp`의
  `MockSampleRepository`(더미 파일 경로로 생성한 `Model::SampleRepository` 서브클래스에
  `MOCK_METHOD(..., override)`로 `FindById`/`FindAll`/`Add`/`SearchByName`을 오버라이드하는
  패턴)와 동일한 방식으로 `MockSampleRepository`/`MockOrderRepository`를 만들어
  `OrderService`가 실제 시료 존재 확인 로직을 정확히 한 번 호출하는지, 미등록 시료일 때
  `OrderRepository::Add`가 호출되지 않는지 검증.

### 7.2 Controller/통합(시스템) 테스트

- 콘솔 입출력을 흉내 낸 통합 테스트(또는 `OrderController` + 실제(파일 기반) Repository
  조합)로 다음을 검증
  - 시료 ID/고객명/수량을 입력하면 화면에 주문번호와 `RESERVED` 상태가 출력된다.
  - 미등록 시료 ID 입력 시 실패 사유가 출력되고 주문이 생성되지 않는다.
- 영속성 회귀 테스트: 주문 접수 → 앱(프로세스) 재시작을 흉내 낸 Repository 재로드 →
  방금 생성한 `RESERVED` 주문이 그대로 조회된다 (Phase 0에서 이미 검증됐더라도 Phase 2
  기능이 실제로 영속화 경로를 타는지 별도로 확인).

---

## 8. 완료 조건 (Definition of Done)

- [x] 콘솔에서 메인 메뉴 → "2" 입력으로 시료 주문 메뉴 진입 → 시료 ID/고객명/수량 입력 →
      주문번호와 `RESERVED` 상태가 출력되는 흐름이 실제로 동작한다(`MainMenuController`가
      `OrderController`로 정상 라우팅됨을 포함).
- [x] 등록되지 않은 시료 ID로는 주문이 생성되지 않고, 그 사유가 화면에 안내된다.
- [x] 생성된 주문이 JSON 파일(`data/orders.json`)에 영속화되며, 앱 재시작(재로드) 후에도
      동일하게 조회된다.
- [x] `main.cpp`가 `OrderService`/`OrderView`/`OrderController`를 생성해 배선하고, 기존
      `SampleController` 배선("1")과 관련 기존 테스트(`Tests/ControllerTests.cpp` 등)가
      `MainMenuController` 시그니처 변경 이후에도 계속 통과한다.
- [x] `code-review` 에이전트 검토 통과 — 특히 Model(도메인 검증/채번/영속화)과
      Controller/View(입출력 흐름) 책임 분리가 유지되는지, 함수 길이/SOLID 원칙 준수 여부.
- [x] `tester` 에이전트가 위 7장의 테스트 시나리오를 GoogleTest/GMock으로 작성하고,
      `SampleOrderSystem.exe --test`(Debug) 빌드 및 테스트 실행이 전부 통과한다.
- [x] Phase 3이 그대로 이어받을 수 있도록 `OrderRepository::FindAll()`을 통한 `RESERVED`
      주문 목록 조회가 사용 가능한 상태로 남아 있다.
