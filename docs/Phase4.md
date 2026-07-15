# Phase 4 — 출고 처리

## 1. 목표 요약 (PLAN.md 인용)

> **목표**: PRD 7.7.
>
> - Model: `CONFIRMED` 주문 → 출고 처리 → `RELEASE`
> - Controller/View: `CONFIRMED` 목록 표시 → 번호 선택 → 출고 완료 안내
>
> **완료 기준**: 승인부터 출고까지 전체 주문 라이프사이클이 콘솔에서 end-to-end로 동작.

PRD 7.7 원문:

> - 재고가 충분해진 `CONFIRMED` 상태 주문에 대해 출고 처리
> - 특정 주문에 대해 출고 실행 → 주문 상태를 `RELEASE`로 전환

PRD 10장 UI 참고:

> - 출고 처리: `CONFIRMED` 목록 표시 → 번호 선택 → 출고 처리 완료 및 상태(`RELEASE`) 안내

Phase 4는 **PRD 7.7에만 집중**한다. 상태 전이 로직(`RESERVED`/`REJECTED`/`PRODUCING`/
`CONFIRMED` 확정, 생산 큐, 승인 시점 생산량 확정 규칙)은 Phase 3의 산출물을 그대로 전제로
삼고 재구현하지 않는다.

---

## 2. 전제 (Phase 0~3 산출물에 대한 가정)

Phase 4 작성 시점에 `docs/Phase0.md`~`docs/Phase3.md`가 동시 작성 중이라 실제 텍스트를
참조할 수 없었다. 아래는 PLAN.md/PRD.md에 명시된 내용만을 근거로 삼은 전제이며, 실제 Phase
0~3 문서 및 구현 결과가 이와 다르면 **인터페이스명은 다르더라도 아래 책임 분리 원칙은
유지**되어야 한다.

- `Order` 모델은 `orderId`, `sampleId`, `customerName`, `quantity`, `status`(enum:
  `RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASE`), `createdAt`, `updatedAt`
  필드를 가진다 (PRD 8.2).
- `Sample` 모델은 `id`, `name`, `avgProductionTime`, `yieldRate`, `stock`(물리적 재고)
  필드를 가진다 (PRD 8.1). **물리적 `stock`은 두 시점에 걸쳐 변경되는 것이 확정된 설계다**
  (PRD 7.4, Phase 3 문서 3.2절/6.1절): (1) 생산 완료 시 `stock += actualQuantity` (Phase 3
  담당), (2) 출고(`RELEASE`) 처리 시 `stock -= order.quantity` (Phase 4 담당, 본 문서).
  Phase 3의 가용 재고 계산식(`availableStock = sample.stock - Σ(CONFIRMED/PRODUCING 주문
  수량)`)은 `CONFIRMED` 주문이 출고되어 물리적 `stock`에서 실제로 빠지기 전까지는 "이미
  배정된 것"으로 간주해 차감 대상에서 제외하는 방식이므로, 출고 시 물리적 `stock`이 줄지
  않으면 이후 새 주문들이 이미 배정된 재고를 중복으로 가용 재고에 포함시키는 오류가 발생한다.
  따라서 Phase 4는 출고 처리 로직 안에서 반드시 `Sample.stock -= order.quantity`를 실행해야
  한다.
- Repository 계층(Phase 0)은 최소한 다음 형태의 CRUD를 제공한다고 가정한다.
  - `OrderRepository`: `FindAll()`, `FindByStatus(OrderStatus)`, `FindById(orderId)`,
    `Update(Order)` (또는 `Save(Order)`) — Phase 4는 이 중 상태 필터 조회 + 단건 갱신만
    사용한다.
  - `SampleRepository`: 시료 조회는 화면 표시(시료명 등 부가 정보)용으로 필요하며, **출고
    처리 시 물리적 `stock`을 차감하기 위한 갱신 대상이기도 하다** (`Update(Sample)` 또는
    `Save(Sample)`).
