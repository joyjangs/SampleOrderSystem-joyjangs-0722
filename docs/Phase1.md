# Phase 1 — 시료 관리

## 0. 문서 상태 메모

- 본 문서 작성 시점에 `docs/Phase0.md`는 아직 존재하지 않는다(별도 에이전트가 동시 작성 중).
  따라서 아래 "의존 관계"에 적은 Phase 0 산출물(도메인 모델/Repository 시그니처)은
  `docs/PLAN.md`의 Phase 0 설명과 `CLAUDE.md`의 아키텍처 원칙을 근거로 한 **기대치**이며,
  실제 `docs/Phase0.md` 및 구현 결과가 확정되면 이름/시그니처가 다를 수 있다. 그 경우 이
  문서를 Phase 0 산출물에 맞춰 갱신해야 한다.
- 현재 소스 트리에는 `Model/`, `View/`, `Controller/`, `main.cpp` 등이 전혀 존재하지 않는다.
  Phase 1은 Phase 0이 만든 뼈대(폴더 구조, 도메인 모델, JSON Repository, 테스트 하네스,
  최소 `main.cpp`) 위에서 시료 관리 기능을 수직으로 완성하는 것을 전제로 한다.

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

- [ ] (Phase 0 산출물 확인) `Model/Sample.h`/`.cpp`에 `Sample` 클래스가 이미 정의돼 있는지
      확인한다. 없다면 Phase 0 완료를 먼저 확인 후 착수한다.
- [ ] `Sample` 클래스에 다음 필드가 있는지 확인/보강 (PRD 8.1과 일치해야 함):
      `id`(string), `name`(string), `avgProductionTime`(number, 분 단위), `yieldRate`(0.0~1.0),
      `stock`(number, 정수, 기본 0).
- [ ] Sample 값 검증 로직을 Model 계층에 둔다 (View/Controller에 검증 로직이 새지 않도록):
  - [ ] `Sample::IsValidYieldRate(double)` 또는 생성자/팩토리 내부 검증: `0.0 < yieldRate <= 1.0`
        만 허용 (0 이하/1 초과는 거부). — "핵심 비즈니스 로직 주의사항" 참고.
  - [ ] `Sample::IsValidAvgProductionTime(double)`: 평균 생산시간은 0보다 커야 한다(0 이하 거부).
  - [ ] `id`/`name`은 공백이 아닌 문자열이어야 한다(trim 후 empty 거부).
  - [ ] `stock`은 등록 시 음수를 허용하지 않는다(기본값 0, 사용자가 초기 재고를 입력하는
        경우에도 0 이상만 허용). — TODO: 확인 필요. PRD 7.2에는 "시료 등록" 입력 항목에
        재고(stock)가 명시돼 있지 않고 8.1 데이터 모델에만 `stock` 필드가 있다. Phase 1
        구현 시 등록 화면에서 초기 재고를 입력받을지, 항상 0으로 시작할지는 사용자 확인이
        필요하다. 본 문서는 잠정적으로 "등록 시 초기 재고는 0으로 고정하고, 재고 증가는
        Phase 3(생산 완료)에서만 발생시킨다"는 원칙을 기본안으로 채택한다(CLAUDE.md의
        "물리적 stock은 실제 생산완료/출고 시점에만 변경한다" 원칙과 가장 정합적이므로).
        실무적으로 초기 입고 재고를 입력해야 한다면 별도 지시 필요.
- [ ] `SampleValidationException`(또는 Phase 0에서 정한 공통 예외/Result 타입)을 재사용해
      검증 실패를 표현한다. Phase 0이 예외 기반인지 `std::optional`/`Result<T,Error>` 기반인지
      확인 후 그 방식을 그대로 따른다(Phase 1에서 새로운 오류 처리 관례를 만들지 않는다).
