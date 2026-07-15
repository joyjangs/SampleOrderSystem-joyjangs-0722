# Phase 7 — 통합 마무리

> 이 문서는 이전 초안(Phase 0~4 구현 이전, PLAN.md 서술만으로 작성된 추정 문서)을 실제 코드
> 기준으로 재작성한 것이다. 재작성 시점(phase-planner 실행 시점)에는 Phase 6이 아직
> 설계(`docs/Phase6.md`)만 확정되고 구현 코드가 없는 상태였으나, **그 이후 Phase 6 구현이
> 완료되어 `origin/master`에 병합되었다** (PR #15/#16, `feature/phase6-dummy-data-generator`).
> 아울러 매직넘버를 enum으로 바꾸는 `refactor/menu-enum-consistency`(PR #14)도 병합됐고,
> `fix/production-line-auto-complete`(생산 완료를 수동 버튼 대신 경과 시간 기반 자동 처리로
> 전환)가 아직 병합 대기 중이다. 아래 본문은 원본 phase-planner 산출물을 그대로 유지하되, 이
> preamble로 최신 상태를 보완한다.
>
> - `origin/master`: Phase 0~6이 **모두 코드까지 병합 완료**된 상태. `Model/OrderApprovalService`,
>   `Model/ProductionLine`, `Model/OrderReleaseService`, `Model/MonitoringService`,
>   `Model/DummyDataGenerator`, 그리고 대응하는 Controller/View가 전부 존재하고
>   `SampleOrderSystem.vcxproj`에도 등록돼 있으며, `main.cpp`도 메뉴 1~7을 전부 배선했다.
> - **아직 미병합**: `fix/production-line-auto-complete` — `ProductionLine::CompleteFrontJobIfDue`
>   로 생산 완료를 경과 시간 기반 자동 처리로 전환하고 수동 "생산 완료 처리" 메뉴를 제거한
>   변경. Phase 7 착수 전 이 PR의 병합 여부를 먼저 확인한다.
>
> **가장 중요한 발견 — Phase 3~6의 프로덕션 코드는 병합되어 있지만, 그 코드를 검증하는
> 테스트는 `origin/master`에 사실상 존재하지 않는다.** `SampleOrderSystem.vcxproj`의 Debug 전용
> `Tests\*.cpp` 목록은 아래 9개뿐이다.
>
> ```
> Tests\ModelSerializationTests.cpp
> Tests\RepositoryTests.cpp
> Tests\ControllerTests.cpp
> Tests\SampleControllerTests.cpp
> Tests\SampleManagementSystemTests.cpp
> Tests\OrderTests.cpp
> Tests\OrderServiceTests.cpp
> Tests\OrderControllerTests.cpp
> Tests\OrderManagementSystemTests.cpp
> ```
>
> 이 중 어떤 파일에도 `OrderApprovalService::Approve`/`Reject`, `Model::ProductionLine`,
> `Model::ComputeProgress`, `Controller::OrderApprovalController`,
> `Controller::ProductionLineController`, `Model::OrderReleaseService::Release`,
> `Controller::ReleaseController`, `Model::MonitoringService`, `Controller::MonitoringController`,
> `Model::DummyDataGenerator`, `Controller::DummyDataController`를 대상으로 한 테스트가 **단 한
> 건도 없다**. `RepositoryTests.cpp`/`ModelSerializationTests.cpp`가 `ProductionJob`의 JSON
> 직렬화·Repository CRUD는 다루지만, 이는 Phase 0/2 수준의 저장소 검증일 뿐 Phase 3의 **승인
> 시점 가용 재고 계산이나 PRD 7.4 예시1·예시2 자체는 전혀 검증하지 않는다.**
>
> 즉 Phase 3~6 각 문서의 완료 조건(DoD)에는 "`tester` 에이전트가 표에 정리된 테스트를 작성해
> 통과시킨다"는 항목이 체크리스트로 있었지만, 실제 `origin/master`에는 그 산출물이 반영되어
> 있지 않다(세션 방침 — CLAUDE.md 관련 사용자 메모리 "tester/supervisor 호출 임시 보류: 모든
> Phase 구현이 끝날 때까지 code-review만 진행하고 tester+supervisor는 한 번에 수행" — 때문에
> 의도적으로 미뤄진 것). **Phase 7의 최우선 임무는 이 공백을 메우는 것**이다. 이 문서의 6절
> 테스트 계획은 이 사실을 전제로, Phase 3~6 전체에 걸쳐 "지금까지 한 번도 실행되지 않은"
> 테스트를 어떤 범위로 새로 작성해야 하는지를 우선순위와 함께 구체적으로 나열한다.

## 0. Phase 7의 성격 — 다른 Phase/에이전트와의 역할 구분

- CLAUDE.md의 기본 흐름은 "매 Phase 종료 시 code-review → tester → (직후) supervisor"이지만,
  이 프로젝트는 세션 방침으로 "Phase 3~6 구현이 모두 끝날 때까지 tester/supervisor 호출을
  보류하고 code-review만 수행한 뒤, 마지막에 한 번에 tester+supervisor를 수행"하기로 되어
  있었다. Phase 6까지 구현이 끝난 지금이 바로 그 시점이다.
- 따라서 Phase 7은 단순히 "전체 재검증"이 아니라, Phase 3~6에 대해 **최초로** 체계적인
  테스트를 작성·실행하는 단계가 된다.
- Phase 7 안에서도 순서는 다음을 따른다.
  1. `fix/production-line-auto-complete` PR 병합 여부 확인
  2. `tester` 에이전트를 호출해 Phase 3~6 전체를 대상으로 한 번에 테스트 작성·빌드·실행
     (6절 체크리스트 전체)
  3. `code-review` 에이전트를 "전체 diff" 범위로 호출해 Phase 0~6 전체에 걸친 교차 이슈 점검
  4. 위 2·3 각각 직후 `supervisor` 호출 (CLAUDE.md 기본 흐름 준수)
  5. 문서-구현 정합성 확인, 커밋 이력 정리

## 1. 목표 요약 (PLAN.md 원문 인용)

**목표**: 전체 흐름 검증 및 문서 정리.

- 전체 시스템 테스트(시료 등록 → 주문 → 승인/생산 → 출고 → 모니터링) 재실행
- `code-review` 에이전트로 전체 diff 최종 점검
- `CLAUDE.md`/`docs/PRD.md`에 구현 중 확정된 세부사항 반영 여부 확인
- 커밋 이력 정리 (PRD 9.3 "의미 있는 Commit 이력 관리")

**완료 기준**: 전체 빌드/테스트 통과, 문서와 실제 구현 간 불일치 없음.

## 2. 작업 항목 체크리스트

### 2.1 전제 확인

- [x] Phase 6 구현이 `docs/Phase6.md` 설계대로 완료되고 병합됨(PR #15/#16).
- [ ] `fix/production-line-auto-complete`가 병합되었는지 확인. 병합 전이면 그 상태 그대로
      테스트 작성 대상에 포함(생산 완료가 수동 버튼 방식인지 자동 방식인지에 따라
      `ProductionLineControllerTests.cpp`의 시나리오가 달라짐).

### 2.2 전체 시스템 테스트 작성/재실행 (Model/View/Controller 계층별, tester 담당)

**Model 계층**

- [ ] `Tests/OrderApprovalServiceTests.cpp`(신설) — `OrderApprovalService::Approve`/`Reject`,
      `Model::CalculateAvailableStock`, PRD 7.4 예시1·예시2(5절 참고) 단위 테스트
- [ ] `Tests/ProductionLineTests.cpp`(신설) — `ProductionLine::Enqueue`/`Peek`/`ListQueue`/
      `CompleteFrontJobIfDue`, `Model::ComputeProgress` 단위 테스트
- [ ] `Tests/OrderReleaseServiceTests.cpp`(신설) — `OrderReleaseService::Release` 정상/예외
      케이스, `Sample.stock` 차감 검증
- [ ] `Tests/MonitoringServiceTests.cpp`(신설) — `GetOrderStatusSummary`/`GetSampleStockViews`/
      `GetMainMenuSummary`, `REJECTED` 제외, 물리적 재고만 사용하는지(`CalculateAvailableStock`
      비호출) 검증
- [ ] `Tests/DummyDataGeneratorTests.cpp`(신설, `docs/Phase6.md` 7.1절 시나리오)
- [ ] 위 신규 테스트 파일들을 `SampleOrderSystem.vcxproj`의 Debug 전용 `Tests\*.cpp`
      `ClCompile` 목록에 실제로 추가한다(빠뜨리면 빌드는 되지만 테스트가 아예 실행되지 않는
      상태로 조용히 방치될 수 있음).

**Controller 계층 (GMock 기반)**

- [ ] `Tests/OrderApprovalControllerTests.cpp`(신설) — `RESERVED` 목록 표시/승인·거절 라우팅,
      "항상 접수 순서상 첫 번째만 처리" 강제 여부
- [ ] `Tests/ProductionLineControllerTests.cpp`(신설) — 화면 진입/새로고침 시
      `CompleteFrontJobIfDue(now)` 자동 호출 및 완료 시 `Sample`/`Order` 갱신 검증(수동 완료
      메뉴가 없다는 전제)
- [ ] `Tests/ReleaseControllerTests.cpp`(신설) — `CONFIRMED` 목록/번호 선택/실패 시 방어적
      `try/catch` 동작(`docs/Phase4.md` 7.2절)
- [ ] `Tests/MonitoringControllerTests.cpp`(신설) — `MonitoringController::Run()`이
      `GetOrderStatusSummary`/`GetSampleStockViews`를 각각 1회 호출해 그대로 View에 위임하는지
- [ ] `Tests/DummyDataControllerTests.cpp`(신설) — 옵션 프롬프트 → `Generate` 호출 → 결과 View
      위임, 음수 개수 입력 시 `ShowInvalidCount`
- [ ] `Tests/ControllerTests.cpp`(기존) — `MainMenuController`가 `Run()` 루프에서
      `view_.PrintSummary(monitoringService_.GetMainMenuSummary())`를 메뉴 출력 직전에 호출하는지
      확인하는 케이스가 있는지 점검, 없으면 추가

**시스템(End-to-End) 테스트**

- [ ] `Tests/OrderManagementSystemTests.cpp`(기존, 또는 신규 `Tests/FullLifecycleSystemTests.cpp`)
      — 아래 3절의 전체 e2e 시나리오

### 2.3 code-review 최종 점검 (전체 diff 대상)

- [ ] `git log`/`git diff`로 Phase 0~6 전체 변경사항 파악
- [ ] `code-review` 에이전트를 "전체 diff" 범위로 호출해 교차 Phase 이슈 점검:
      - Phase 3(`OrderApprovalService`)과 Phase 4(`OrderReleaseService`)가 같은 "Model 서비스
        계층 + 값 복사 후 setter 조합 + Repository::Update" 패턴을 일관되게 따르는지
      - `ProductionLineController::ApplyCompletion`이 Controller에서 직접 `Sample.stock`/
        `Order.status`를 갱신하는 패턴이 Phase 3/4의 "Model 서비스 계층" 패턴과 다른 점이
        여전히 남아있는지, 일관성 문제로 재보고할지 판단
      - Phase 5의 `MonitoringService`가 어디에서도 `CalculateAvailableStock`을 호출하지
        않는지(물리적 재고 vs 가용 재고 혼동 금지) 전체 소스 기준 재확인
      - Model에 로직이 몰려 있고 Controller/View가 얇게 유지되는지(`main → Controller →
        (Model, View)` 의존 방향) 전체 소스 트리 기준 재확인
- [ ] 리뷰 결과에 대해 직후 `supervisor` 호출

### 2.4 문서-구현 정합성 확인

- [ ] `CLAUDE.md`/`docs/PRD.md` 기술 내용과 실제 구현을 항목별로 대조
- [ ] 각 `docs/Phase{n}.md`(n=0~6)의 완료 기준이 실제로 충족되었는지 재확인 — 특히 Phase
      3~6 문서의 "tester가 테스트를 작성해 통과시킨다"는 항목은 **이번 Phase 7 이전에는
      충족되지 않았음**을 명시하고, Phase 7에서 비로소 충족되는 것으로 정정한다.
- [ ] `docs/DATA_SCHEMA.md`가 실제 최신 코드와 계속 일치하는지 확인

### 2.5 커밋 이력 정리 (PRD 9.3)

- [ ] `git log --oneline`으로 지금까지 이력을 확인해 "의미 있는 커밋 이력" 기준 부합 여부 점검
- [ ] 정리가 필요한 부분이 있다면 사용자에게 제안하고, 실제 커밋 생성/재작성은 사용자 지시가
      있을 때만 진행(히스토리 재작성 금지 원칙 유지)

## 3. 인터페이스/데이터 스키마

Phase 7은 신규 인터페이스나 스키마를 추가하지 않는다. 실제 코드로 이미 확정된 시그니처는
`docs/Phase3.md`~`docs/Phase6.md`, `docs/DATA_SCHEMA.md`를 참고한다.

## 4. 의존 관계

- **전제(입력)**: Phase 0~6 코드까지 `origin/master`에 병합 완료. `fix/production-line-auto-complete`
  병합 여부만 착수 전 확인.
- **산출(출력)**: 이 프로젝트의 최종 단계이므로 이후 Phase는 없다. 산출물은 "빌드/테스트 통과
  + 문서-구현 정합성 확인 완료 + 정리된 커밋 이력"이라는 최종 상태 그 자체다.

## 5. 핵심 비즈니스 로직 주의사항 (회귀 검증 대상)

### 5.1 승인 시점 생산량 확정 규칙 (PRD 7.4, CLAUDE.md 원문)

```
예시1) 현재 재고 0, A시료 수율 0.5
  주문1: A 50개 주문 → 부족분 50 → 실 생산량 = ceil(50/0.5) = 100
  주문2: 주문1 생산 중 A 50개 추가 주문 → 승인 시점 재고는 여전히 0 →
         부족분 50 → 실 생산량 = ceil(50/0.5) = 100 (주문1의 미래 생산량은 고려하지 않음)

예시2) 현재 재고 90, A시료 수율 1.0
  주문1: A 100개 주문 → 부족분 = 100 - 90 = 10 → 실 생산량 10
  주문2: A 10개 주문 → 물리적 재고 90개는 이미 주문1의 출고분으로 예정 → 가용 재고 0 →
         실 생산량 10 (모니터링에는 물리적 재고 90개 그대로 표시)
```

- 실제 계산식: `Model::CalculateAvailableStock`(`Model/AvailableStockCalculator.h`).
- **회귀 테스트 대상**: `Model::OrderApprovalService::Approve`를 실제로 두 번 연속 호출해 위
  예시1·예시2를 그대로 재현하는 단위 테스트(`Tests/OrderApprovalServiceTests.cpp`, 신설).

### 5.2 물리적 재고 vs 가용 재고 혼동 금지 (Phase 5, CLAUDE.md)

- `Model::MonitoringService`는 세 Repository를 읽기 전용(`const&`)으로만 참조하며,
  `Model::CalculateAvailableStock`을 어디에서도 호출하지 않는다.
- **회귀 테스트 대상**(`Tests/MonitoringServiceTests.cpp`, 신설): PRD 7.4 예시2와 동일한
  상태를 만든 뒤 `SampleStockView.stock == 90`(물리적 재고 그대로)임을 확인.

### 5.3 더미 데이터 생성 후 모니터링 즉시 반영 (Phase 6)

- `MonitoringService`/메인 메뉴 요약은 캐시 없이 매 호출마다 Repository를 다시 읽는다.
  `DummyDataGenerator`는 `SampleRepository::Add`/`OrderService::PlaceOrder`만 호출하고 별도의
  "모니터링 갱신" 코드를 두지 않는다.
- **회귀/통합 테스트 대상**: 더미 데이터 생성 전/후 `MonitoringService::GetMainMenuSummary()`를
  비교해 `registeredSampleCount`/`totalOrderCount`/`totalStock`이 정확히 증가하고,
  `productionQueueCount`는 변하지 않는지(더미 주문은 항상 `RESERVED`) 확인.

### 5.4 생산 완료 자동화 (`fix/production-line-auto-complete`, 병합 시)

- `ProductionLine::CompleteFrontJobIfDue(now)`가 진행률 100% 미만이면 아무 것도 하지 않고,
  100% 이상이면 완료 처리 + 다음 큐 작업을 즉시 시작하는지 확인.
- **회귀 테스트 대상**(`Tests/ProductionLineTests.cpp`): `startedAt`을 과거로 설정한 job에
  대해 `CompleteFrontJobIfDue`가 완료 처리하고, 진행 중인 job의 `startedAt`이 아직 estimatedTime에
  못 미치면 완료하지 않는지.

## 6. 테스트 계획 (`tester` 에이전트, Phase 3~6 통합 1회성 작성/실행)

### 6.1 `Tests/OrderApprovalServiceTests.cpp` (신설, 최우선)

1. 재고 충분: 주문 수량 ≤ 가용 재고 → `order.status == Confirmed`, `ApprovalResult.wasQueued ==
   false`, `ProductionJob` 생성 안 됨.
2. 재고 부족: 주문 수량 > 가용 재고 → `order.status == Producing`, `ProductionJob.shortage`/
   `actualQuantity`/`estimatedTime` 계산값 검증.
3. 수율에 따른 ceil 반올림 경계값: `shortage=3, yieldRate=0.4 → actualQuantity = ceil(7.5) = 8`.
4. **PRD 예시1 재현**: Sample(A, stock=0, yieldRate=0.5) → 주문1(50개) 승인 →
   `shortage=50, actualQuantity=100, status=Producing` → (여전히 stock=0인 채로) 주문2(50개)
   승인 → `shortage=50, actualQuantity=100`.
5. **PRD 예시2 재현**: Sample(A, stock=90, yieldRate=1.0) → 주문1(100개) 승인 →
   `shortage=10, actualQuantity=10` → 주문2(10개) 승인 → `shortage=10, actualQuantity=10`,
   전 구간 `sample.stock == 90` 유지.
6. `RESERVED`가 아닌 주문에 `Approve` 호출 시 예외/오류.
7. `RESERVED` 거절 → `status == Rejected`, `updatedAt` 갱신. `RESERVED`가 아닌 주문 거절 시도 →
   오류.

### 6.2 `Tests/ProductionLineTests.cpp` (신설)

1. `Enqueue` 후 `Peek()`이 가장 먼저 등록된 job을 반환(FIFO).
2. 여러 job enqueue 후 `CompleteFrontJobIfDue` 반복 호출 시 등록 순서대로 완료.
3. 완료 반환 job의 `actualQuantity`가 승인 시점 확정값과 동일(재계산 안 됨).
4. 빈 큐에서 `Peek()`/`CompleteFrontJobIfDue` 안전 처리(`nullopt`).
5. 빈 큐에 최초 `Enqueue`하면 `startedAt`이 즉시 설정, 이미 진행 중인 job이 있을 때 추가
   `Enqueue`한 job은 `startedAt`이 설정되지 않음(대기).
6. `CompleteFrontJobIfDue` 완료 후 다음 job의 `startedAt`이 새로 설정됨.
7. `CompleteFrontJobIfDue`: 진행률이 아직 100% 미만이면 아무 것도 하지 않고 `nullopt` 반환,
   100% 이상이면 완료 처리.
8. `ComputeProgress`: 대기 중(`startedAt` 없음) → `0.0`; 경과 시간이 `estimatedTime`의 절반일
   때 → `≈0.5`; 초과 시 `1.0`으로 clamp; 순수 함수인지 확인.

### 6.3 `Tests/OrderReleaseServiceTests.cpp` (신설)

1. `CONFIRMED` → `Release` 성공: `status == Release`, `updatedAt` 갱신, `Sample.stock`이
   `order.quantity`만큼 정확히 감소.
2. `RESERVED`/`PRODUCING`/`REJECTED`/이미 `RELEASE`인 주문에 `Release` 호출 → 예외, `Order`/
   `Sample` 모두 미변경.
3. 대상 `Sample`이 없는 경우 → `std::runtime_error`, `Order`도 미변경.
4. 경계값: 출고 후 `stock`이 정확히 0이 되는 케이스.

### 6.4 `Tests/MonitoringServiceTests.cpp` (신설, 5.2절 회귀 포함)

1. 주문이 없을 때 `OrderStatusSummary` 전부 0.
2. `Reserved`/`Confirmed`/`Producing`/`Release`/`Rejected`가 섞여 있을 때 `Rejected`만
   집계에서 완전히 빠지는지.
3. `GetSampleStockViews()`: `stock == 0` → `Depleted`(최우선), `stock > 0`이고 `stock <
   pendingOrderQuantity` → `Shortage`, 그 외 → `Sufficient`.
4. `pendingOrderQuantity`가 오직 `Producing` 상태 주문 quantity 합인지 확인.
5. **PRD 7.4 예시2 조건에서 물리적 재고 노출 검증**: 재고 90에서 `CONFIRMED`/`PRODUCING`
   주문으로 가용 재고가 0으로 소진된 상태를 만든 뒤 `SampleStockView.stock`이 여전히 90인지.
6. `GetMainMenuSummary()`: 등록 시료 수/총 재고 합/전체 주문 수(Rejected 제외)/생산 라인 대기
   건수가 정확한지.

### 6.5 Controller 계층 (GMock View, 신설)

- `Tests/OrderApprovalControllerTests.cpp`: `RESERVED` 목록 표시, 승인/거절 라우팅, "항상
  접수 순서상 첫 번째만 처리 대상" 강제 검증.
- `Tests/ProductionLineControllerTests.cpp`: 화면 진입/새로고침 시 `CompleteFrontJobIfDue(now)`
  자동 호출, 완료 시 `Sample`/`Order` Repository 갱신 검증(수동 완료 메뉴 없음 전제).
- `Tests/ReleaseControllerTests.cpp`: `CONFIRMED` 목록/자유 번호 선택/범위 밖 입력 처리/방어적
  `try-catch` 검증.
- `Tests/MonitoringControllerTests.cpp`: `Run()`이 `GetOrderStatusSummary`/
  `GetSampleStockViews`를 각 1회 호출해 그대로 View에 위임하는지.
- `Tests/DummyDataControllerTests.cpp`: 옵션 프롬프트 → `Generate` 호출 → 결과 View 위임,
  음수 개수 입력 시 `ShowInvalidCount`.
- `Tests/ControllerTests.cpp`(기존 파일 보강): `MainMenuController::Run()`이 메뉴 출력 직전에
  `PrintSummary(GetMainMenuSummary())`를 호출하는지에 대한 케이스 존재 여부 점검, 없으면 추가.

### 6.6 `Tests/DummyDataGeneratorTests.cpp` (신설, `docs/Phase6.md` 7절 재인용)

- 정상 생성(개수/범위 검증), 경계값(`sampleCount=0, orderCount=0`), 시료 ID 채번 충돌 회피
  (`S-{n}` 순차), 동일 시드 재현성, 등록된 시료가 하나도 없을 때 주문 생성 실패 처리.
- 더미 데이터 → 모니터링 즉시 반영(5.3절 시나리오 그대로).

### 6.7 전체 통합 시나리오 (System Test)

- 시료 2개 이상 등록 → 각각에 대해 여러 건 주문 → 일부 거절, 일부 승인(재고 충분/부족 혼합) →
  생산 큐 FIFO 순서 진행 → 생산 완료 후 `Confirmed` → 출고 → `Release`까지 한 번의 테스트
  흐름으로 검증. 종료 시점 `MonitoringService` 결과가 실제 데이터와 일치하는지 확인.
- **영속성 회귀**: 위 시나리오 중 `Producing` 상태 존재 시점에 Repository를 재로드(`Load()`
  재호출)한 뒤에도 주문/생산 큐 상태(`startedAt` 포함)가 그대로 복원되는지 확인.
- **더미 데이터 통합**: 더미 데이터 생성 후 위 시나리오에 자연스럽게 섞어 승인/생산/출고까지
  이어가도 정상 동작하는지 확인.
- 실패를 감추기 위해 프로덕션 코드나 테스트 기대값을 몰래 조정하지 않고, 통과/실패 개수와
  실패 원인을 명확히 보고한다.

### 6.8 수동 e2e (콘솔 앱 직접 실행)

1. 시료 등록 (메뉴 1)
2. 주문 접수 → `RESERVED` (메뉴 2)
3. 주문 승인 → 재고 충분/부족 분기, 부족 시 생산 큐 등록·FIFO 진행·자동 완료까지 (메뉴 3, 5)
4. `CONFIRMED` 주문 출고 → `RELEASE` (메뉴 6)
5. 모니터링에서 상태별 건수(`REJECTED` 제외)/재고 현황(여유·부족·고갈) 확인 (메뉴 4)
6. 더미 데이터 생성(메뉴 7) → 메인 메뉴 요약/모니터링에 반영되는지 확인
7. 앱 종료 후 재시작 → `Order`/`ProductionJob` 진행 중 상태가 JSON에서 그대로 복원되는지 확인

## 7. 완료 조건 (Definition of Done)

- [ ] Debug/Release × Win32/x64 전체 구성 빌드 성공 (신규 경고 발생 시 원인 보고)
- [ ] `SampleOrderSystem.exe --test` 전체 스위트(6절의 신규 테스트 파일 전부 포함) 100% 통과,
      특히 **PRD 7.4 예시1·예시2**(6.1절 4·5번)와 **모니터링 물리적 재고 노출**(6.4절 5번)
      테스트가 실제로 작성되어 통과함이 최우선 확인 대상
- [ ] 신규 `.cpp` 테스트 파일이 모두 `SampleOrderSystem.vcxproj`에 등록되어 실제로 실행됨
- [ ] PRD 5장 업무 흐름 전체가 콘솔 앱에서 수동으로도 end-to-end 동작 확인됨(6.8절)
- [ ] `code-review` 전체 diff 최종 점검 완료 및 직후 `supervisor` 검증 완료
- [ ] 문서-구현 정합성 점검 결과가 사용자에게 보고됨(불일치 없으면 "없음"으로 명시)
- [ ] 커밋 이력 점검 결과와 필요 시 정리 제안이 사용자에게 보고됨
- [ ] `supervisor`의 최종 요약을 받아 사용자가 프로젝트 종료 여부를 판단할 수 있는 상태가 됨

## 8. 확인이 필요한 지점 (TODO)

1. **`ProductionLineController::ApplyCompletion`의 Controller 직접 상태 갱신** — Phase 3/4가
   서로 다른 패턴(Model 서비스 vs Controller 직접 갱신)을 쓰고 있는 점이 Phase 7의
   code-review에서 일관성 문제로 다시 지적될 가능성이 높다. 지적되면 사용자에게 리팩터링
   여부를 확인한다(Phase 7 자체가 임의로 구조를 바꾸지 않는다).
2. **커밋 이력 정리의 구체적 방식**(스쿼시/태그 여부 등)은 PRD/PLAN/CLAUDE.md에 명시가 없다.
   파괴적 히스토리 재작성은 하지 않는 것을 기본 원칙으로 삼되, 필요 시 사용자에게 방식을
   확인한다.
