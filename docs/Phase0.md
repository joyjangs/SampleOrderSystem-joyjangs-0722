# Phase 0 — 기반 구축 (모듈 단위)

> 본 문서는 `docs/PLAN.md`의 "Phase 0 — 기반 구축(모듈 단위)" 절을 구체화한 작업 계획이다.
> Phase 0은 이후 모든 기능(Phase 1~7)이 공유하는 뼈대이므로, 기능 단위가 아니라 모듈
> 단위(도메인 모델 / 영속성 / 테스트 하네스 / 콘솔 골격)로 진행한다.

## 0. 현재 저장소 상태 (전제)

- `Model/`, `View/`, `Controller/` 폴더 및 소스 파일 없음. `main.cpp` 없음.
- `SampleOrderSystem.vcxproj`/`SampleOrderSystem.slnx`만 존재하는 빈 콘솔 프로젝트.
  `.vcxproj`에는 이미 `packages/gmock.1.11.0`을 가리키는 NuGet 참조(`gmock.targets` import,
  `EnsureNuGetPackageBuildImports` 타깃)가 구성되어 있으나, 실제 `ClCompile`/`ClInclude`
  항목은 하나도 없다.
- `SampleOrderSystem.slnx`에는 `SampleOrderSystem.vcxproj` 프로젝트 하나만 등록되어 있다
  (테스트 전용 프로젝트 없음).
- `packages/gmock.1.11.0/`에 GoogleTest/GMock NuGet 패키지가 이미 다운로드되어 있다
  (`lib/native/include/{gtest,gmock}`, `build/native/gmock.targets`).
- JSON 파싱 라이브러리는 아직 저장소에 존재하지 않는다 (별도 확정 필요, 아래 1.3 참고).

## 1. 목표 요약 (PLAN.md 인용)

**목표**: 이후 모든 기능이 공유할 뼈대를 완성한다.

- 프로젝트 폴더 구조 정리: `Model/`, `View/`, `Controller/` 분리 (ConsolMVC류 의존 방향 적용:
  `main → Controller → (Model, View)`, Model/View는 서로를 모름)
- 도메인 모델 정의: `Sample`, `Order`(status enum 포함), `ProductionJob`
- JSON 파서 + 파일 기반 Repository 계층 구축 (Sample/Order/ProductionJob 각각의 로드/저장/CRUD)
  — 애플리케이션 재시작 후 주문·생산 라인 정보가 복원되는지 확인
- GoogleTest/GMock 테스트 프로젝트를 솔루션에 연결 (`packages/gmock.1.11.0` 재사용)
- 콘솔 진입점(`main.cpp`)에서 최소 골격(빈 메인 메뉴 표시) 동작 확인

**완료 기준**: 샘플 데이터 하나를 저장 → 재시작 → 로드까지 되는 콘솔 앱이 빌드/실행되고,
Repository CRUD에 대한 단위 테스트가 통과한다.

## 2. 작업 항목 체크리스트

### 2.1 프로젝트/폴더 구조

- [ ] 솔루션 루트에 `Model/`, `View/`, `Controller/`, `Common/`(선택, 공용 유틸/타입) 폴더 생성
- [ ] `SampleOrderSystem.vcxproj`/`.vcxproj.filters`에 각 폴더를 필터로 반영 (Model Files /
      View Files / Controller Files 등, 기존 `.vcxproj.filters` 구조를 존중해 확장)
- [ ] 의존 방향 문서화(주석/설계 노트 수준): `main.cpp → Controller → (Model, View)`.
      Model은 View/Controller를 include하지 않고, View는 Model의 도메인 로직을 호출하지 않으며
      Controller로부터 필요한 데이터를 전달받아 출력만 담당한다.
- [ ] `main.cpp`를 솔루션 루트(또는 별도 지정 위치, 기존 구조 유지)에 추가

### 2.2 도메인 모델 (Model 계층)

- [ ] `Model/OrderStatus.h` (또는 `Model/Order.h`에 포함): `enum class OrderStatus`
      { `Reserved`, `Rejected`, `Producing`, `Confirmed`, `Release` } 정의.
      `ToString`/`FromString` 변환 함수(직렬화용)를 함께 둔다.
- [ ] `Model/Sample.h` / `Sample.cpp`: `Sample` 클래스
  - 필드: `id`(string), `name`(string), `avgProductionTime`(double/number),
    `yieldRate`(double, 0.0~1.0), `stock`(int)
  - 접근자(getter) 및 최소한의 불변식 검증은 Phase 1(시료 등록)에서 구현하되, Phase 0에서는
    데이터 컨테이너로서의 구조와 JSON 직렬화가 가능한 형태만 확정한다.
