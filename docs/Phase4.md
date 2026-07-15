# Phase 4 — 출고 처리

> 이 문서는 Phase 0~3의 실제 구현(코드 확인 완료, `feature/phase3-approval-production` 브랜치
> 기준)을 전제로 삼는다. 이전 초안은 Phase 0~3 문서를 아직 볼 수 없는 상태에서 작성돼
> `Order::Release()`라는 상태 전이 메서드가 새로 생긴다고 가정했으나, **실제 코드에는 그런
> 메서드가 없다.** `Model/Order.h`에는 여전히 `SetStatus(OrderStatus)`/
> `SetUpdatedAt(std::string)`라는 로우레벨 setter만 있고(Phase 3에서도 확인된 사실), 상태 전이
> 자체는 항상 Model 계층의 별도 유스케이스 서비스(`OrderService`, `OrderApprovalService`)가
> 이 setter들을 조합해 처리해왔다. Phase 4도 동일한 패턴을 따라 `OrderReleaseService`를 둔다.
>
> 핵심 확인 사항 요약:
> - `Order`: `SetStatus`/`SetUpdatedAt` 외 전이 메서드 없음 → 새 메서드를 추가하지 않는다.
> - `OrderApprovalService::Approve`/`Reject`는 `const Order&`를 받아 `order.GetStatus()`를
>   직접 검증하고, 위반 시 `std::invalid_argument`를 던진다. 대상 시료가 없으면
>   `std::runtime_error`를 던진다. `OrderReleaseService::Release`도 동일한 컨벤션을 따른다.
> - `AvailableStockCalculator.h`의 주석에 **"RELEASE orders are already deducted from physical
>   stock (Phase 4)"** 라고 이미 명시돼 있다 — 즉 "출고 시 물리적 `stock -= quantity`" 규칙은
>   Phase 3 코드에 이미 전제로 박혀 있는 확정 사항이며 Phase 4가 반드시 구현해야 한다.
> - 재고 증가(`ProductionLineController::HandleCompleteCurrentJob`)와 재고 차감(본 Phase)은
>   대칭 관계다. 다만 Phase 3의 생산 완료 처리는 **Controller가 직접**
>   `Sample.stock`/`Order.status`를 갱신하는 방식으로 구현되어 있고(별도 Model 서비스 없음),
>   반면 승인/거절은 **Model 계층 서비스**(`OrderApprovalService`)가 처리한다. 즉 이 코드베이스
>   안에 두 가지 선례가 공존한다. Phase 4는 상태 전이(도메인 규칙: "CONFIRMED만 출고 가능")가
>   있는 유스케이스이므로 `OrderService`/`OrderApprovalService`와 같은 **Model 서비스 계층**
>   패턴을 따르기로 확정한다(CLAUDE.md "View나 Controller에 도메인 로직... 섞이지 않도록" 원칙에
>   더 부합하며, 이미 두 곳에서 쓰인 지배적 패턴이기도 하다).

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

## 2. 전제 (Phase 0~3 실제 코드 확인 결과)

- `Model::Order` (`Model/Order.h`, `Model/OrderStatus.h`)
  - 필드: `orderId_`, `sampleId_`, `customerName_`, `quantity_`, `status_`(`OrderStatus`),
    `createdAt_`, `updatedAt_`.
  - `OrderStatus` = `{Reserved, Rejected, Producing, Confirmed, Release}`.
  - 전이 메서드 없음. `SetStatus(OrderStatus)`/`SetUpdatedAt(std::string)`만 존재. 상태를
    바꾸려면 **값을 복사한 뒤** 두 setter를 호출하고 `OrderRepository::Update(copy)`로
    영속화해야 한다(참조를 직접 수정해도 Repository 내부 벡터에는 반영되지 않음 —
    `JsonFileRepository<T>::Update`가 `GetId()`로 찾은 기존 원소를 통째로 대입하는 구조,
    `Model/JsonFileRepository.h` 36~44행).
  - `GetId()`/`GetOrderId()`는 동일 필드(`orderId_`)를 가리키는 별칭 accessor.