- [ ] (Phase 0 산출물 확인) `SampleRepository`(가칭)에 다음 인터페이스가 있는지 확인/보강:
  - `bool Add(const Sample&)` — 중복 `id`면 실패
  - `std::optional<Sample> FindById(const std::string& id) const`
  - `std::vector<Sample> FindAll() const`
  - `std::vector<Sample> SearchByName(const std::string& keyword) const` — 부분 일치(대소문자
    무시 여부는 TODO: 확인 필요, 기본안은 대소문자 무시 부분 문자열 포함 검색)
  - `bool Update(const Sample&)` (재고 갱신 등 후속 Phase에서 사용 — Phase 1에서 직접
    호출하지 않아도 시그니처만 확정)
  - Repository는 내부적으로 JSON 파일 로드/저장을 담당하며, Phase 1은 이 인터페이스만
    사용하고 JSON 직렬화 세부사항을 다시 구현하지 않는다.
- [ ] 시료 ID 채번 규칙 확인: PRD 예시는 `S-001` 형식. 사용자가 직접 ID를 입력하는지,
      시스템이 자동 채번하는지 PRD 7.2에는 명시가 없다 — TODO: 확인 필요. 기본안: **사용자가
      직접 ID를 입력**하고 Model이 중복 여부만 검증한다(주문 흐름에서 `Sample ID`를 사람이
      알아야 참조 가능하므로 자동 채번보다 수동 입력이 자연스럽다). 자동 채번을 원하면 별도
      지시 필요.

### 2.2 Controller 계층

- [ ] `Controller/SampleController.h`/`.cpp` 신설.
  - [ ] 생성자에서 `SampleRepository&`(또는 Phase 0이 제공하는 Repository 인터페이스 참조/
        스마트 포인터)와 `SampleView&`를 주입받는다(생성자 주입, DI로 테스트 용이성 확보).
  - [ ] `void Run()` 또는 `void ShowMenu()`: 시료 관리 하위 메뉴 루프 진입점. 메인 메뉴
        Controller가 이 메서드를 호출해 하위 메뉴로 위임한다.
  - [ ] `void HandleRegister()`: View로부터 입력을 받아 `Sample` 생성 시도 → 검증 실패 시
        View에 에러 메시지 출력, 성공 시 Repository에 저장 후 결과 출력.
  - [ ] `void HandleListAll()`: Repository에서 전체 목록을 가져와 View에 위임해 출력.
  - [ ] `void HandleSearchByName()`: View로부터 검색어를 입력받아 Repository 검색 후 결과를
        View에 위임해 출력(검색 결과 0건인 경우도 처리).
  - [ ] Controller는 입력 파싱/흐름 제어만 하고, 수율/생산시간 유효성 판단 로직 자체는 갖지
        않는다(Model에 위임) — CLAUDE.md 아키텍처 원칙 준수.
- [ ] 메인 메뉴 Controller(Phase 0에서 만든 최소 골격)에 "시료 관리" 메뉴 항목을 추가해
      `SampleController::Run()`으로 라우팅한다. (메인 메뉴 자체의 요약 정보 갱신은 Phase 5
      범위이므로 Phase 1에서는 메뉴 진입 연결만 한다.)

### 2.3 View 계층

