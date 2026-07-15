# Phase 1 — 시료 관리

## 0. 문서 상태 메모

- Phase 0 구현이 완료되어 실제 소스 트리(`Model/`, `View/`, `Controller/`, `main.cpp`)가
  존재한다. 아래 내용은 실제 코드(`Model/Sample.h`, `Model/Repository.h`,
  `Model/JsonFileRepository.h`, `Model/SampleRepository.h`, `Controller/MainMenuController.h`/
  `.cpp`, `main.cpp`)를 직접 확인해 반영한 **확정 사실**이다.
- Phase 0에서 `Model::Sample`은 검증 로직이 전혀 없는 순수 데이터 컨테이너로 구현됐고,
  Repository는 `IRepository<TEntity, TId>` 공용 인터페이스 + `JsonFileRepository<TEntity>`
  공용 템플릿 + 엔티티별 얇은 서브클래스(`SampleRepository` 등) 구조로 구현됐다. 등록 검증
  로직과 시료 전용 검색 기능은 Phase 0이 의도적으로 Phase 1에 위임한 부분이며, 본 문서는 그
  위임을 실제 코드에 반영하는 계획을 담는다.
- `main.cpp`은 아직 `SampleRepository`를 Controller에 주입하지 않고, `MainMenuController`도
  아직 하위 메뉴로 위임하지 않는다(전부 `ShowNotImplemented()`). 이 배선을 완성하는 것이
  Phase 1의 범위다.

## 1. 목표 요약 (PLAN.md 인용)

**목표**: PRD 7.2 — 시료 등록/조회/검색.

- Model: `Sample` CRUD (등록 시 id/이름/평균 생산시간/수율 검증)
- Controller: 시료 관리 메뉴 라우팅
- View: 시료 등록 입력 화면, 목록(재고 포함) 출력, 이름 검색 결과 출력
- 메인 메뉴에서 시료 관리 하위 메뉴 진입 가능하게 연결

**완료 기준**: 시료 등록 → 목록 조회 → 이름 검색이 콘솔에서 실제로 동작. 수율/평균 생산시간
유효성 검증에 대한 단위 테스트 포함.

관련 PRD 조항(7.2 발췌):

- 시료는 시스템의 가장 기본 단위이며, **시스템에 등록된 시료만 주문 가능**하다(이 제약의
  "주문 시 검증"은 Phase 2 책임이지만, Phase 1은 이 제약이 성립하도록 Sample 저장소를
  Phase 2가 조회할 수 있는 형태로 준비해야 한다).
- 시료 등록 속성: 시료 ID, 이름, 평균 생산시간, 수율.
- 시료 조회: 등록된 모든 시료 목록(현재 재고 수량 포함).
- 시료 검색: 이름 등 속성으로 특정 시료 검색.
- 수율(Yield) = 정상 생산 시료 수 / 총 생산 시료 수. 예: 100개 중 90개 정상 → 수율 0.9
  (즉 0.0 초과 1.0 이하 범위가 정상 범위. 0은 "생산해도 전량 불량"을 의미하므로 실무적으로
  허용하지 않는 편이 안전 — 아래 "핵심 비즈니스 로직 주의사항" 참고).

Phase 1은 **주문/승인/생산/모니터링/출고 기능을 구현하지 않는다.** Sample 데이터가 다른
Phase(2~7)에서 어떻게 소비되는지는 알아야 하지만(재고 필드, ID 체계 등 인터페이스 안정성
확보 목적), 해당 로직 자체는 만들지 않는다.

## 2. 작업 항목 체크리스트

### 2.1 Model 계층

- [x] (확인 완료) `Model/Sample.h`에 `Sample` 클래스가 이미 정의돼 있다. 필드 구성은 PRD 8.1과
      일치: `id`(string), `name`(string), `avgProductionTime`(double), `yieldRate`(double),
      `stock`(int, 기본값 0). 현재 생성자 `Sample(std::string id, std::string name,
      double avgProductionTime, double yieldRate, int stock)`는 **검증 없이 그대로 저장**하는
      순수 데이터 컨테이너이며, `GetId/GetName/GetAvgProductionTime/GetYieldRate/GetStock`
      getter와 `SetName/SetAvgProductionTime/SetYieldRate/SetStock` setter, `ToJson()`/
      `static FromJson()`만 존재한다. Phase 1은 이 클래스에 검증 로직을 실제로 추가한다.