- [ ] `Model/Order.h` / `Order.cpp`: `Order` 클래스
  - 필드: `orderId`(string), `sampleId`(string), `customerName`(string), `quantity`(int),
    `status`(OrderStatus), `createdAt`(datetime — 문자열 ISO8601 또는 `time_t`, 아래 3.2
    참고), `updatedAt`(datetime)
  - Phase 0에서는 상태 전이 로직(승인/거절/생산완료 등)을 구현하지 않는다 — 이는 Phase 2/3/4의
    책임이다. Phase 0은 데이터 구조와 (역)직렬화만 담당한다.
- [ ] `Model/ProductionJob.h` / `ProductionJob.cpp`: `ProductionJob` 클래스
  - 필드: `orderId`(string), `sampleId`(string), `shortage`(int), `actualQuantity`(int),
    `estimatedTime`(double), `startedAt`(datetime, nullable — 생산 큐 대기 중이면 값 없음,
    생산이 시작되면 시작 시각을 저장. 포맷은 3.2의 다른 datetime 필드와 동일한 규칙 적용)
  - `progress`는 저장 필드로 두지 않는다.[^progress-derived]
- [ ] 각 모델에 `ToJson()`/`FromJson()`(또는 자유 함수 `to_json`/`from_json`) 인터페이스를
      일관된 방식으로 정의 — Repository 계층에서 재사용

### 2.3 JSON 라이브러리 확정 (완료)