- Phase 3 종료 시점에 `CONFIRMED` 상태 주문이 최소 1건 이상 존재할 수 있는 상태(테스트
  데이터 또는 실제 승인 흐름)가 이미 만들어져 있다.
- MVC 의존 방향(`main → Controller → (Model, View)`, Model/View 상호 비의존)은 Phase 0에서
  확정된 그대로 유지한다.

---

## 3. 작업 항목 체크리스트

### 3.1 Model

- [ ] `Order`에 출고 처리 전용 메서드 추가: `Release()` (또는 동등한 이름).
  - 사전조건: 현재 상태가 `CONFIRMED`가 아니면 예외/오류 반환 (`RESERVED`, `PRODUCING`,
    `REJECTED`, 이미 `RELEASE`인 주문에 대한 재출고 시도를 차단).
  - 사후조건: `status = RELEASE`, `updatedAt`을 현재 시각으로 갱신.
  - 상태 전이 로직 자체(전이 가능 여부 판단)는 `Order` 클래스(Model) 내부에 두고,
    Controller에는 "허용되지 않는 전이"에 대한 도메인 로직이 새어나가지 않게 한다
    (CLAUDE.md MVC 원칙).
  - `Order::Release()` 자체는 `Sample.stock`을 알지 못하므로(단일 책임 — `Order`는 자신의
    상태 전이만 책임진다) 물리적 재고 차감은 아래 유스케이스/서비스 계층 또는 Controller가
    `Sample`을 함께 갱신하는 방식으로 처리한다.
- [ ] 출고 처리를 담당하는 유스케이스/서비스 객체 도입.
  - Phase 3에서 이미 "주문 승인 처리"를 담당하는 서비스 클래스(예: `OrderApprovalService`
    류)를 두었다면, 동일한 패턴으로 `OrderReleaseService`(가칭)를 Model 계층에 두어
    Controller는 이 서비스만 호출하게 한다.
  - 최소 책임: (1) `orderId`로 주문 조회, (2) `CONFIRMED` 여부 검증, (3) `Order::Release()`
    호출, (4) 해당 `Order.sampleId`로 `Sample` 조회 후 **`sample.stock -= order.quantity`
    적용**(생산 완료 시 `stock`을 증가시킨 것과 대칭되는, 확정된 재고 감소 로직 —
    Phase 3 문서 2.1/3.2절 참고), (5) `OrderRepository`/`SampleRepository`를 통한 영속화,
    (6) 결과(성공/실패 사유) 반환.
  - TODO: 확인 필요 — Phase 3가 이런 서비스 클래스 패턴을 이미 확정했는지 여부. 확정되지
    않았다면 Phase 4에서 가장 단순한 형태(주문/시료 자체에는 상태 전이·재고 증감 메서드만
    두고 Controller가 Repository 조회/저장을 조합)로 구현해도 무방하다. 다만 재고 차감
    로직 자체는 반드시 존재해야 하며, 과설계(불필요한 서비스 계층 신설)만 피하면 된다.

### 3.2 Controller

- [ ] `ShipmentController`(가칭, 이름은 기존 Controller 네이밍 컨벤션을 따른다 — Phase 1~3
  에서 확정된 접미사/스타일이 있다면 그대로 사용) 신설 또는 기존 Order 관련 Controller에
  출고 처리 흐름 추가.
  - 흐름: `CONFIRMED` 주문 목록을 `OrderRepository`에서 조회 → View에 목록 전달/출력 →
    사용자에게 번호(또는 주문번호) 입력 요청 → 선택된 주문에 대해 Model의 출고 처리 호출 →
    결과를 View에 전달해 안내 메시지 출력.
  - 예외 처리: `CONFIRMED` 주문이 하나도 없을 때 View에 "출고 대상 없음" 메시지만 표시하고
    입력을 받지 않는다. 잘못된 번호 입력 시 재입력 유도 또는 취소 처리(구체적 UX는 자유
    결정 — PRD 10장 "실제 화면 구성은 자유롭게 결정 가능").