- [ ] `View/SampleView.h`/`.cpp` 신설. 콘솔 입출력만 담당하고 도메인 판단은 하지 않는다.
  - [ ] `void ShowMenu()`: 시료 관리 하위 메뉴(등록/조회/검색/뒤로가기) 출력.
  - [ ] 시료 등록 입력: `SampleRegistrationInput ReadRegistrationInput()`(가칭 DTO 또는 개별
        getter 조합) — id, name, avgProductionTime, yieldRate를 순서대로 입력받는다. 입력
        형식 오류(숫자 파싱 실패 등)는 View에서 재입력을 유도하되, "값의 의미적 유효성"
        (수율 범위 등)은 Model 검증 결과를 받아 View가 에러 메시지만 출력한다.
  - [ ] `void ShowRegistrationResult(bool success, const std::string& message)`.
  - [ ] `void ShowSampleList(const std::vector<Sample>& samples)`: 표 형태로 ID/이름/평균
        생산시간/수율/재고 출력 (PRD 10장 UI 참고: "ID / 시료명 / 평균 생산시간 / 수율 /
        현재 재고"). 목록이 비어 있으면 "등록된 시료가 없습니다" 안내.
  - [ ] `std::string ReadSearchKeyword()`.
  - [ ] `void ShowSearchResult(const std::vector<Sample>& results, const std::string& keyword)`:
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

### 3.2 Model 검증 함수 시그니처(제안)

```cpp
namespace Model {
class Sample {
public:
    static bool IsValidYieldRate(double yieldRate);          // 0.0 < y <= 1.0
    static bool IsValidAvgProductionTime(double time);       // time > 0
    static bool IsValidId(const std::string& id);            // not empty (trim 후)
    static bool IsValidName(const std::string& name);        // not empty (trim 후)

    // 검증 실패 시 std::optional<Sample> 반환 또는 예외 throw — Phase 0의 관례를 따른다.
    static std::optional<Sample> Create(
        const std::string& id,
        const std::string& name,
        double avgProductionTime,
        double yieldRate,
        int initialStock = 0);
};
}
```

> Phase 0에서 이미 `Sample`을 POD/애그리게이트 형태로 정의했다면, 위 정적 검증 함수만 Phase 1에서
> 추가하는 방식으로 통합한다. Phase 0 산출물이 다른 이름의 팩토리/검증 규약을 갖고 있다면 이
> 문서를 그에 맞춰 갱신할 것.

### 3.3 Repository 인터페이스(제안, Phase 0 산출물과 통합 필요)

```cpp
namespace Model {
class SampleRepository {
public:
    virtual ~SampleRepository() = default;
    virtual bool Add(const Sample& sample) = 0;
    virtual bool Update(const Sample& sample) = 0;
    virtual std::optional<Sample> FindById(const std::string& id) const = 0;
    virtual std::vector<Sample> FindAll() const = 0;
    virtual std::vector<Sample> SearchByName(const std::string& keyword) const = 0;
};
}
```

- Phase 1은 이 인터페이스의 구체 구현(JSON 파일 기반)이 Phase 0에서 제공된다고 가정한다.
  구현체 이름(`JsonSampleRepository` 등)은 Phase 0 문서 확정 후 본 문서에 반영한다.
- `SearchByName`은 부분 문자열 포함 검색으로 가정(정확 일치가 아님) — PRD 7.2 "이름 등
  속성으로 특정 시료 검색"이 정확 일치를 요구하는지 불명확하므로 기본안은 부분 일치.

### 3.4 Controller/View 시그니처(제안)

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

### 이 Phase가 전제로 삼는 Phase 0 산출물

- 폴더 구조: `Model/`, `View/`, `Controller/` 분리, `main → Controller → (Model, View)` 의존
  방향, Model/View 상호 비의존.
- `Model::Sample` 도메인 클래스(및 `Order`, `ProductionJob`은 Phase 1에서 사용하지 않음).
- JSON 기반 `SampleRepository`(또는 동등 이름) 구현체 — 로드/저장/CRUD 및 애플리케이션
  재시작 후 데이터 복원이 이미 검증돼 있어야 한다.
- GoogleTest/GMock 테스트 프로젝트가 솔루션에 연결돼 있고, `packages/gmock.1.11.0`을 참조하는
  빌드가 가능해야 한다.
- 최소 동작하는 `main.cpp`(빈 메인 메뉴 표시) — Phase 1은 여기에 "시료 관리" 메뉴 진입점을
  추가한다.

### 이후 Phase가 Phase 1에 기대하는 산출물

- Phase 2(시료 주문): 주문 생성 시 "등록된 시료 ID인지" 검증하기 위해 `SampleRepository::FindById`
  를 그대로 재사용한다. Phase 1에서 이 시그니처를 바꾸면 Phase 2가 영향을 받으므로 신중히 유지.
- Phase 3(승인/생산 큐): 승인 시점 가용 재고 계산 및 생산 완료 후 `stock` 갱신을 위해
  `SampleRepository::Update`(또는 동등 기능)를 사용한다. Phase 1에서 `Update`가 `stock`을
  포함한 전체 필드를 갱신하는 방식으로 구현되어야 Phase 3이 그대로 재사용 가능하다.
- Phase 5(모니터링): 시료별 물리적 `stock` 조회를 위해 `SampleRepository::FindAll`을 재사용.
- Phase 6(더미 데이터 생성기): `SampleRepository::Add`를 그대로 사용해 더미 시료를 추가한다.

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

### 6.2 Repository 단위 테스트 (`SampleRepository`, Mock 또는 JSON 파일 기반, GMock 활용)

- `Add`로 신규 시료 추가 후 `FindById`로 동일 데이터가 조회되는지.
- 동일 `id`로 `Add` 재시도 시 실패(중복 거부) 확인.
- `FindAll`이 등록된 개수만큼 반환하는지(0건/1건/N건).
- `SearchByName`이 부분 일치 키워드로 올바른 부분집합을 반환하는지, 대소문자 무시 여부(정책에
  따라 케이스 추가).
- 검색어에 해당하는 시료가 없을 때 빈 벡터 반환.

### 6.3 Controller 단위 테스트 (GMock으로 `SampleRepository`/`SampleView`를 목으로 대체)

- `HandleRegister`: View가 유효한 입력을 반환하면 Repository의 `Add`가 정확한 `Sample`
  값으로 1회 호출되는지(`EXPECT_CALL`), 성공 결과가 View에 전달되는지.
- `HandleRegister`: View가 유효하지 않은 값(예: 수율 0)을 반환하면 Repository의 `Add`가
  호출되지 않고 실패 메시지가 View에 전달되는지.
- `HandleListAll`: Repository의 `FindAll` 반환값이 그대로 `ShowSampleList`에 전달되는지.
- `HandleSearchByName`: View의 검색어 입력이 Repository의 `SearchByName`에 그대로 전달되고,
  그 결과가 `ShowSearchResult`에 전달되는지.

### 6.4 시스템(통합) 테스트

- 실제 JSON Repository 구현체(임시 테스트 파일 경로 사용)와 실제 Controller를 연결해:
  1) 시료 등록 → 2) 목록 조회 시 등록된 시료가 보이는지 → 3) 이름 검색으로 해당 시료가
  검색되는지 end-to-end로 확인.
