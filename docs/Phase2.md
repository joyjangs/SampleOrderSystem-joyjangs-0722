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

## 2. 전제 (Phase 0 / Phase 1 산출물 가정)

Phase 0/1 문서가 아직 확정되지 않은 상태에서 작성하므로, PLAN.md 기술 내용을 근거로 다음을
전제로 삼는다. 실제 Phase 0/1 문서가 확정되면 아래 가정과 다른 부분(특히 클래스/함수 시그니처)
은 이 문서를 갱신해 맞춘다.

- **Phase 0 산출물**
  - `Model/Order.h`(또는 `.hpp`)에 `Order` 도메인 클래스와 `OrderStatus` enum
    (`RESERVED`, `REJECTED`, `PRODUCING`, `CONFIRMED`, `RELEASE`)이 이미 정의돼 있다.
  - `Order` 리포지토리(`OrderRepository` 등, 정확한 이름은 Phase 0 문서 확정 후 대조)가 JSON
    파일 기반으로 CRUD를 제공하며, 앱 재시작 후에도 데이터가 복원된다.
  - `main → Controller → (Model, View)` 의존 방향, Model/View 상호 비의존 구조가 이미
    갖춰져 있다.
  - JSON 직렬화 방식(필드명, 라이브러리: nlohmann/json 등)이 Phase 0에서 확정돼 있고, Phase 2는
    이를 그대로 따른다.
- **Phase 1 산출물**
  - `Sample` 등록/조회/검색 기능이 동작하며, `SampleRepository`(정확한 이름은 Phase 1 문서
    확정 후 대조)를 통해 등록된 시료 목록/단건 조회(ID로 존재 여부 확인)가 가능하다.
  - 시료 관리 메뉴가 메인 메뉴에 연결돼 있다.

> TODO: 확인 필요 — Phase 0/1 문서가 확정되면 Repository 클래스명·메서드 시그니처
> (`FindById`/`GetById` 등 명명 규칙)를 이 문서와 대조해 일치시킨다.

---

## 3. 작업 항목 체크리스트

### 3.1 Model

- [ ] `Order` 클래스(Phase 0에서 정의된 것을 재사용)에 아래 책임이 없다면 추가
  - [ ] 정적 팩토리 또는 생성자: `Order::CreateReserved(orderId, sampleId, customerName, quantity, createdAt)`
        형태로, 생성 즉시 `status = RESERVED`, `createdAt == updatedAt`으로 초기화되도록 보장
        (상태 전이 로직이 View/Controller에 새지 않도록 Model 내부에 캡슐화)
  - [ ] 주문 수량(`quantity`)이 1 이상의 정수인지 검증하는 로직 (0 이하 입력은 Model 단에서
        거부 — 예외 또는 결과 타입으로 실패를 알림)
- [ ] `OrderIdGenerator` (혹은 유사 책임을 가진 작은 클래스/함수) 신설
  - [ ] PRD 8.2 포맷 `ORD-YYYYMMDD-NNNN` 규칙에 맞춰 주문번호 채번
  - [ ] 채번 시 당일 발급된 주문 수를 기준으로 순번(`NNNN`, 4자리 zero-padding)을 매김 —
        기존 주문 목록(Repository 조회 결과)에서 당일 최대 순번을 조회해 +1 하는 방식으로 구현해
        재시작 후에도 순번이 끊기지 않게 한다
  - [ ] TODO: 확인 필요 — 날짜가 바뀌는 자정 경계, 동시성(단일 프로세스·단일 사용자 전제이므로
        동시성 이슈는 Non-goal로 간주하고 별도 락 처리는 하지 않음)