- [x] `Sample`에 정적 검증 함수를 추가한다(기존 getter/setter는 그대로 유지):
  - [x] `static bool IsValidYieldRate(double yieldRate)`: `0.0 < yieldRate <= 1.0`만 허용.
  - [x] `static bool IsValidAvgProductionTime(double time)`: `time > 0`만 허용.
  - [x] `static bool IsValidId(const std::string& id)` / `static bool IsValidName(const std::string& name)`:
        trim 후 비어있지 않아야 함.
- [x] 생성자 대신 검증을 통과한 경우에만 `Sample`을 만들 수 있는 정적 팩토리를 추가한다:
      `static std::optional<Sample> TryCreate(std::string id, std::string name,
      double avgProductionTime, double yieldRate, int stock = 0)`. 기존 생성자는 그대로 두되
      (Repository의 `FromJson` 등 내부 용도로 계속 사용), 신규 등록 경로(Controller)는 반드시
      `TryCreate`를 거치도록 한다. 실패 시 `std::nullopt`을 반환하며 예외를 던지지 않는다
      (Phase 0에 도메인 검증 예외 관례가 없고, `FindById`가 이미 `std::optional`을 쓰는
      스타일과 일관되게 `std::optional` 기반으로 Phase 1이 관례를 확정한다).
  - [x] `stock`은 등록 시 항상 0으로 고정한다. 등록 화면에서 초기 재고를 입력받지 않고
        `TryCreate`도 `stock` 파라미터를 기본값 0으로만 받는다(생산 완료 시점에만 증가시킨다는
        CLAUDE.md 원칙과 정합). PRD 7.2가 등록 입력 항목에 재고를 명시하지 않는 점과도 일치.
- [x] (확인 완료, 더 이상 TODO 아님) 시료 ID 채번 규칙: **사용자가 직접 ID를 입력**하고
      Model/Controller는 중복 여부만 검증한다(자동 채번 없음). docs/Phase6.md(더미 데이터
      생성기) 조정 과정에서 이미 확정되어 반영된 사실이다.
- [x] `Model/SampleRepository.h`/`.cpp`에 `SearchByName` 메서드를 새로 추가한다(현재는 존재하지
      않음). `IRepository` 공통 인터페이스에는 없는 `Sample` 전용 메서드이므로 `SampleRepository`
      에만 선언한다: `virtual std::vector<Sample> SearchByName(const std::string& keyword) const`.
      부분 일치 여부/대소문자 무시 여부는 "TODO: 확인 필요" — 기본안은 대소문자 무시 부분
      문자열 포함 검색. **`virtual`로 선언**해 6.3의 GMock 테스트에서 `SampleRepository`를
      상속한 목 클래스가 `Add`/`FindById`(이미 `IRepository`에서 가상)와 함께 `SearchByName`도
      오버라이드할 수 있게 한다(그렇지 않으면 GMock으로 이 메서드를 목킹할 수 없다).
- [x] `Model/Repository.h`의 `IRepository<TEntity, TId>::Add`/`Update`는 `void`이며 `Add`는
      중복 `id`를 거부하지 않는다(그냥 `push_back`). 이 공용 템플릿은 Order/ProductionJob도
      상속하므로 Phase 1이 임의로 시그니처를 바꾸지 않는다. 대신 "동일 id로 재등록 시 실패"
      요구사항은 **Controller가 `Add` 호출 전에 `FindById`로 중복을 먼저 확인**하는 방식으로
      충족한다(아래 2.2 참고).

### 2.2 Controller 계층