- `Model::OrderRepository : JsonFileRepository<Order>` — `FindAll()`, `FindById(id)`,
  `Add`, `Update`, `Remove`, `Save`, `Load`를 이미 제공(`Model/OrderRepository.h`, 기본 경로
  `data/orders.json`). Phase 4는 `FindAll()`(상태 필터는 Controller에서 수행)과 `Update`만
  사용한다. 별도의 `FindByStatus`는 없다 — Phase 3의 `OrderApprovalController`도 `FindAll()`
  후 직접 필터링하는 방식을 쓴다.
- `Model::Sample` (`Model/Sample.h`) — `GetStock()`/`SetStock(int)` 보유. 물리적 재고는 이미
  Phase 3에서 생산 완료 시 `stock += actualQuantity`로 증가시키는 선례가
  `Controller/ProductionLineController.cpp`(`HandleCompleteCurrentJob`)에 있다.
- `Model::SampleRepository : JsonFileRepository<Sample>` — `FindById`, `Update` 등 동일 CRUD
  제공(`Model/SampleRepository.h`).
- `Model::OrderApprovalService` (`Model/OrderApprovalService.h/.cpp`) — 이 Phase가 그대로
  본뜰 참조 패턴:
  ```cpp
  class OrderApprovalService {
  public:
      OrderApprovalService(SampleRepository& sampleRepository, OrderRepository& orderRepository);
      ApprovalResult Approve(const Order& order);   // status != Reserved -> throw invalid_argument
      void Reject(const Order& order);              // status != Reserved -> throw invalid_argument
  private:
      SampleRepository& sampleRepository_;
      OrderRepository& orderRepository_;
  };
  ```
  - 생성자에서 두 Repository를 참조로 주입받는다.
  - 전이 전 `order.GetStatus()`를 직접 검증하고 위반 시 예외를 던진다(정상 UI 흐름에서는
    Controller가 이미 올바른 상태의 주문만 넘기므로, 예외는 "호출 계약 위반"을 뜻함).
  - 대상 `Sample`이 없으면 `std::runtime_error`.
  - 값 복사 → `SetStatus`/`SetUpdatedAt(Common::CurrentTimestampIso8601())` → `Update(copy)`.
- `Controller::OrderApprovalController`/`Controller::ProductionLineController` —
  `Controller::ISubMenuController`(`void Run() override`)를 구현하는 방식이 확정 컨벤션.
  `Controller::MainMenuController`는 `std::map<std::string,
  std::reference_wrapper<ISubMenuController>>` 기반 라우팅이며 이미 일반화돼 있어 새 Controller
  추가 시 그 자체를 수정할 필요가 없다(`Controller/MainMenuController.h` 13~26행,
  `MainMenuOption::Release = 6` 이미 정의됨, `ToMenuOptionKey`로 문자열 키 변환).
- `main.cpp`의 DI 배선 방식: Repository들을 만들고 `Load()` 호출 → 각 서비스/뷰/컨트롤러를
  생성자에서 참조로 엮음 → `MainMenuController`에 `{ToMenuOptionKey(...), controller}` 맵 항목
  추가. Phase 4는 `Model::OrderReleaseService orderReleaseService(sampleRepository,
  orderRepository);`, `View::ReleaseView releaseView;`, `Controller::ReleaseController
  releaseController(orderReleaseService, orderRepository, releaseView);`를 만들고
  `{ToMenuOptionKey(MainMenuOption::Release), releaseController}`를 맵에 추가하기만 하면 된다
  (현재 main.cpp에는 아직 `MainMenuOption::Release`("6") 항목이 등록돼 있지 않음 — Phase 4가
  채워야 할 자리).
- `View` 계층 컨벤션(`View/OrderApprovalView.h`, `View/ProductionLineView.h`): 순수 입출력만
  담당하는 클래스, 모든 메서드가 `virtual ... const`(GMock으로 Controller를 목 View 기반 단위
  테스트하기 위함), 계산된 값(예: progress)은 Controller가 미리 계산해 넘긴 것을 그대로 출력.

---

## 3. 작업 항목 체크리스트

### 3.1 Model