- **확정된 방식**: 외부 라이브러리를 도입하지 않고 `Common::JsonValue`(재귀 하강 파서 +
  직렬화기, null/bool/number/string/array/object 지원)를 직접 구현했다. 로컬 PoC에서 검증된
  "의존성 없는 자체 JSON 파서" 방식을 재사용한 것이며(CLAUDE.md 원칙: "선행 PoC에서 검증된
  구조/기법을 그대로 활용"), nlohmann/json 등 외부 헤더 온리 라이브러리는 채택하지 않았다.
  vendor 헤더 추가나 NuGet 패키지 없이 `Common/Json.h`/`Common/Json.cpp` 두 파일만으로
  완결된다.
- 이 결정은 `CLAUDE.md`의 "데이터 영속성" 항목에도 반영되어 있다 — 더 이상 "예:
  nlohmann/json" 같은 잠정 표현이 아니다.
- 어떤 라이브러리를 쓰든 다음 제약은 고정한다:
  - 외부 라이브러리는 헤더 온리로 프로젝트에 포함하거나, NuGet 패키지로 `packages/`에
    받아 `packages.config`/`.vcxproj`에 반영한다 (gmock과 동일한 통합 패턴 유지).
  - JSON (역)직렬화 코드는 Model 클래스 자체 또는 별도 `Model/*Json.h`(직렬화 전용 헤더)에
    위치시켜, Model의 순수 도메인 로직과 직렬화 관심사를 과도하게 뒤섞지 않는다(Clean
    Code/SRP 관점, code-review 대상).

### 2.4 Repository 계층 (영속성)

- [ ] `Model/Repository.h`: 공용 Repository 인터페이스 템플릿 또는 각 엔티티별 인터페이스
      정의. 예시 시그니처(아래 3.1 인터페이스 절 참고)
- [ ] `Model/SampleRepository.h` / `.cpp`: `Sample`의 JSON 파일 기반 CRUD
- [ ] `Model/OrderRepository.h` / `.cpp`: `Order`의 JSON 파일 기반 CRUD
- [ ] `Model/ProductionJobRepository.h` / `.cpp`: `ProductionJob`의 JSON 파일 기반 CRUD
- [ ] 각 Repository는 지정된 JSON 파일 경로(예: `data/samples.json`, `data/orders.json`,
      `data/production_jobs.json`)를 읽고 쓴다. 파일이 없으면 빈 컬렉션으로 시작하고, 최초
      저장 시 파일/폴더를 생성한다.
- [ ] 저장 시점 정책 확정: TODO: 확인 필요 — "매 CRUD 연산마다 즉시 파일에 flush"할지,
      "명시적 `Save()` 호출 시에만 flush"할지는 PRD/CLAUDE.md에 명시되어 있지 않다. Phase 0
      구현자는 이 중 하나를 선택하고, "재시작 후 진행 중이던 주문/생산 큐 상태까지 복원"
      요구사항(CLAUDE.md 핵심 구현 요소, PRD 9.2)을 만족하는지 통합 테스트로 검증한다. 즉시
      flush 방식이 요구사항을 더 단순하게 만족시키므로 기본안으로 권장한다.
- [ ] `data/` 폴더(JSON 저장 위치)를 프로젝트에 추가하고 `.gitignore` 또는 샘플 초기 JSON
      파일 포함 여부를 결정한다 (빈 배열 `[]`로 초기화된 파일을 커밋해 최초 실행 시 파일
      부재로 인한 예외를 피하는 것을 권장).

### 2.5 콘솔 진입점 (View/Controller 최소 골격)

- [ ] `View/MainMenuView.h` / `.cpp`: 메인 메뉴 화면 출력만 담당하는 최소 클래스/함수
      (하위 메뉴 항목 텍스트만 출력, 아직 각 메뉴 기능은 구현하지 않음 — Phase 1 이후 채움)
- [ ] `Controller/MainMenuController.h` / `.cpp`: 메인 메뉴 루프(입력 → 종료 조건 처리)의
      최소 골격. Phase 0 범위에서는 "종료" 선택지만 실제로 동작하면 충분하고, 나머지 메뉴
      항목은 "미구현" 안내만 출력해도 된다.
- [x] `main.cpp`: Repository 인스턴스 생성 → `Run()` 호출. **범위 축소(확정)**: Phase 0의
      `MainMenuController`는 아직 어떤 메뉴 항목도 Repository를 사용하지 않으므로(모든 하위
      메뉴가 "미구현" 안내만 출력), Repository를 Controller에 실제로 주입하지 않는다.
      `main.cpp`는 3개 Repository를 생성하고 `Load()`까지만 호출해 "재시작 시 로드" 요구사항을
      만족시킨다. `IRepository<...>&`를 실제로 받는 배선은 이를 사용하는 첫 하위 메뉴가 생기는
      Phase 1(시료 관리)에서 해당 Controller 생성자에 주입하는 시점에 도입한다.

## 3. 인터페이스 / 데이터 스키마

### 3.1 Repository 인터페이스 (예시 시그니처)

```cpp
// Model/Repository.h
template <typename TEntity, typename TId>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::vector<TEntity> FindAll() const = 0;
    virtual std::optional<TEntity> FindById(const TId& id) const = 0;
    virtual void Add(const TEntity& entity) = 0;
    virtual void Update(const TEntity& entity) = 0;
    virtual void Remove(const TId& id) = 0;
    virtual void Save() = 0;   // 메모리 상태를 JSON 파일에 flush
    virtual void Load() = 0;   // JSON 파일에서 메모리로 적재
};
```

- `SampleRepository : IRepository<Sample, std::string>`
- `OrderRepository : IRepository<Order, std::string>`
- `ProductionJobRepository : IRepository<ProductionJob, std::string>` (키는 `orderId` 기준,
  주문당 생산 작업은 최대 1건이라는 전제 — TODO: 확인 필요. PRD/PLAN에 "주문당 1개의
  ProductionJob"이라는 명시적 제약은 없으나 상태 흐름상(RESERVED→PRODUCING은 1회) 자연스러운
  전제이며, Phase 3에서 재확인한다.)
- Controller는 구체 클래스가 아니라 `IRepository<...>` 인터페이스에 의존하도록 구성해
  DIP를 지킨다 (code-review 검토 대상).

### 3.2 JSON 스키마 (PRD 8장 기준, Phase 0에서 확정)

```jsonc
// data/samples.json
[
  { "id": "S-001", "name": "SampleA", "avgProductionTime": 10.0, "yieldRate": 0.9, "stock": 50 }
]

// data/orders.json
[
  {
    "orderId": "ORD-20260416-0043",
    "sampleId": "S-001",
    "customerName": "고객사A",
    "quantity": 50,
    "status": "RESERVED",
    "createdAt": "2026-04-16T10:00:00",
    "updatedAt": "2026-04-16T10:00:00"
  }
]

// data/production_jobs.json
[
  {
    "orderId": "ORD-20260416-0043",
    "sampleId": "S-001",
    "shortage": 50,
    "actualQuantity": 100,
    "estimatedTime": 1000.0,
    "startedAt": null
  }
]
```

[^progress-derived]: `progress`는 영속 필드가 아니다. Phase 3에서 확정된 설계에 따라
    `startedAt`(nullable)만 저장하고, "생산 라인 조회" 시점마다 `ComputeProgress`(순수 함수,
    `progress = min(1.0, (now - startedAt) / estimatedTime)`)로 파생 계산한다. 계산 로직의
    세부 사항은 `docs/Phase3.md`를 참고한다.

- `status` 필드는 PRD 8.2와 동일한 문자열 값(`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/
  `RELEASE`)을 그대로 사용해 JSON 가독성과 PRD 간 일관성을 유지한다.
- `createdAt`/`updatedAt`의 datetime 표현 형식(ISO8601 문자열 vs epoch 정수)은 TODO: 확인
  필요 — PRD는 타입만 `datetime`으로 명시하고 포맷은 정하지 않았다. 사람이 읽기 쉽고
  JSON 텍스트로 안정적인 ISO8601 문자열(`YYYY-MM-DDTHH:MM:SS`)을 기본안으로 권장한다.

## 4. 의존 관계

- **전제(선행 산출물)**: 없음 — Phase 0은 최초 Phase이며 기존 소스 코드에 의존하지 않는다.
  단, `.vcxproj`에 이미 구성된 gmock NuGet 참조와 `packages/gmock.1.11.0`은 그대로 재사용한다
  (중복 설정 금지).
- **이후 Phase가 기대하는 산출물**:
  - Phase 1(시료 관리): `Sample` 클래스, `SampleRepository` CRUD, Model/View/Controller
    폴더 구조, 메인 메뉴 진입 골격을 그대로 확장해 하위 메뉴를 채운다.
  - Phase 2(주문 접수): `Order`/`OrderStatus`, `OrderRepository`를 확장해 `RESERVED` 생성
    로직을 얹는다.
  - Phase 3(승인/거절 + 생산 큐): `ProductionJob`/`ProductionJobRepository`, `Order`의 상태
    전이 지점, Repository의 `Update`/`Save` 시맨틱(즉시 flush 여부)에 의존해 "승인 시점
    확정" 규칙을 구현한다.
  - Phase 5(모니터링)/Phase 6(더미 데이터)은 Repository의 `FindAll()`을 그대로 재사용한다.
  - 전 Phase 공통: JSON 스키마(3.2)와 Repository 인터페이스(3.1) 시그니처를 변경할 경우
    이후 Phase 문서와 어긋나지 않도록 본 문서를 함께 갱신한다.

## 5. 핵심 비즈니스 로직 주의사항

- Phase 0 자체에는 PRD 7.4의 "승인 시점 생산량 확정 규칙"과 같은 복잡한 도메인 로직이
  포함되지 않는다. 다만 **Phase 3에서 이 규칙을 구현하려면 Phase 0에서 다음을 미리
  보장해야 한다**:
  - `Sample.stock`은 물리적 재고만을 의미하는 필드로 유지하고, "승인 시점 가용 재고"를
    저장할 별도 필드를 `Sample`에 만들지 않는다(가용 재고는 Phase 3에서 계산 시점에만
    쓰이는 값으로, Repository/모델의 영속 필드로 만들면 안 된다 — TODO: 확인 필요 시
    Phase 3 문서에서 재확인).
  - `ProductionJob`의 `shortage`/`actualQuantity`/`estimatedTime` 필드는 반드시 Phase 0에서
    정의한 그대로 유지하고(값 자체는 Phase 3에서 채움), 이후 재계산을 위한 여분 필드(예:
    "생산 시작 시점 재고")를 추가하지 않는다 — PRD 7.4/7.6 규칙상 재계산 자체가 금지된다.
  - JSON 저장/로드 시 `ProductionJob`은 "진행 중이던" 상태까지 그대로 복원되어야 한다
    (CLAUDE.md 데이터 영속성 항목). Phase 0의 완료 기준 테스트에 이 복원 케이스를 반드시
    포함한다.

## 6. 테스트 계획 (tester 에이전트 작성 대상)

### 6.1 단위 테스트 — 도메인 모델 직렬화

- `Sample`을 JSON으로 직렬화 후 역직렬화하면 모든 필드(id/name/avgProductionTime/
  yieldRate/stock)가 동일하게 복원되는지
- `Order`를 직렬화/역직렬화 시 `status` enum 값이 문자열로 정확히 왕복 변환되는지
  (`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASE` 5개 모두)
- `ProductionJob`의 모든 필드가 직렬화/역직렬화 후 동일한지 (수치 정밀도 포함,
  `estimatedTime`이 double인 경우 반올림 오차 없이 왕복되는지), 특히 `startedAt`이 `null`(큐
  대기 중)인 경우와 실제 시각 값이 채워진 경우 모두 정확히 저장/복원되는지 (`progress`는
  저장 필드가 아니므로 직렬화 테스트 대상에서 제외한다)
- 알 수 없는/잘못된 `status` 문자열이 들어온 경우의 처리(예외 또는 기본값) — 방어적 코딩
  검증

### 6.2 단위 테스트 — Repository CRUD

- `SampleRepository`: `Add` 후 `FindById`로 조회되는지, `FindAll` 개수가 맞는지, `Update`
  후 필드가 반영되는지, `Remove` 후 조회되지 않는지
- `OrderRepository`/`ProductionJobRepository`도 동일한 CRUD 시나리오
- 빈 파일/파일 없음 상태에서 `Load()` 호출 시 예외 없이 빈 컬렉션을 반환하는지
- 존재하지 않는 id로 `FindById` 호출 시 `std::nullopt`(또는 정의된 실패 값) 반환 확인

### 6.3 통합(시스템) 테스트 — 영속성 재시작 시나리오

- **완료 기준 핵심 케이스**: `Sample` 1건을 생성 → `Save()` → Repository 인스턴스를
  새로 생성(재시작 흉내) → `Load()` → 동일한 `Sample`이 조회되는지
- `Order`/`ProductionJob`에 대해서도 동일한 재시작 시나리오 적용 — 특히 `PRODUCING` 상태의
  `Order`와 이에 연결된 `ProductionJob`(생산이 이미 시작되어 `startedAt`이 채워진 경우와
  아직 큐 대기 중이라 `startedAt`이 `null`인 경우 모두)이 재시작 후에도 그대로 복원되는지
  (CLAUDE.md 영속성 요구사항 핵심 검증 포인트)
- 여러 건(3건 이상)을 저장 후 재로드했을 때 순서/개수가 보존되는지 (FIFO 큐 순서는 Phase
  3에서 본격 검증하지만, Phase 0에서는 최소한 저장 순서가 임의로 뒤섞이지 않는지만 확인)

### 6.4 콘솔 골격 스모크 테스트

- `main.cpp` 실행 시 메인 메뉴가 출력되고 "종료" 입력 시 정상 종료되는지 (자동화된
  GoogleTest로 표준입출력을 mocking하기 어렵다면, 최소한 Controller의 "종료 처리" 로직만
  단위 테스트로 분리해 검증 — 예: `MainMenuController::HandleInput("0")`이 종료 플래그를
  true로 설정하는지)

## 7. 완료 조건 (Definition of Done)

- [x] `Model/`, `View/`, `Controller/`, `Common/` 폴더가 생성되고 `.vcxproj`/`.vcxproj.filters`에
      반영됨
- [x] `Sample`/`Order`(+`OrderStatus`)/`ProductionJob` 클래스가 PRD 8장 필드와 일치하게
      정의되고, JSON (역)직렬화가 가능함
- [x] JSON 라이브러리 채택이 확정되고 `CLAUDE.md`/본 문서에 실제 채택 내용이 반영됨
      (외부 라이브러리 없이 `Common::JsonValue` 자체 구현으로 확정 — 2.3절 참고)
- [x] `SampleRepository`/`OrderRepository`/`ProductionJobRepository`가 CRUD를 지원하고,
      JSON 파일로 저장/로드됨
- [x] `SampleOrderSystemTests` 테스트 프로젝트가 `.slnx`에 추가되어 `packages/gmock.1.11.0`을
      재사용해 빌드됨
- [x] `main.cpp`가 빌드되어 콘솔에서 실행되며 최소 메인 메뉴가 출력되고 정상 종료됨
      (Repository는 생성/`Load()`까지만 수행하고 Controller에는 아직 주입하지 않음 — 2.5절
      "범위 축소" 참고, Phase 1에서 배선)
- [x] 샘플 데이터 1건을 저장 → (프로세스 재시작을 흉내 낸) 재로드까지 동작하는 것이
      통합 테스트로 확인됨
- [x] `code-review` 에이전트 검토에서 MVC 의존 방향 위반, 심각한 SOLID 위반이 없다고
      확인됨 (경미한 개선 제안은 다음 Phase에서 반영 가능)
- [x] `tester` 에이전트가 위 6장 테스트를 작성 및 실행해 전부 통과함을 보고함 (35/35 통과)
- [x] `supervisor` 에이전트가 위 산출물이 CLAUDE.md/PRD.md/PLAN.md/본 문서와 부합함을 확인함