- 애플리케이션 재시작을 흉내내는 테스트(Repository를 새로 생성해 같은 JSON 파일을 다시
  로드) — 등록한 시료가 남아있는지 확인 (Phase 0 영속성 요구사항과 Phase 1 기능의 결합
  확인).

## 7. 완료 조건 (Definition of Done)

- [ ] `Model::Sample`(또는 Phase 0에서 정의된 이름)에 등록 검증 로직(수율 `0 < y <= 1`,
      생산시간 `> 0`, id/name 비어있지 않음)이 구현되고 단위 테스트로 검증됨.
- [ ] `SampleRepository`(또는 동등 인터페이스)를 통한 등록/전체조회/이름검색이 실제 JSON
      파일 기반으로 동작하고, 재시작 후에도 데이터가 유지됨.
- [ ] `Controller/SampleController`, `View/SampleView`가 MVC 계층 분리를 지키며 구현되고,
      메인 메뉴에서 "시료 관리" 진입 → 등록/조회/검색이 콘솔에서 실제로 동작함.
- [ ] `code-review` 에이전트 통과: Clean Code, 함수 라인 수, SOLID, 디자인 패턴 사용의 적절성
      기준 충족.
- [ ] `tester` 에이전트 통과: 6장에 명시한 단위/통합 테스트가 모두 작성되어 빌드 및 실행에
      성공(GREEN).
- [ ] `supervisor` 에이전트가 본 문서 및 PRD 7.2/CLAUDE.md 대비 구현 결과를 검증해 불일치가
      없음을 확인.