- [x] `Controller/SampleController.h`/`.cpp` 신설.
  - [x] 생성자 `SampleController(Model::SampleRepository& repository, View::SampleView& view)`
        — 생성자 주입, DI로 테스트 용이성 확보.
  - [x] `void Run()`: 시료 관리 하위 메뉴 루프 진입점. `MainMenuController`가 이 메서드를
        호출해 하위 메뉴로 위임한다.
  - [x] `void HandleRegister()`: View로부터 입력을 받아 **`Sample::TryCreate(...)`를 호출하기
        전에 먼저 `repository_.FindById(id)`로 중복 여부를 확인**한다. 이미 존재하면
        Repository의 `Add`를 호출하지 않고 실패 결과를 View에 전달한다(`IRepository::Add`
        자체는 중복을 거부하지 않으므로 Controller가 이 책임을 진다). 중복이 아니면
        `TryCreate`로 검증하고, 성공 시 `repository_.Add(*sample)` 호출 후 결과를 View에
        전달, 실패(`std::nullopt`) 시 검증 실패 메시지를 View에 전달한다.
  - [x] `void HandleListAll()`: `repository_.FindAll()` 결과를 그대로 View의
        `ShowSampleList(...)`에 위임해 출력.
  - [x] `void HandleSearchByName()`: View로부터 검색어를 입력받아 `repository_.SearchByName(...)`
        호출 후 결과를 View의 `ShowSearchResult(...)`에 위임해 출력(0건 포함).
  - [x] Controller는 입력 파싱/흐름 제어만 하고, 수율/생산시간 유효성 판단 로직 자체는 갖지
        않는다(Model에 위임) — CLAUDE.md 아키텍처 원칙 준수.
- [x] `Controller/MainMenuController.h`/`.cpp`를 수정해 "1"(시료 관리) 입력을
      `SampleController`로 위임하도록 배선한다:
  - [x] 생성자를 `MainMenuController(View::MainMenuView& view, Controller::ISubMenuController& sampleController)`
        로 변경하고, `sampleController_` 참조 멤버를 추가한다. (실제 구현은 code-review에서 DIP를
        지적받아 `SampleController` 구체 타입이 아니라 `Controller::ISubMenuController` 인터페이스를
        받도록 한 단계 더 강화됐다 — `SampleController`가 이 인터페이스를 구현한다.)
  - [x] `HandleInput`에서 `input == "1"`일 때 기존 `view_.ShowNotImplemented()` 대신
        `sampleController_.Run()`을 호출하도록 분기를 분리한다. "2"~"6"은 여전히
        `ShowNotImplemented()`를 유지한다.
  - [x] `main.cpp`에서 `Model::SampleRepository sampleRepository`를 `View::SampleView`,
        `Controller::SampleController`에 순서대로 주입하고, 그 `SampleController`를 다시
        `MainMenuController`에 주입하도록 배선을 갱신한다(현재 `main.cpp`는 Repository를 만들고
        `Load()`만 호출할 뿐 어떤 Controller에도 주입하지 않는 상태 — 이 배선이 비어있다는
        점이 Phase 0에서 "Repository를 Controller에 아직 주입하지 않는다"고 범위를 축소해둔
        지점이며, 실제로 이 배선을 완성하는 것이 Phase 1의 역할이다).

### 2.3 View 계층