- [ ] `Order`/`Sample`에 새 메서드를 추가하지 않는다(기존 `SetStatus`/`SetUpdatedAt`,
  `SetStock`/`GetStock`만으로 충분).
- [ ] `Model::OrderReleaseService` 신설 (`Model/OrderReleaseService.h/.cpp`,
  `OrderApprovalService`와 동일한 생성자/예외 컨벤션):
  - 생성자: `OrderReleaseService(SampleRepository& sampleRepository, OrderRepository& orderRepository)`
  - `void Release(const Order& order)`:
    1. `order.GetStatus() != OrderStatus::Confirmed` → `throw std::invalid_argument(...)`
       ("Release() requires a CONFIRMED order" 등, `Approve`/`Reject`의 메시지 스타일과 통일).
    2. `sampleRepository_.FindById(order.GetSampleId())`가 없으면
       `throw std::runtime_error(...)` ("Release() called for an order whose sample no longer
       exists" 스타일).
    3. `Sample` 복사본의 `stock`을 `order.GetQuantity()`만큼 감소시켜
       `sampleRepository_.Update(updatedSample)`로 저장. (음수 클램프는 하지 않는다 — 정상
       흐름에서는 `CONFIRMED`가 되는 시점에 이미 그 수량만큼 재고가 확보돼 있었어야 하므로
       음수가 나오는 것 자체가 데이터 불일치 신호다. 아래 6절 "주의사항" 참고.)
    4. `Order` 복사본에 `SetStatus(OrderStatus::Release)` + `SetUpdatedAt(now)` 적용 후
       `orderRepository_.Update(updatedOrder)`로 저장.
    5. `Sample` 갱신을 먼저, `Order` 갱신을 나중에 하든 순서 자체가 도메인적으로 중요하진
       않지만(둘 다 성공/실패가 사실상 원자적이라고 가정 — 별도 트랜잭션 롤백은 이 프로젝트
       범위 밖), 코드 리뷰 시 순서를 하나로 통일해 일관성을 유지한다.
  - `OrderApprovalService::Approve`가 `ApprovalResult`를 반환하는 것과 달리, `Release`는
    성공 시 알려줄 부가 정보(생성된 `ProductionJob` 같은 것)가 없으므로 `Reject`처럼 `void`
    반환으로 충분하다(과설계 방지).

### 3.2 Controller

- [ ] `Controller::ReleaseController : ISubMenuController` 신설 (`Controller/ReleaseController.h/.cpp`).
  - 생성자: `ReleaseController(Model::OrderReleaseService& releaseService, Model::OrderRepository& orderRepository, View::ReleaseView& view)`.
  - `void Run() override` 흐름:
    1. `orderRepository_.FindAll()`에서 `status == OrderStatus::Confirmed`인 주문만 추려
       (Phase 3의 `OrderApprovalController.cpp`에 있는 `ReservedOrdersSortedByOrderId`와
       동일한 형태의 익명 네임스페이스 헬퍼 `ConfirmedOrdersSortedByOrderId`를 만들어 `orderId`
       오름차순 정렬 — 정렬은 순서 강제를 위해서가 아니라 화면에 안정적인 순서로 보여주기
       위함일 뿐, 승인처럼 "가장 오래된 것만 처리 가능"하도록 강제할 필요는 없다. 아래 5절
       참고).
    2. 비어 있으면 `view_.ShowNoConfirmedOrders()` 후 리턴.
    3. `view_.ShowConfirmedOrders(confirmed)`로 목록 출력(1-based 번호를 화면에 매겨 보여줌).
    4. `int selection = view_.PromptOrderSelection();` (1-based 인덱스, 범위를 벗어나거나
       0/음수면 취소/무효로 처리).
    5. `selection`이 `[1, confirmed.size()]` 범위를 벗어나면 `view_.ShowInvalidSelection()` 후
       리턴.
    6. `const Order& selected = confirmed[selection - 1];`
    7. `releaseService_.Release(selected)` 호출. 정상 흐름에서는 목록 자체가 이미 `CONFIRMED`
       필터를 거쳤으므로 예외가 나올 일이 없지만(호출 계약 위반은 곧 버그), **인덱스 선택 UX
       특성상** 화면에 표시된 목록과 실제 처리 시점 사이에 데이터가 바뀔 가능성을 완전히
       배제할 수 없으므로(예: 동일 세션 내 리스트를 다시 안 그리는 구조가 아니라면 사실상
       발생하지 않지만, 방어적으로) `try { ... } catch (const std::exception& e) {
       view_.ShowReleaseFailure(e.what()); return; }`로 감싼다. (`OrderApprovalController`가
       예외를 잡지 않는 것과 달리 여기서 잡는 이유: 승인/거절은 컨트롤러가 고른 주문의 상태를
       스스로 보장하는 단일 대상 흐름이지만, 출고는 처음 도입되는 "번호로 여러 항목 중 선택"
       UX이므로 방어적 처리를 추가한다.)
    8. 성공 시 `view_.ShowReleaseSuccess(selected.GetOrderId())`.
- [ ] `main.cpp`에 `Model::OrderReleaseService`/`View::ReleaseView`/`Controller::ReleaseController`
  생성 및 `MainMenuController` 맵에 `{ToMenuOptionKey(MainMenuOption::Release), releaseController}`
  등록 (PRD 7.1 메뉴표의 "출고 처리" 항목, 기존에 정의된 `MainMenuOption::Release = 6`을 그대로
  사용 — 새 enum 값을 만들 필요 없음).

### 3.3 View

- [ ] `View::ReleaseView` 신설 (`View/ReleaseView.h/.cpp`), `OrderApprovalView`/
  `ProductionLineView`와 동일한 컨벤션(모든 메서드 `virtual ... const`, 가상 소멸자, Model 직접
  참조 없이 `Model::Order` 데이터만 인자로 받음):
  - `virtual void ShowConfirmedOrders(const std::vector<Model::Order>& orders) const;` — 1-based
    번호와 함께 주문번호/시료ID/고객명/수량/상태를 표 형태로 출력(Phase 1 `SampleView::ShowSampleList`
    스타일 참고).
  - `virtual void ShowNoConfirmedOrders() const;`
  - `virtual int PromptOrderSelection() const;` — 1-based 번호 입력(형식 오류/범위 밖은 Controller가
    판단할 수 있도록 정수만 파싱해 반환하거나, 파싱 실패 시 0 등 "무효" 값을 반환).
  - `virtual void ShowReleaseSuccess(const std::string& orderId) const;`
  - `virtual void ShowReleaseFailure(const std::string& reason) const;`
  - `virtual void ShowInvalidSelection() const;`

---

## 4. 인터페이스/데이터 스키마

PRD 8.2 `Order` 스키마에 새 필드를 추가하지 않는다. 기존 `status` 필드가 `"CONFIRMED"`에서
`"RELEASE"` 문자열 값으로 바뀌고 `updatedAt`이 갱신될 뿐이다. PRD 8.1 `Sample` 스키마도 새 필드
없이 `stock` 값만 감소한다.

```cpp
// Model/OrderReleaseService.h (신설)
#pragma once

#include "Model/Order.h"
#include "Model/OrderRepository.h"
#include "Model/SampleRepository.h"

namespace Model {

// PRD 7.7's release use-case: validates the order is CONFIRMED, deducts the
// physical Sample::stock by order.quantity (the symmetric counterpart to
// ProductionLineController's stock += actualQuantity on production
// completion — see AvailableStockCalculator.h's "RELEASE orders are already
// deducted from physical stock" precondition), and persists the Order's new
// RELEASE status. Same throw-on-contract-violation convention as
// OrderApprovalService::Approve/Reject.
class OrderReleaseService {
public:
    OrderReleaseService(SampleRepository& sampleRepository, OrderRepository& orderRepository);

    void Release(const Order& order);

private:
    SampleRepository& sampleRepository_;
    OrderRepository& orderRepository_;
};

}  // namespace Model
```

```cpp
// Controller/ReleaseController.h (신설)
#pragma once

#include "Controller/ISubMenuController.h"
#include "Model/OrderReleaseService.h"
#include "Model/OrderRepository.h"
#include "View/ReleaseView.h"

namespace Controller {

// PRD 7.7 shipment/release — unlike OrderApprovalController's forced
// "always the oldest" single-shot pattern, release has no ordering
// constraint (releasing one CONFIRMED order never changes another's
// available-stock calculation — that calculation only cares about
// CONFIRMED/PRODUCING quantities, not release order), so the user freely
// picks any CONFIRMED order by list number (PRD 10 "번호 선택").
class ReleaseController : public ISubMenuController {
public:
    ReleaseController(Model::OrderReleaseService& releaseService, Model::OrderRepository& orderRepository,
                      View::ReleaseView& view);

    void Run() override;

private:
    Model::OrderReleaseService& releaseService_;
    Model::OrderRepository& orderRepository_;
    View::ReleaseView& view_;
};

}  // namespace Controller
```

```cpp
// View/ReleaseView.h (신설)
#pragma once

#include <string>
#include <vector>

#include "Model/Order.h"

namespace View {

class ReleaseView {
public:
    virtual ~ReleaseView() = default;

    virtual void ShowConfirmedOrders(const std::vector<Model::Order>& orders) const;
    virtual void ShowNoConfirmedOrders() const;
    virtual int PromptOrderSelection() const;  // 1-based index; out-of-range means invalid
    virtual void ShowReleaseSuccess(const std::string& orderId) const;
    virtual void ShowReleaseFailure(const std::string& reason) const;
    virtual void ShowInvalidSelection() const;
};

}  // namespace View
```

JSON 저장 스키마 변경 없음. `data/orders.json`의 해당 주문 레코드가 `"status": "RELEASE"`로,
`data/samples.json`의 해당 시료 레코드가 감소된 `"stock"` 값으로 저장될 뿐이다(둘 다 PRD 8.1/8.2에
이미 정의된 필드, 새 필드 추가 없음).

---

## 5. 의존 관계

**Phase 4가 전제로 삼는 이전 Phase 산출물**

- Phase 0: `Order`/`Sample` 도메인 모델과 `JsonFileRepository<T>` 기반 `OrderRepository`/
  `SampleRepository`(`FindAll`/`FindById`/`Update`/`Save`/`Load`).
- Phase 2: `Order` 생성(`OrderService::PlaceOrder`) 및 `RESERVED` 상태 영속화.
- Phase 3: `OrderApprovalService`가 만든 `CONFIRMED` 상태 주문(재고 충분 즉시 확정 케이스,
  또는 `ProductionLineController::HandleCompleteCurrentJob`을 거쳐 `PRODUCING → CONFIRMED`로
  전환된 케이스) — Phase 4의 출고 대상 목록의 유일한 출처. 또한 Phase 3의 가용 재고 계산식
  (`CalculateAvailableStock` = `sample.stock - Σ(CONFIRMED/PRODUCING 주문 수량)`)은 "`CONFIRMED`
  주문은 출고되기 전까지 재고가 배정된 상태이며, 출고 시 물리적 `stock`에서 실제로 빠진다"는
  전제를 갖는다(`Model/AvailableStockCalculator.h`의 "RELEASE orders are already deducted from
  physical stock (Phase 4)" 주석이 이 전제를 코드에 이미 못박아 두었다). Phase 4는 이 전제를
  그대로 이행해야 한다 — 구현하지 않으면 Phase 3의 가용 재고 계산이 향후 신규 주문에 대해 이미
  출고된 수량만큼 재고를 계속 "배정된 것"으로 착각하게 되어, 실제로는 재고가 없는데도
  `availableStock`이 실제보다 낮게(더 보수적으로) 계산되는 것이 아니라 반대로 물리적 `stock`이
  실제 보유량보다 부풀려진 채로 남아 시스템 전체 재고 회계가 어긋난다.
- `Controller::ISubMenuController`/`Controller::MainMenuController`(map 기반 라우팅,
  `MainMenuOption::Release = 6` 이미 정의)와 `main.cpp`의 DI 배선 패턴을 그대로 재사용.

**Phase 4 이후 Phase가 이 Phase에 기대하는 산출물**

- Phase 5(모니터링): `RELEASE` 상태 주문 건수 집계가 정확해지려면 Phase 4가 상태 전이를 실제로
  수행해야 한다. 재고 현황 표기는 물리적 `stock`(출고로 감소된 값 포함)을 그대로 사용한다 —
  가용 재고와 혼동 금지(CLAUDE.md).
- Phase 6(더미 데이터 생성기): `RELEASE` 상태의 더미 `Order`를 직접 주입하는 경우, 대응하는
  `Sample.stock`도 이미 감소된 값으로 함께 생성해야 회계가 맞는다는 점을 더미 데이터 생성기
  문서에서 다뤄야 한다(Phase 4가 만드는 "CONFIRMED → RELEASE 시 stock 차감" 불변식을 더미
  데이터가 우회해서 깨지 않도록).
- Phase 7(통합 마무리): "시료 등록 → 주문 → 승인/생산 → 출고 → 모니터링" end-to-end 테스트에서
  출고 단계가 본 Phase의 `ReleaseController`/`ReleaseView`를 그대로 사용한다.

---

## 6. 핵심 비즈니스 로직 주의사항

- **상태 전이 가드**: `CONFIRMED`가 아닌 주문(`RESERVED`, `PRODUCING`, `REJECTED`, 이미
  `RELEASE`)에 대한 출고 시도는 `OrderReleaseService::Release`가 `std::invalid_argument`로
  거부한다. PRD 6.3 상태 흐름도상 `RELEASE`로 진입하는 경로는 `CONFIRMED → RELEASE` 하나뿐이다.
- **재고 감소는 확정된 설계** (`sample.stock -= order.quantity`): PRD 7.4 "물리적 `stock`은
  실제 생산 완료/출고 시점에만 변경한다"는 문장은 재고 변경이 "생산 완료 시"와 "출고 시" 두
  시점 모두에서 일어난다는 뜻이며, `Model/AvailableStockCalculator.h`에 이미 이 전제가 주석으로
  박혀 있다. 정리하면:
  - 생산 완료 시: `stock += actualQuantity` (Phase 3, `ProductionLineController` 담당)
  - 출고(`RELEASE`) 처리 시: `stock -= order.quantity` (Phase 4, 본 Phase 담당)
- **음수 재고에 대한 처리 방침**: `order.quantity`만큼 차감했을 때 `stock`이 음수가 될 수
  있는가? 정상 흐름에서는 불가능해야 한다 — `CONFIRMED`가 된 시점(즉시 확정이든 생산 완료를
  거쳤든)에 이미 그 주문 수량만큼의 재고가 물리적으로 확보돼 있었어야 하기 때문이다. 다만 이
  불변식이 100% 보장된다고 가정하지 않고, **차감 결과가 음수여도 클램프하지 않고 그대로
  저장**하기로 한다(데이터 불일치를 조용히 감추지 않고 드러나게 하려는 의도). 이 부분은 TODO:
  확인 필요 — PRD/CLAUDE.md에 명시적 규정이 없으므로, 만약 운영상 음수 재고를 허용하지 않아야
  한다는 요구가 있다면 사용자 확인 후 클램프 로직을 추가한다.
- **REJECTED/모니터링 무관**: 출고 처리는 `REJECTED` 주문과 무관하며 모니터링 집계 규칙 변경도
  유발하지 않는다.
- **멱등성/중복 출고 방지**: 동일 주문에 대해 출고 처리를 두 번 시도해도(예: 같은 세션에서
  이미 `RELEASE`가 된 주문을 다시 선택), 목록 자체가 매번 `CONFIRMED`만 필터링해 다시 그려지므로
  이미 `RELEASE`된 주문은 애초에 목록에 나타나지 않는다. 그럼에도 `OrderReleaseService::Release`
  내부의 상태 검증(`status != Confirmed → throw`)이 최후 방어선 역할을 한다.
- **승인과 달리 순서 강제가 없다**: Phase 3의 승인은 "승인 시점 가용 재고"가 접수 순서에
  의존하므로 임의 순서 처리를 금지해야 했다(강제로 가장 오래된 `RESERVED`만 처리). 반면 출고는
  이미 확정된 `CONFIRMED` 주문의 물리적 재고를 실제로 빼는 것뿐이라 어떤 순서로 처리해도 다른
  `CONFIRMED`/`PRODUCING` 주문의 가용 재고 계산 결과에 영향을 주지 않는다(그 계산은 이미
  "출고 여부와 무관하게 CONFIRMED/PRODUCING이면 배정된 것"으로 취급하기 때문). 따라서 Phase 4는
  PRD 10장이 요구하는 "번호 선택" UX(자유 선택)를 그대로 구현해도 되며, `OrderApprovalController`
  처럼 "항상 첫 번째만" 강제할 필요가 없다.

---

## 7. 테스트 계획 (tester 에이전트용)

> 실행 방식(Phase 0~3에서 확정): 별도 테스트 프로젝트 없이 `Tests/*.cpp`에 GoogleTest/GMock
> 테스트를 추가하면 `SampleOrderSystem.vcxproj`가 Debug 빌드에서만 조건부 컴파일한다.
> `SampleOrderSystem.exe --test`로 실행하면 콘솔 앱 대신 `RUN_ALL_TESTS()`가 실행된다(Release
> 빌드는 테스트 소스를 포함하지 않음). Phase 4 테스트도 이 방식을 그대로 따른다.

### 7.1 `OrderReleaseService::Release` 단위 테스트

1. `CONFIRMED` 주문에 `Release(order)` 호출 → 저장된 `Order`의 `status == RELEASE`,
   `updatedAt`이 호출 전보다 갱신됨을 확인.
2. 위 1번과 같은 케이스에서 `Sample.stock`이 정확히 `order.quantity`만큼 감소해 저장됨을
   확인(예: 출고 전 `stock=50`, `order.quantity=20` → 출고 후 `stock=30`).
3. `RESERVED` 상태 주문에 `Release` 호출 → `std::invalid_argument` 던짐, `Order`/`Sample`
   모두 Repository에 `Update`가 호출되지 않음(GMock `EXPECT_CALL(...).Times(0)`으로 검증).
4. `PRODUCING` 상태 주문에 `Release` 호출 → 3번과 동일하게 예외 및 미변경 확인.
5. `REJECTED` 상태 주문에 `Release` 호출 → 3번과 동일.
6. 이미 `RELEASE`인 주문에 다시 `Release` 호출(중복 출고) → 예외 및 미변경 확인.
7. `order.sampleId`에 해당하는 `Sample`이 `SampleRepository`에 없는 경우 → `std::runtime_error`
   던짐, `Order`도 갱신되지 않음(부분 실패로 인한 상태 불일치 방지).
8. (경계값) 출고 후 `stock`이 정확히 0이 되는 케이스(재고를 모두 소진하는 출고) → 정상 처리되고
   `stock == 0`.

### 7.2 `ReleaseController`/`ReleaseView` 통합(GMock View) 테스트

9. `CONFIRMED` 주문이 여러 건 있을 때: `ShowConfirmedOrders`가 정확한 목록으로 1회 호출되고,
   `PromptOrderSelection`이 반환한 인덱스에 해당하는 주문에 대해서만
   `OrderReleaseService::Release`(또는 이를 감싼 실제 Repository 갱신)가 수행되는지 확인.
10. `CONFIRMED` 주문이 하나도 없을 때: `ShowNoConfirmedOrders`만 호출되고 `PromptOrderSelection`은
    호출되지 않는지 확인.
11. 범위를 벗어난 인덱스(0, 음수, 목록 크기 초과) 입력 시 `ShowInvalidSelection`이 호출되고
    Repository 갱신이 발생하지 않는지 확인.
12. (방어적 처리 검증) `Release`가 예외를 던지는 상황을 강제로 만들었을 때
    `ShowReleaseFailure`가 호출되고 컨트롤러가 예외를 삼켜 앱이 죽지 않는지 확인.

### 7.3 시스템(end-to-end) 테스트

13. 실제 JSON 파일 기반 Repository를 사용해: `CONFIRMED` 주문 목록 표시 → 번호 선택 → 출고
    성공 → 파일에 저장된 `Order.status == "RELEASE"`이고 해당 `Sample.stock`이 출고 수량만큼
    감소해 저장돼 있는지 재로드(`Load()`)까지 포함해 확인(Phase 0 영속성 요구사항과 정합).
14. (PLAN.md Phase 4 완료 기준) 시료 등록 → 주문 접수 → 승인(재고 충분 케이스로 즉시
    `CONFIRMED`) → 출고까지 이어지는 전체 경로가 하나의 테스트에서 끊기지 않고 동작하며, 출고
    후 `Sample.stock`이 출고 수량만큼 줄어 있는지 확인.
15. 승인(부족 케이스, Phase 3) → 생산 완료 처리(`PRODUCING → CONFIRMED`, `stock` 증가) → 출고
    (`stock` 감소)까지 이어지는 전체 경로에서 최종 `stock`이 "초기 재고 + actualQuantity -
    order.quantity"와 일치하는지 확인(Phase 3와 Phase 4의 재고 증감이 정확히 대칭인지에 대한
    회귀 테스트).

---

## 8. 완료 조건 (Definition of Done)

- [ ] `Model::OrderReleaseService::Release`가 `CONFIRMED → RELEASE` 전이만 허용하고, 그 외
  상태에서는 예외를 던져 안전하게 거부한다.
- [ ] 출고 성공 시 물리적 `Sample.stock`이 `order.quantity`만큼 정확히 감소한다는 것이 테스트로
  확인된다(생산 완료 시 `stock`이 증가하는 Phase 3 로직과 대칭되는, 확정된 규칙).
- [ ] 출고 실패(주문 미존재/`CONFIRMED` 아님/중복 출고/시료 미존재) 시 `Order`/`Sample` 모두
  변경되지 않는다는 것이 테스트로 확인된다.
- [ ] `Controller::ReleaseController`가 `CONFIRMED` 목록 조회 → 번호 선택(자유 선택, 강제 순서
  없음) → 출고 처리 → 결과 안내의 전체 흐름을 콘솔에서 실제로 수행한다.
- [ ] `View::ReleaseView`는 Model에 직접 의존하지 않고 Controller가 전달한 데이터만으로 화면을
  구성한다(MVC 의존 방향 위반 없음).
- [ ] `main.cpp`에 `MainMenuOption::Release`("6") 라우팅이 실제로 연결된다.
- [ ] 승인(Phase 3)부터 출고(Phase 4)까지 end-to-end 시나리오가 최소 1개 이상 시스템 테스트로
  검증된다(PLAN.md Phase 4 완료 기준).
- [ ] `code-review` 에이전트 검토 통과(Clean Code, 함수 라인 수, SOLID, 과설계 여부 — 특히
  `Release`가 `void`로 충분한지, 불필요한 결과 enum을 추가하지 않았는지).
- [ ] `tester` 에이전트가 7절 테스트 시나리오를 GoogleTest/GMock으로 작성하고 실제 빌드·실행하여
  전부 통과함을 보고한다.
- [ ] `supervisor` 에이전트가 code-review/tester 산출물을 CLAUDE.md/PRD.md/PLAN.md 및 실제
  Phase 0~3 코드와 대조 검증하여 불일치가 없음을 확인한다.

---

## 9. 확인이 필요한 지점 (TODO 요약)

1. **음수 재고 클램프 여부**: 6절에서 설명한 대로, 정상 흐름에서는 발생하지 않아야 하지만
   방어적으로 클램프할지(예: `std::max(stock - quantity, 0)`) 여부는 PRD/CLAUDE.md에 명시가
   없어 확정하지 못했다. 우선은 클램프하지 않고 그대로 반영하는 쪽으로 계획했으며, 구현 중
   실제로 이 케이스가 발생할 가능성이 있다고 판단되면 사용자에게 확인 후 정책을 정한다.
2. **`ReleaseController`의 예외 방어(try/catch) 필요성**: 5.2절에서 다른 승인/거절 컨트롤러와
   달리 출고 컨트롤러에만 방어적 `try/catch`를 추가하기로 했는데, 이것이 과설계인지(정상
   흐름에서는 절대 예외가 나지 않으므로 불필요하다고 볼 여지도 있음) `code-review`에서 재검토
   되어야 한다.