- [ ] 메인 메뉴(Phase 0/1에서 구축된 메뉴 라우팅)에 "출고 처리" 항목이 이미 자리만 잡혀
  있다면 실제 Controller 호출로 연결한다. PRD 7.1 메뉴 표에 "출고 처리"가 이미 정의돼
  있으므로 신규 메뉴 항목을 만드는 것이 아니라 기존 자리에 기능을 연결하는 작업이다.

### 3.3 View

- [ ] `CONFIRMED` 주문 목록 출력 화면 구현: 최소한 주문번호, 시료 ID(또는 시료명), 고객명,
  수량, 상태를 표 형태로 출력. (Phase 1의 시료 목록 출력 스타일과 일관되게.)
- [ ] 번호 선택 입력 프롬프트.
- [ ] 출고 완료 안내 메시지: 처리된 주문번호와 변경된 상태(`RELEASE`)를 명시적으로 출력.
- [ ] 실패 케이스 메시지: 존재하지 않는 주문번호, `CONFIRMED`가 아닌 주문 선택 시 사유를
  포함한 안내.
- [ ] View는 Model을 직접 참조하지 않고 Controller가 넘겨준 데이터(DTO 또는 `Order` 목록)만
  받아 출력한다 (MVC 의존 방향 유지).

---

## 4. 인터페이스/데이터 스키마

이 Phase는 PRD 8.2 `Order` 스키마에 **새 필드를 추가하지 않는다**. 기존 필드의 상태 값만
`CONFIRMED → RELEASE`로 전이시키고 `updatedAt`을 갱신한다.

```cpp
// Model 계층 — Order (Phase 0~3에서 이미 정의된 클래스에 메서드 추가)
class Order {
public:
    // ... 기존 멤버 (Phase 0~3 문서 참고)

    // Phase 4에서 추가
    // 사전조건 위반 시 false 반환(또는 예외 — 기존 Order 클래스의 에러 처리 컨벤션을 따름)
    bool Release();
};
```

```cpp
// Model 계층 — 출고 유스케이스 (신설 시)
// Repository 인터페이스는 Phase 0에서 정의된 시그니처를 그대로 사용한다고 가정
class OrderReleaseService {
public:
    OrderReleaseService(OrderRepository& orderRepository,
                         SampleRepository& sampleRepository);

    enum class ReleaseResult { Success, OrderNotFound, NotConfirmed, SampleNotFound };

    // 내부 처리: (1) order 조회/CONFIRMED 검증, (2) order.Release() 호출,
    // (3) sample 조회 후 sample.stock -= order.quantity 적용, (4) 두 Repository 모두 갱신
    ReleaseResult Release(const std::string& orderId);
};
```