- [x] `View/SampleView.h`/`.cpp` 신설. 콘솔 입출력만 담당하고 도메인 판단은 하지 않는다.
  - [x] `void ShowMenu()`: 시료 관리 하위 메뉴(등록/조회/검색/뒤로가기) 출력.
  - [x] 시료 등록 입력: `SampleRegistrationInput ReadRegistrationInput()`(가칭 DTO 또는 개별
        getter 조합) — id, name, avgProductionTime, yieldRate를 순서대로 입력받는다. 입력
        형식 오류(숫자 파싱 실패 등)는 View에서 재입력을 유도하되, "값의 의미적 유효성"
        (수율 범위 등)은 Model 검증 결과를 받아 View가 에러 메시지만 출력한다.
  - [x] `void ShowRegistrationResult(bool success, const std::string& message)`.
  - [x] `void ShowSampleList(const std::vector<Sample>& samples)`: 표 형태로 ID/이름/평균
        생산시간/수율/재고 출력 (PRD 10장 UI 참고: "ID / 시료명 / 평균 생산시간 / 수율 /
        현재 재고"). 목록이 비어 있으면 "등록된 시료가 없습니다" 안내.
  - [x] `std::string ReadSearchKeyword()`.
  - [x] `void ShowSearchResult(const std::vector<Sample>& results, const std::string& keyword)`:
        결과 목록 출력, 0건이면 "일치하는 시료가 없습니다" 안내.

## 3. 인터페이스/데이터 스키마

### 3.1 Sample (PRD 8.1과 동일, Phase 0에서 이미 정의된 필드 재확인용)

```
Sample {
  string id;                 // 예: "S-001"
  string name;
  double avgProductionTime;  // 평균 생산시간(분/개), > 0
  double yieldRate;          // 수율, 0.0 < yieldRate <= 1.0
  int    stock;               // 현재 재고 수량, >= 0, Phase 1 등록 시 기본 0
}
```

### 3.2 Model 검증 함수 시그니처(확정 — 기존 클래스에 추가)

```cpp
// Model/Sample.h — 기존 필드/getter/setter/ToJson/FromJson은 그대로 유지, 아래를 추가한다.
namespace Model {
class Sample {
public:
    // 기존 (변경 없음)
    Sample() = default;
    Sample(std::string id, std::string name, double avgProductionTime, double yieldRate, int stock);
    // ... 기존 getter/setter/ToJson/FromJson ...

    // Phase 1에서 신규 추가
    static bool IsValidYieldRate(double yieldRate);          // 0.0 < y <= 1.0
    static bool IsValidAvgProductionTime(double time);       // time > 0
    static bool IsValidId(const std::string& id);            // not empty (trim 후)
    static bool IsValidName(const std::string& name);        // not empty (trim 후)

    // 검증 실패 시 std::nullopt 반환(예외를 던지지 않음). stock은 항상 0으로 고정.
    static std::optional<Sample> TryCreate(
        std::string id,
        std::string name,
        double avgProductionTime,
        double yieldRate,
        int stock = 0);
};
}
```

- 기존 생성자(`Sample(id, name, avgProductionTime, yieldRate, stock)`)는 검증하지 않는 그대로
  유지한다(`FromJson` 등 내부 역직렬화 경로가 계속 사용). 신규 등록(Controller) 경로는 반드시
  `TryCreate`를 거친다.

### 3.3 Repository 인터페이스(확정 — Phase 0 실제 구현)

```cpp
// Model/Repository.h — 공용 인터페이스, Order/ProductionJob도 동일하게 상속받으므로
// Phase 1이 시그니처를 변경하지 않는다.
namespace Model {
template <typename TEntity, typename TId>
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual std::vector<TEntity> FindAll() const = 0;
    virtual std::optional<TEntity> FindById(const TId& id) const = 0;
    virtual void Add(const TEntity& entity) = 0;      // 중복 id를 거부하지 않음(그냥 push_back)
    virtual void Update(const TEntity& entity) = 0;   // id로 찾아 있으면 교체, 없으면 no-op
    virtual void Remove(const TId& id) = 0;
    virtual void Save() = 0;
    virtual void Load() = 0;
};

// Model/JsonFileRepository.h — 공용 템플릿. Add/Update/Remove가 각각 내부에서 Save()를
// 자동 호출해 즉시 파일에 flush한다.
template <typename TEntity>
class JsonFileRepository : public IRepository<TEntity, std::string> { /* ... */ };

// Model/SampleRepository.h — 기존 구현
class SampleRepository : public JsonFileRepository<Sample> {
public:
    explicit SampleRepository(std::string filePath = "data/samples.json");

    // Phase 1에서 신규 추가 (Sample 전용이므로 IRepository에는 없음)
    std::vector<Sample> SearchByName(const std::string& keyword) const;
};
}
```

- `Add`/`Update`는 `bool`이 아닌 `void`이며, 성공/실패를 반환하지 않는다. "동일 id로 재등록 시
  실패"는 Repository가 아니라 **Controller가 `Add` 호출 전 `FindById`로 중복을 먼저 확인**하는
  방식으로 구현한다(2.2 참고). 이는 `IRepository`가 Order/ProductionJob과 공유하는 범용
  템플릿이라 Phase 1이 임의로 고칠 수 없기 때문이다.
- `SearchByName`은 부분 문자열 포함 검색으로 가정(정확 일치가 아님) — PRD 7.2 "이름 등
  속성으로 특정 시료 검색"이 정확 일치를 요구하는지 불명확하므로 기본안은 부분 일치, 대소문자
  무시.

### 3.4 Controller/View 시그니처(확정)

```cpp
namespace Controller {
class SampleController {
public:
    SampleController(Model::SampleRepository& repository, View::SampleView& view);
    void Run();
private:
    void HandleRegister();
    void HandleListAll();
    void HandleSearchByName();
    Model::SampleRepository& repository_;
    View::SampleView& view_;
};

// 기존 MainMenuController 변경 — 생성자에 ISubMenuController& 추가, "1" 분기를 위임으로 변경.
// (실제 구현은 code-review에서 DIP를 지적받아, SampleController 구체 클래스가 아니라
// Controller::ISubMenuController 인터페이스에 의존하도록 한 단계 더 강화됐다.)
class ISubMenuController {
public:
    virtual ~ISubMenuController() = default;
    virtual void Run() = 0;
};

class SampleController : public ISubMenuController { /* ... 위 3.4절 상단과 동일 ... */ };

class MainMenuController {
public:
    MainMenuController(View::MainMenuView& view, Controller::ISubMenuController& sampleController);
    void Run();
    void HandleInput(const std::string& input);
    bool IsExitRequested() const;
private:
    View::MainMenuView& view_;
    Controller::ISubMenuController& sampleController_;
    bool isExitRequested_ = false;
};
}

namespace View {
class SampleView {
public:
    void ShowMenu();
    int ReadMenuChoice();
    bool ReadRegistrationInput(std::string& id, std::string& name,
                               double& avgProductionTime, double& yieldRate);
    void ShowRegistrationResult(bool success, const std::string& message);
    void ShowSampleList(const std::vector<Model::Sample>& samples);
    std::string ReadSearchKeyword();
    void ShowSearchResult(const std::vector<Model::Sample>& results,
                           const std::string& keyword);
};
}
```

## 4. 의존 관계

### 이 Phase가 전제로 삼는 Phase 0 산출물 (실제 구현 확인 완료)

- 폴더 구조: `Model/`, `View/`, `Controller/` 분리, `main → Controller → (Model, View)` 의존
  방향, Model/View 상호 비의존 — 실제로 확인됨.
- `Model::Sample` 도메인 클래스(순수 데이터 컨테이너, 3.2 참고). `Order`, `ProductionJob`은
  Phase 1에서 사용하지 않는다.
- `Model::IRepository<TEntity, TId>` 공용 인터페이스 + `Model::JsonFileRepository<TEntity>`
  공용 템플릿(즉시 flush 방식 CRUD, 3.3 참고) + `Model::SampleRepository`(기본 파일 경로
  `data/samples.json`) — 로드/저장/CRUD 및 애플리케이션 재시작 후 데이터 복원이 이미
  구현/검증돼 있다.
- GoogleTest/GMock 테스트 프로젝트가 솔루션에 연결돼 있고, `packages/gmock.1.11.0`을 참조하는
  빌드가 가능하다.
- 최소 동작하는 `main.cpp`(메인 메뉴 표시, 모든 메뉴 항목이 `ShowNotImplemented()`)와
  `Controller::MainMenuController(View::MainMenuView&)` — Phase 1은 이 생성자에
  `SampleController&` 파라미터를 추가하고 "1" 분기를 실제 위임으로 바꾼다(2.2 참고).

### 이후 Phase가 Phase 1에 기대하는 산출물

- Phase 2(시료 주문): 주문 생성 시 "등록된 시료 ID인지" 검증하기 위해 `SampleRepository::FindById`
  (상속받은 `IRepository::FindById`)를 그대로 재사용한다. Phase 1에서 이 시그니처를 바꾸면
  Phase 2가 영향을 받으므로 신중히 유지.
- Phase 3(승인/생산 큐): 승인 시점 가용 재고 계산 및 생산 완료 후 `stock` 갱신을 위해
  `SampleRepository::Update`(`IRepository::Update`, `void` 반환, id 매치 시 전체 교체)를
  사용한다.
- Phase 5(모니터링): 시료별 물리적 `stock` 조회를 위해 `SampleRepository::FindAll`을 재사용.
- Phase 6(더미 데이터 생성기): `SampleRepository::Add`(`void` 반환, 중복 미검사)를 그대로
  사용해 더미 시료를 추가한다. 더미 생성기는 Controller 계층의 중복 검사를 거치지 않고
  Repository를 직접 호출하므로, ID 충돌 방지는 더미 생성기 자체의 책임이다.

## 5. 핵심 비즈니스 로직 주의사항

Phase 1 자체에는 PRD 7.4의 "승인 시점 생산량 확정 규칙" 같은 복잡한 로직은 없다. 다만 이후
Phase가 깨지지 않도록 다음을 주의한다.

- **수율 범위 검증**: PRD는 "정상 시료 수 / 총 생산 시료 수"로 수율을 정의한다. 산술적으로는
  `0.0 <= yieldRate <= 1.0`이 가능하지만, `yieldRate = 0`이면 이후 Phase 3의 생산량 계산식
  `실 생산량 = ceil(부족분 / 수율)`이 0으로 나누기(division by zero) 오류를 일으킨다. 따라서
  Phase 1의 등록 검증에서 **`yieldRate > 0`을 반드시 강제**해 Phase 3에서 0-division이 발생할
  가능성을 원천 차단한다. 이는 Phase 1 문서에 반드시 남겨야 하는 이후 Phase와의 정합성
  포인트다.
- **stock 필드의 이중적 의미와의 혼동 방지**: CLAUDE.md/PRD 7.4에 따르면 승인 계산에는
  "가용 재고"(Phase 3에서 별도 추적)를 쓰고, 모니터링(Phase 5)에는 물리적 `stock`을 쓴다.
  Phase 1은 물리적 `stock` 필드만 다루며, "가용 재고" 개념 자체를 Phase 1에서 만들지
  않는다(Phase 3의 책임). Phase 1이 실수로 `stock`을 승인/주문 계산용으로 착각해 API를
  설계하지 않도록 주의.
- **등록된 시료만 주문 가능** 제약(PRD 7.2)의 실제 검증 로직은 Phase 2 책임이지만, Phase 1은
  `FindById`가 존재하지 않는 ID에 대해 정확히 `std::nullopt`(또는 빈 optional)을 반환하도록
  구현해 Phase 2가 이를 조건 없이 재사용할 수 있게 한다.

## 6. 테스트 계획 (tester 에이전트가 작성)

### 6.1 Model 단위 테스트 (`Model::Sample` 검증 로직)

- 정상 케이스
  - 유효한 값(id="S-001", name="Wafer-A", avgProductionTime=10.0, yieldRate=0.9)으로 `Create`
    호출 시 성공하고 필드가 그대로 저장되는지 확인.
  - 등록 시 `stock` 기본값이 0인지 확인.
- 경계값 케이스
  - `yieldRate = 1.0` → 허용(정상).
  - `yieldRate = 0.0` → 거부(0-division 방지 규칙).
  - `yieldRate = 1.0001` → 거부.
  - `yieldRate`가 음수 → 거부.
  - `avgProductionTime = 0` → 거부.
  - `avgProductionTime`이 음수 → 거부.
  - `id`가 빈 문자열/공백만 있는 문자열 → 거부.
  - `name`이 빈 문자열/공백만 있는 문자열 → 거부.

### 6.2 Repository 단위 테스트 (`SampleRepository`, 실제 JSON 파일 기반 — `IRepository`는
인터페이스이므로 GMock으로 대체하지 않고 임시 파일 경로로 실제 구현을 검증)