- [ ] `OrderService`(또는 Controller가 직접 접근하지 않고 Model 계층에 두는 유스케이스 조립
      클래스) 신설 — 아래 책임을 캡슐화해 Controller가 도메인 규칙을 직접 알 필요 없게 함
  - [ ] `PlaceOrder(sampleId, customerName, quantity) -> Result<Order>` (또는 예외 기반)
    - [ ] `SampleRepository`를 통해 `sampleId`가 등록된 시료인지 조회 (PRD 7.2 "등록된 시료만
          주문 가능")
    - [ ] 미등록 시료면 주문을 생성하지 않고 실패를 반환 (`REJECTED`로 만들지 않음 — 애초에
          주문 자체가 성립하지 않는 입력 오류이므로 저장하지 않는다)
    - [ ] 수량 유효성 검증 실패 시에도 마찬가지로 저장하지 않고 실패 반환
    - [ ] 검증 통과 시 `OrderIdGenerator`로 채번, `Order::CreateReserved(...)`로 생성,
          `OrderRepository::Add/Save`로 영속화 후 생성된 `Order`를 반환
- [ ] 위 로직에 대해 재고(`stock`)나 가용 재고 계산은 일절 관여하지 않음을 코드 주석/설계로
      명시 (Phase 3과 책임 경계를 코드 리뷰 시점에 명확히 하기 위함)

### 3.2 Controller

- [ ] `OrderController`(신설) — 주문 접수 메뉴 흐름 담당
  - [ ] `HandlePlaceOrder()`: View로부터 시료 ID/고객명/수량 입력을 받아 `OrderService::PlaceOrder`
        호출 → 성공 시 결과(주문번호, 상태)를 View에 전달해 출력, 실패 시 실패 사유를 View에
        전달해 출력
  - [ ] Controller는 도메인 규칙(시료 존재 검증, 채번 규칙, 상태 전이)을 직접 구현하지 않고
        Model(`OrderService`)에 위임 — MVC 계층 분리 원칙 준수
- [ ] 메인 메뉴에 "시료 주문" 항목을 연결 (Phase 1에서 만들어진 메인 메뉴 라우팅 구조를 재사용)

### 3.3 View

- [ ] `OrderView`(신설)에 아래 출력 함수 추가
  - [ ] `PromptSampleId()`, `PromptCustomerName()`, `PromptQuantity()` — 입력 프롬프트 및 raw
        입력 획득 (파싱/검증은 하지 않고 Controller/Model에 넘김, 혹은 형식 파싱 실패 시 재입력
        요청 정도의 최소 처리만 View에 둠 — 도메인 검증과 입력 형식 검증은 구분)
  - [ ] `ShowOrderPlaced(orderId, status)` — 주문 성공 시 주문번호와 상태(`RESERVED`) 출력
        (PRD 10장 UI 참고: "확인 → 주문번호 및 상태(RESERVED) 출력")
  - [ ] `ShowOrderRejectedInput(reason)` — 미등록 시료 ID, 잘못된 수량 등 입력 실패 사유 출력
        (주문 상태 `REJECTED`와 혼동되지 않도록 함수명/문구에 유의 — 이건 "입력 자체가 성립하지
        않아 주문이 생성되지 않은 경우"이며 PRD의 주문 거절(`REJECTED`, Phase 3 영역)과는 다른
        개념임을 화면 문구에서도 구분)

---

## 4. 인터페이스/데이터 스키마

### 4.1 Order (Phase 0에서 정의, Phase 2는 재사용만 함 — PRD 8.2 그대로)

```cpp
enum class OrderStatus {
    RESERVED,
    REJECTED,
    PRODUCING,
    CONFIRMED,
    RELEASE
};

struct Order {
    std::string orderId;       // "ORD-20260416-0043"
    std::string sampleId;
    std::string customerName;
    int quantity;
    OrderStatus status;
    std::string createdAt;     // ISO 8601 문자열 등 Phase 0에서 확정한 포맷 그대로 사용
    std::string updatedAt;
};
```

> Phase 2는 위 구조체/클래스 정의를 새로 만들지 않는다. Phase 0 문서가 확정되면 실제 필드명/
> 타입(`datetime` 표현 방식 등)을 대조해 이 표를 갱신한다.

### 4.2 Phase 2에서 새로 추가되는 인터페이스 (제안)

```cpp
// Model 계층
class OrderIdGenerator {
public:
    // existingOrders: 오늘 날짜 기준 최대 순번을 판단하기 위해 조회된 기존 주문 목록
    std::string GenerateNextId(const std::vector<Order>& existingOrders,
                                const std::string& today) const;
};

struct PlaceOrderResult {
    bool success;
    std::optional<Order> order;   // success == true일 때만 유효
    std::string failureReason;    // success == false일 때만 유효 (예: "등록되지 않은 시료 ID", "수량은 1 이상이어야 함")
};

class OrderService {
public:
    explicit OrderService(SampleRepository& sampleRepo, OrderRepository& orderRepo);

    PlaceOrderResult PlaceOrder(const std::string& sampleId,
                                const std::string& customerName,
                                int quantity);
};
```

```cpp
// Controller 계층
class OrderController {
public:
    OrderController(OrderService& orderService, OrderView& orderView);

    void HandlePlaceOrder();
};
```

```cpp
// View 계층
class OrderView {
public:
    std::string PromptSampleId();
    std::string PromptCustomerName();
    int PromptQuantity();

    void ShowOrderPlaced(const std::string& orderId, OrderStatus status);
    void ShowOrderRejectedInput(const std::string& reason);
};
```

> 위 클래스/메서드명은 제안이며, Phase 0/1 문서가 확정한 명명 규칙(예: `Repository` 대신
> `Store`, `Result` 대신 예외 기반 등)과 다르면 그에 맞춰 조정한다. 핵심은 "Model이 도메인
> 검증·채번·영속화를 담당하고 Controller/View는 흐름·출력만 담당한다"는 책임 분리이며, 이는
> 반드시 지켜져야 한다.

### 4.3 JSON 스키마

Phase 2는 Phase 0에서 정의된 Order JSON 스키마를 그대로 사용하며 필드를 추가하지 않는다.
(PRD 8.2 참고)

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

- **Phase 0에 기대하는 것**: `Order`/`OrderStatus` 정의, `OrderRepository`(CRUD, 특히
  전체 목록 조회와 신규 추가), JSON 직렬화 방식, 날짜/시간 문자열 포맷 유틸리티(있다면).
- **Phase 1에 기대하는 것**: `Sample`/`SampleRepository`를 통한 "ID로 등록 여부 확인" 기능,
  메인 메뉴 라우팅 구조(하위 메뉴 추가 방식).
- **Phase 3이 이 Phase에 기대하는 것**: `RESERVED` 상태로 영속화된 `Order` 목록을 그대로
  조회할 수 있어야 한다 (Phase 3의 "RESERVED 목록 표시 → 승인/거절" 흐름의 입력). 따라서
  `OrderRepository`에는 최소한 "전체 조회" 또는 "상태별 조회"가 가능해야 하며, Phase 2에서
  이 조회 인터페이스가 이미 사용 가능한 상태여야 한다(Phase 0에서 이미 제공될 것으로 가정하되,
  Phase 2 구현 중 부족하면 Repository에 조회 메서드를 보강할 수 있다 — 단, Repository의
  기본 CRUD 시그니처 자체를 깨지 않는 범위에서).
- **Phase 5(모니터링)가 기대하는 것**: `RESERVED` 주문이 정상적으로 영속화되어 있어야
  상태별 건수 집계에 반영된다.

---

## 6. 핵심 비즈니스 로직 주의사항

Phase 2 자체에는 PRD 7.4(승인 시점 생산량 확정 규칙) 같은 복잡한 계산은 없다. 다만 Phase 2
구현 시 다음을 반드시 지켜 Phase 3과의 경계를 흐리지 않는다.

- **재고를 절대 건드리지 않는다.** 주문 접수(`RESERVED` 생성)는 `Sample.stock`이나 "가용
  재고" 개념에 전혀 관여하지 않는다. 재고 소진/가용 재고 계산은 승인 시점(Phase 3)에만
  일어난다. Phase 2 구현 중 실수로 `Sample.stock`을 차감하는 코드를 넣지 않도록 주의한다.
- **등록된 시료만 주문 가능** (PRD 7.2): `sampleId`가 `SampleRepository`에 존재하지 않으면
  주문 자체를 생성하지 않는다. 이 실패는 `REJECTED` 주문을 만드는 것이 아니라, 애초에
  `Order`가 영속화되지 않는 "입력 검증 실패"로 처리한다. (`REJECTED`는 정상적으로 접수된
  `RESERVED` 주문에 대해 생산 담당자가 승인/거절을 판단한 결과이며 Phase 3의 개념이다.)
- **주문번호 채번은 순서를 보장해야 한다.** Phase 3에서 "주문이 들어온 순서대로 재고를
  소진"하는 규칙(PRD 7.4)을 적용하려면, `createdAt`(및 채번 순서)이 실제 접수 순서를
  정확히 반영해야 한다. 따라서 채번/생성 시각 기록은 반드시 `Order`가 실제로 생성되는
  시점에 이뤄져야 하며, 재입력/재시도 등으로 순서가 뒤바뀌지 않도록 한다.
- **주문 수량 검증**: PRD에 명시적 상한선은 없으므로 하한(1 이상의 정수)만 검증한다.
  TODO: 확인 필요 — 최대 주문 수량 상한이 필요한지는 PRD에 명시돼 있지 않음. 필요 시 별도
  지시가 있을 때 반영한다.

---

## 7. 테스트 계획 (tester 에이전트 작성 대상)

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
  - 고객명이 빈 문자열일 때의 처리 — TODO: 확인 필요 (PRD에 고객명 형식 제약 명시 없음;
    빈 문자열 허용 여부를 정책으로 정하고 테스트에 반영. 잠정: 빈 문자열은 거부).
- Repository/Sample 존재 여부 확인 시 GMock으로 `SampleRepository`를 모킹해
  `OrderService`가 실제 시료 존재 확인 로직을 정확히 한 번 호출하는지 검증.

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

- [ ] 콘솔에서 메인 메뉴 → 시료 주문 메뉴 진입 → 시료 ID/고객명/수량 입력 → 주문번호와
      `RESERVED` 상태가 출력되는 흐름이 실제로 동작한다.
- [ ] 등록되지 않은 시료 ID로는 주문이 생성되지 않고, 그 사유가 화면에 안내된다.
- [ ] 생성된 주문이 JSON 파일에 영속화되며, 앱 재시작(재로드) 후에도 동일하게 조회된다.
- [ ] `code-review` 에이전트 검토 통과 — 특히 Model(도메인 검증/채번/영속화)과
      Controller/View(입출력 흐름) 책임 분리가 유지되는지, 함수 길이/SOLID 원칙 준수 여부.
- [ ] `tester` 에이전트가 위 7장의 테스트 시나리오를 GoogleTest/GMock으로 작성하고, 빌드 및
      테스트 실행이 전부 통과한다.
- [ ] Phase 3이 그대로 이어받을 수 있도록 `RESERVED` 주문 목록 조회 인터페이스가 사용
      가능한 상태로 남아 있다.