> `Sample.stock`을 출고 시 차감하는 것은 Phase 3 문서(가용 재고 계산식)와의 정합을 위해
> **확정된 설계**다 — 선택지가 아니다. (Phase 3 문서 6.2절 "Phase 4에서 출고 시 물리적
> `stock -= quantity`를 확정하는 로직이 반드시 필요" 참고)

```cpp
// Controller 계층
class ShipmentController {
public:
    ShipmentController(OrderRepository& orderRepository,
                        SampleRepository& sampleRepository, ShipmentView& view);

    void Run(); // CONFIRMED 목록 표시 -> 선택 -> 출고 처리(OrderReleaseService 호출) -> 결과 안내
};
```

```cpp
// View 계층
class ShipmentView {
public:
    void ShowConfirmedOrders(const std::vector<Order>& orders);
    std::string PromptOrderSelection(); // 주문번호 또는 목록 인덱스 입력
    void ShowReleaseSuccess(const std::string& orderId);
    void ShowReleaseFailure(const std::string& reason);
    void ShowNoConfirmedOrders();
};
```

> 위 클래스/메서드 명은 예시이며, Phase 0~1~2~3에서 이미 확정된 네이밍 컨벤션(예: 기존
> Controller가 `~Controller`, View가 `~View`로 끝나는지, Repository 참조 방식이 참조자인지
> 포인터인지 등)이 있다면 그것을 우선한다. 이 문서의 시그니처는 뼈대일 뿐 구현 시 실제
> Phase 0~3 코드와 대조해 맞춘다.

JSON 저장 스키마 변경 없음 — `Order.status` 필드가 `"RELEASE"` 문자열 값을 갖게 되고,
`Sample.stock` 필드 값이 출고 수량만큼 감소해 저장될 뿐이며, 두 필드 모두 PRD 8.1/8.2에
이미 정의된 필드다(새 필드 추가 없음).

---

## 5. 의존 관계

**Phase 4가 전제로 삼는 이전 Phase 산출물**

- Phase 0: `Order`/`Sample` 도메인 모델, `OrderRepository`(JSON 파일 기반 CRUD), 상태 필터
  조회 기능(`FindByStatus` 또는 동등 기능 — 없다면 `FindAll()` 후 Controller/Model에서
  필터링해도 무방하나, Repository 계층에 필터 조회를 두는 편이 재사용성이 높다).
- Phase 2: `Order` 생성 및 `RESERVED` 상태 영속화 흐름.
- Phase 3: 승인 로직을 통해 `CONFIRMED` 상태로 전이된 주문이 존재. `Order` 상태 enum과
  `updatedAt` 갱신 컨벤션(승인 시 `updatedAt`을 어떻게 갱신했는지)을 그대로 재사용한다.
  또한 Phase 3의 가용 재고 계산식(`availableStock = sample.stock - Σ(CONFIRMED/PRODUCING
  주문 수량)`)은 "`CONFIRMED` 주문은 출고되기 전까지 재고가 배정된 상태이며, 출고 시
  물리적 `stock`에서 실제로 차감된다"는 전제를 갖는다(Phase 3 문서 3.2절/6.2절). Phase 4는
  이 전제를 그대로 이어받아 출고 시 `stock`을 차감하는 쪽으로 구현해야 한다.

**Phase 4 이후 Phase가 이 Phase에 기대하는 산출물**

- Phase 5(모니터링): `RELEASE` 상태 주문 건수 집계에 Phase 4의 출고 처리 결과가 정확히
  반영되어야 한다. Phase 4에서 상태 전이 시 `updatedAt`을 반드시 갱신해야 Phase 5의 "최근
  변경" 류 표시(있다면)와 일관성이 생긴다.
- Phase 7(통합 마무리): "시료 등록 → 주문 → 승인/생산 → 출고 → 모니터링" 전체 흐름
  테스트에서 출고 단계가 이 Phase의 Controller/View를 그대로 사용한다.

---

## 6. 핵심 비즈니스 로직 주의사항

Phase 4 자체에는 Phase 3 수준의 복잡한 계산 규칙은 없다. 다만 다음을 놓치지 않는다.

- **상태 전이 가드**: `CONFIRMED`가 아닌 주문(`RESERVED`, `PRODUCING`, `REJECTED`, 이미
  `RELEASE`)에 대해 출고를 시도하면 반드시 거부한다. PRD 6.3 상태 흐름도상 `RELEASE`로
  진입할 수 있는 경로는 오직 `CONFIRMED → RELEASE` 하나뿐이다.
- **재고 감소 원칙 (확정된 설계)**: 출고 처리는 물리적 `stock`을 **반드시 감소시킨다**
  (`sample.stock -= order.quantity`). PRD 7.4 "물리적 `stock`은 실제 생산 완료/출고
  시점에만 변경한다"는 문장은 재고 변경이 "생산 완료 시"와 "출고 시" **두 시점 모두**에서
  일어난다는 뜻이며, Phase 3의 가용 재고 계산식(`availableStock = sample.stock -
  Σ(CONFIRMED/PRODUCING 주문 수량)`)도 "`CONFIRMED` 주문은 출고 전까지 재고가 배정된
  상태로 취급하고, 출고되면 물리적 `stock`에서 실제로 빠진다"는 것을 전제로 성립한다
  (Phase 3 문서 3.2절/6.2절). 즉 재고 변경은 다음 두 이벤트로 확정된다.
  - 생산 완료 시: `stock += actualQuantity` (Phase 3 담당)
  - 출고(`RELEASE`) 처리 시: `stock -= order.quantity` (Phase 4 담당, 본 Phase)
  이 규칙이 지켜지지 않으면(즉 Phase 4가 `stock`을 그대로 두면), 이미 출고된 주문의 수량이
  Phase 3의 가용 재고 계산에서 영원히 "배정된 것"으로 남지 않아 물리적 재고가 실제보다 많이
  가용한 것처럼 잘못 계산되는 것이 아니라, 반대로 출고된 수량만큼 `stock`이 줄지 않아 실제
  보유량보다 부풀려진 채로 남는 오류가 발생한다. 두 계산(Phase 3의 가용 재고, Phase 4의
  물리적 재고 차감)이 서로 맞물려야 전체 재고 회계가 일치한다.
- **REJECTED/모니터링 무관**: 출고 처리는 `REJECTED` 주문과 무관하며 모니터링 집계 규칙
  변경도 유발하지 않는다.
- **멱등성/중복 출고 방지**: 동일 주문에 대해 출고 처리를 두 번 요청받아도(예: 같은 세션
  내 재입력) 두 번째 시도는 "이미 CONFIRMED 상태가 아님" 사유로 실패 처리되어야 한다.

---

## 7. 테스트 계획 (tester 에이전트용)

### 7.1 Model 단위 테스트 (`Order::Release`)

1. `CONFIRMED` 상태 주문에 `Release()` 호출 → 상태가 `RELEASE`로 바뀌고 `updatedAt`이
   갱신되는지 확인.
2. `RESERVED` 상태 주문에 `Release()` 호출 → 실패(false 반환 또는 예외) 및 상태 불변 확인.
3. `PRODUCING` 상태 주문에 `Release()` 호출 → 실패 확인.
4. `REJECTED` 상태 주문에 `Release()` 호출 → 실패 확인.
5. 이미 `RELEASE`인 주문에 다시 `Release()` 호출(중복 출고) → 실패 확인.

### 7.2 서비스/유스케이스 단위 테스트 (`OrderReleaseService`, GMock 활용)

6. `OrderRepository`를 mock으로 대체해, 존재하는 `CONFIRMED` 주문 ID로 호출 시
   `Update`/`Save`가 정확히 1회 호출되고 결과가 `Success`인지 검증.
7. 존재하지 않는 주문 ID로 호출 시 `OrderNotFound` 반환 및 Repository 갱신 메서드가
   호출되지 않는지 검증(`EXPECT_CALL(...).Times(0)`).
8. `CONFIRMED`가 아닌 주문 ID로 호출 시 `NotConfirmed` 반환 및 Repository 갱신 메서드
   미호출 검증.
9. 출고 성공 시 `Sample.stock`이 **정확히 `order.quantity`만큼 감소**했는지 검증하는 테스트
   추가 (6절 "재고 감소 원칙" 회귀 방지). 예: 출고 전 `stock=50`, `order.quantity=20`인
   `CONFIRMED` 주문을 출고 처리하면 출고 후 `stock == 30`이어야 한다.
10. 출고 실패 케이스(`OrderNotFound`, `NotConfirmed`, `SampleNotFound`)에서는 `Sample.stock`이
    **변경되지 않았음**을 검증(부분 실패로 인한 재고 불일치 방지).
11. 동일 주문을 두 번 출고 시도할 때(중복 출고 방지 케이스) 두 번째 시도에서 `stock`이
    추가로 차감되지 않는지 확인(멱등성 검증).

### 7.3 Controller/View 통합(시스템) 테스트

12. `CONFIRMED` 주문이 여러 건 있을 때 목록 표시 → 번호 선택 → 출고 성공 → 저장소에 반영된
    상태가 `RELEASE`이고 **해당 시료의 `stock`이 출고 수량만큼 감소해 저장됐는지**까지
    end-to-end로 확인 (JSON 파일 기반 Repository를 실제로 사용해 저장/재로드까지 검증하면
    Phase 0의 영속성 요구사항과도 정합).
13. `CONFIRMED` 주문이 하나도 없을 때 "출고 대상 없음" 안내가 출력되고 입력을 요구하지
    않는지 확인.
14. 잘못된 번호/주문번호 입력 시 오류 메시지 출력 후 정상적으로 메뉴로 복귀(또는 재입력)
    하는지 확인.
15. (Phase 7 선행 점검용) 시료 등록 → 주문 → 승인(재고 충분 케이스로 즉시 `CONFIRMED`) →
    출고까지 이어지는 전체 경로가 한 테스트에서 끊기지 않고 동작하는지 확인하며, 출고 후
    `Sample.stock`이 출고 수량만큼 줄어 있는지 함께 검증한다.

---

## 8. 완료 조건 (Definition of Done)

- [ ] `Order::Release()`(또는 동등 메서드)가 `CONFIRMED → RELEASE` 전이만 허용하고, 그 외
  상태에서는 안전하게 실패를 반환한다.
- [ ] Controller가 `CONFIRMED` 목록 조회 → 번호 선택 → 출고 처리 → 결과 안내의 전체 흐름을
  콘솔에서 실제로 수행한다(수동 실행 또는 시스템 테스트로 확인).
- [ ] View는 Model에 직접 의존하지 않고 Controller가 전달한 데이터만으로 화면을 구성한다
  (MVC 의존 방향 위반 없음).
- [ ] 출고 성공 시 물리적 `Sample.stock`이 `order.quantity`만큼 정확히 감소한다는 것이
  테스트로 확인된다(생산 완료 시 `stock`이 증가하는 Phase 3 로직과 대칭되는, 확정된 규칙).
- [ ] 출고 실패(주문 미존재, `CONFIRMED` 아님, 중복 출고 등) 시 `Sample.stock`이 변경되지
  않는다는 것이 테스트로 확인된다.
- [ ] 승인(Phase 3)부터 출고(Phase 4)까지 end-to-end 시나리오가 최소 1개 이상 시스템
  테스트로 검증된다(PLAN.md Phase 4 완료 기준).
- [ ] `code-review` 에이전트 검토 통과(Clean Code, 함수 라인 수, SOLID, 과설계 여부).
- [ ] `tester` 에이전트가 위 7절 테스트 시나리오를 GoogleTest/GMock으로 작성하고 실제 빌드·
  실행하여 전부 통과함을 보고한다.
- [ ] `supervisor` 에이전트가 code-review/tester 산출물을 CLAUDE.md/PRD.md/PLAN.md 및 실제
  Phase 0~3 문서/코드와 대조 검증하여 불일치가 없음을 확인한다.

---

## 9. 확인이 필요한 지점 (TODO 요약)

1. **네이밍 컨벤션**: `ShipmentController`/`ShipmentView`/`OrderReleaseService` 등은 가칭이며,
   Phase 0~3에서 이미 확정된 Controller/View/서비스 네이밍 규칙이 있다면 그것을 따른다.
2. **서비스 계층 도입 여부**: Phase 3가 승인 로직에 별도 서비스 클래스를 두었는지 여부에
   따라 Phase 4도 동일한 패턴(서비스 클래스 도입) 또는 더 단순한 패턴(Order/Sample 메서드 +
   Controller 조합)을 택해야 한다. 과설계를 피하기 위해 Phase 3 패턴을 그대로 따르는 것을
   권장. (재고 차감 시점 자체는 확정 사항이며 TODO가 아니다 — 8절 참고)