- `Add`로 신규 시료 추가 후 `FindById`로 동일 데이터가 조회되는지.
- `Add`는 동일 `id`를 거부하지 않는다는 실제 동작을 회귀 테스트로 고정(중복 거부는
  `SampleRepository`가 아니라 `SampleController::HandleRegister`의 책임임을 명시하는 테스트).
- `FindAll`이 등록된 개수만큼 반환하는지(0건/1건/N건).
- `SearchByName`(Phase 1 신규 추가)이 부분 일치 키워드로 올바른 부분집합을 반환하는지,
  대소문자 무시 검색 확인.
- 검색어에 해당하는 시료가 없을 때 빈 벡터 반환.
- `Add`/`Update`/`Remove` 호출 직후(별도 `Save()` 호출 없이) 파일에 즉시 반영되는지, 새
  `SampleRepository` 인스턴스로 `Load()` 했을 때 데이터가 복원되는지(재시작 시나리오).

### 6.3 Controller 단위 테스트 (GMock으로 `SampleRepository`/`SampleView`를 목으로 대체)

- `HandleRegister`: View가 유효한 입력을 반환하고 `FindById`가 `std::nullopt`(중복 없음)을
  반환하면, Repository의 `Add`가 `Sample::TryCreate`로 생성된 값과 동일한 `Sample`로 1회
  호출되는지(`EXPECT_CALL`), 성공 결과가 View에 전달되는지.
- `HandleRegister`: View가 유효하지 않은 값(예: 수율 0)을 반환하면 `TryCreate`가 실패하여
  Repository의 `Add`가 호출되지 않고 실패 메시지가 View에 전달되는지.
- `HandleRegister`: `FindById`가 기존 시료를 반환하는 경우(중복 id), `TryCreate` 성공 여부와
  무관하게 Repository의 `Add`가 호출되지 않고 "중복 id" 실패 메시지가 View에 전달되는지.
- `HandleListAll`: Repository의 `FindAll` 반환값이 그대로 `ShowSampleList`에 전달되는지.
- `HandleSearchByName`: View의 검색어 입력이 Repository의 `SearchByName`에 그대로 전달되고,
  그 결과가 `ShowSearchResult`에 전달되는지.

### 6.4 MainMenuController 배선 테스트

- `HandleInput("1")` 호출 시 `SampleController`(목)의 `Run()`이 정확히 1회 호출되는지.
- `HandleInput("2")`~`HandleInput("6")`은 여전히 `ShowNotImplemented()`를 호출하고
  `SampleController::Run()`은 호출되지 않는지(회귀 방지).

### 6.5 시스템(통합) 테스트

- 실제 JSON Repository 구현체(임시 테스트 파일 경로 사용)와 실제 Controller를 연결해:
  1) 시료 등록 → 2) 목록 조회 시 등록된 시료가 보이는지 → 3) 이름 검색으로 해당 시료가
  검색되는지 end-to-end로 확인.
- 애플리케이션 재시작을 흉내내는 테스트(Repository를 새로 생성해 같은 JSON 파일을 다시
  로드) — 등록한 시료가 남아있는지 확인 (Phase 0 영속성 요구사항과 Phase 1 기능의 결합
  확인).

## 7. 완료 조건 (Definition of Done)

- [x] `Model::Sample`에 `TryCreate`/`IsValidYieldRate`/`IsValidAvgProductionTime`/`IsValidId`/
      `IsValidName` 검증 로직(수율 `0 < y <= 1`, 생산시간 `> 0`, id/name 비어있지 않음)이
      구현되고 단위 테스트로 검증됨.
- [x] `Model::SampleRepository`에 `SearchByName`이 추가되고, 등록/전체조회/이름검색이 실제
      JSON 파일(`data/samples.json`) 기반으로 동작하며, 재시작 후에도 데이터가 유지됨.
      `Add`의 중복 id 거부는 Repository가 아닌 `SampleController::HandleRegister`가 담당함.
- [x] `Controller/SampleController`, `View/SampleView`가 MVC 계층 분리를 지키며 구현되고,
      `Controller::MainMenuController`/`main.cpp` 배선이 갱신되어 메인 메뉴에서 "1"(시료 관리)
      진입 → 등록/조회/검색이 콘솔에서 실제로 동작함. (`MainMenuController`는 `SampleController`
      구체 타입이 아니라 `Controller::ISubMenuController` 인터페이스에 의존하도록 DIP를 한 단계
      더 강화해 구현됨 — code-review 반영 사항.)
- [x] `code-review` 에이전트 통과: Clean Code, 함수 라인 수, SOLID, 디자인 패턴 사용의 적절성
      기준 충족.
- [x] `tester` 에이전트 통과: 6장에 명시한 단위/통합 테스트가 모두 작성되어 빌드 및 실행에
      성공(GREEN, 63/63).
- [x] `supervisor` 에이전트가 본 문서 및 PRD 7.2/CLAUDE.md 대비 구현 결과를 검증해 불일치가
      없음을 확인.
