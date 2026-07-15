# Phase 6 — 더미 데이터 생성기

## 1. 목표 요약 (PLAN.md 인용)

**목표**: CLAUDE.md 핵심 구현 요소 — 테스트/개발 편의를 위한 더미 데이터.

- Sample/Order 더미 데이터를 생성해 JSON(DB)에 추가하는 기능
- 기존 Repository 구조를 그대로 재사용해 생성된 데이터가 앱에서 바로 조회 가능하게 함

**완료 기준**: 더미 데이터 생성 후 메인 메뉴 요약/모니터링 화면에 반영된다.

이 Phase는 PRD 9.4 "Dummy 데이터 생성 Tool"(선행 PoC 대상 항목)을 실제 제품 코드에 통합하는
작업이다. 별도의 임시 스크립트가 아니라, 콘솔 메뉴에서 담당자가 직접 실행할 수 있는 기능으로
구현한다.

## 2. 전제 (선행 Phase 산출물)

Phase 0~5 문서가 동시 작성 중이라 세부 시그니처가 확정되기 전이므로, 본 문서는 PLAN.md에 명시된
다음 전제를 그대로 따른다. 실제 클래스명/메서드명이 앞선 Phase 문서와 다르게 확정되면 본 문서의
해당 부분(3장 인터페이스)을 그 이름에 맞춰 갱신해야 한다 (TODO: Phase 0/1/2 문서 확정 후 이름
교차 확인 필요).

- **Model**: `Sample`(id, name, avgProductionTime, yieldRate, stock), `Order`(orderId, sampleId,
  customerName, quantity, status, createdAt, updatedAt), `ProductionJob`(orderId, sampleId,
  shortage, actualQuantity, estimatedTime, startedAt) — Phase 0에서 정의. `progress`는 저장
  필드가 아니라 Phase 3의 `ComputeProgress`로 조회 시점마다 파생 계산된다.
- **Repository**: Sample/Order/ProductionJob 각각에 대해 로드/저장/CRUD를 제공하는 Repository
  계층 — Phase 0에서 정의. 더미 데이터 생성기는 이 Repository를 그대로 통해서만 데이터를
  추가한다 (파일 직접 쓰기 금지 — JSON 스키마/직렬화 로직 중복을 피하기 위함).
- **주문 생성 로직**: Phase 2에서 구현된 "주문 접수"(시료 ID/고객명/수량 → `RESERVED` 상태 +
  주문번호 채번) 로직 — 더미 주문도 반드시 이 경로(또는 동일한 Model 계층 함수)를 통해
  생성해, 채번 규칙(`ORD-YYYYMMDD-NNNN` 등, Phase 2에서 확정된 형식)과 검증 규칙("등록된
  시료만 주문 가능")을 우회하지 않는다.
- **메인 메뉴 요약/모니터링**: Phase 5에서 구현된 집계 로직 — 더미 데이터가 Repository에
  반영되면 별도 연동 코드 없이 자동으로 최신값을 읽어와야 한다 (모니터링이 Repository를 매번
  다시 읽는 구조라면 자동 반영, 캐시를 쓰는 구조라면 캐시 무효화 지점 확인 필요 — TODO: Phase 5
  문서 확정 후 확인).
- **시료 ID 채번 기능 부재(확정)**: Phase 1(`docs/Phase1.md`)은 시료 ID를 "사용자 직접 입력 +
  Model 중복 검증" 방식으로 확정했으며 자동 채번/‘다음 ID’ 조회 기능을 제공하지 않는다. 따라서
  본 문서는 더미 생성기가 Phase 1의 (존재하지 않는) 채번 기능을 호출한다고 전제하지 않고, 더미
  생성기 자체가 후보 ID를 만들고 `SampleRepository::FindById`로 충돌을 검사하는 방식을
  전제로 한다(자세한 절차는 3장 참고). 반면 Order는 Phase 2에 실제 자동 채번기
  (`OrderIdGenerator`)가 존재하므로 그대로 재사용한다.

## 3. 작업 항목 체크리스트

### Model 계층

- [ ] `DummyDataGenerator` 클래스 신설 (`Model/DummyDataGenerator.h/.cpp`)
  - 도메인 규칙(수율 0.0~1.0, 평균 생산시간 양수 등)을 스스로 재구현하지 않고, Phase 1에서
    만든 `Sample` 생성/검증 로직과 Phase 2의 `Order` 생성 로직을 재사용한다. (더미 데이터
    생성기가 "두 번째 도메인 로직 구현체"가 되지 않도록 주의 — SRP/중복 방지)
  - 생성기 자체의 책임은 "무작위/규칙적인 입력값을 만들어 기존 생성 API에 넘기는 것"으로
    한정한다.
- [ ] 더미 시료 이름/ID 생성 규칙 정의
  - ID: **확정** — Phase 1(`docs/Phase1.md` 2.1절)은 시료 ID를 "사용자가 직접 입력하고 Model은
    중복 여부만 검증"하는 방식으로 설계되어 있으며, Phase 1에는 더미 생성기가 재사용할 수
    있는 자동 채번 기능/‘다음 ID’ 조회 기능이 존재하지 않는다. 따라서 더미 생성기는 자체
    책임으로 후보 ID를 생성하고 유일성을 직접 보장한다.
    1. `S-{n}`(n은 1부터 시작하는 순번, 예: `S-001`, `S-002`, ...) 형태로 후보 ID를 순차
       생성한다.
    2. 각 후보 ID에 대해 `SampleRepository::FindById(candidateId)`를 호출해 이미 존재하는지
       확인한다(전용 "다음 ID" API를 새로 만들지 않고 기존 조회 API만 사용).
    3. 이미 존재하면(`FindById`가 값을 반환하면) 순번을 증가시켜 다음 후보로 넘어가고, 존재하지
       않는 ID를 찾을 때까지 반복한다.
    4. 이렇게 찾은 ID를 Phase 1의 `Sample::Create(...)` 검증 함수와 `SampleRepository::Add`에
       그대로 전달한다 — 검증/저장 자체는 Phase 1 로직을 재사용하되, "어떤 ID 문자열을 쓸지
       고르는 책임"만 더미 생성기가 진다.
    5. 무한 루프 방지를 위해 시도 횟수 상한(예: `sampleCount`의 10배 또는 별도 상수)을 두고,
       상한 초과 시 해당 시료 생성을 실패로 처리하고 `failureReasons`에 사유를 남긴다.
  - 이름: `더미시료-{n}` 형태 혹은 미리 정의한 후보 이름 목록에서 순환 선택
  - 평균 생산시간/수율: 옵션으로 받은 범위 내 난수 (기본 범위: 평균 생산시간 1~120(min/ea),
    수율 0.5~1.0) — 범위를 벗어나는 값(수율 0 또는 1 초과 등)이 생성되지 않도록 Phase 1의
    검증 로직을 통과시켜 생성
  - 초기 재고(stock): 옵션으로 받은 범위 내 난수 (기본 0~100), 0을 포함해 "고갈" 상태 테스트가
    가능하게 함
- [ ] 더미 주문 생성 규칙 정의
  - 대상 시료: 현재 Repository에 존재하는 시료 중에서만 무작위 선택 (없으면 생성 실패 —
    "등록된 시료만 주문 가능" 규칙 준수). 더미 시료를 먼저 생성한 결과가 있다면 그것도 후보에
    포함
  - 고객명: 미리 정의한 더미 고객명 후보 목록(`고객A`, `고객B`, ... 또는 `Customer-{n}`)에서
    순환/무작위 선택
  - 수량: 옵션으로 받은 범위 내 난수 (기본 1~50)
  - **확정** — 더미 주문은 항상 `RESERVED` 상태로만 생성한다(Phase 2 규칙과 동일). 더미
    생성기가 임의로 `CONFIRMED`/`PRODUCING`/`RELEASE` 등 다른 상태를 직접 만들어 넣거나,
    Phase 3/4의 승인·생산·출고 로직을 재실행해 상태를 전이시키는 기능은 이 Phase의 범위에서
    **제외**한다. 다양한 상태의 더미 주문이 필요하면 담당자가 더미 `RESERVED` 주문 생성 후
    콘솔의 주문 승인/생산/출고 메뉴를 수동으로 이용해야 하며, 이는 더미 데이터 생성기 자체의
    책임이 아니다.
  - 채번: Phase 2의 `OrderIdGenerator`(PRD 8.2 `ORD-YYYYMMDD-NNNN` 형식, 당일 발급 건수 기준
    순번 부여)는 Sample ID와 달리 이미 자동 채번 기능으로 존재하므로, 더미 생성기는 이 기능을
    그대로 호출해 주문번호를 얻는다. 별도의 자체 후보 생성/충돌 검사 로직을 새로 만들 필요는
    없다(채번기 자체가 이미 유일성을 보장).
  - 난수 시드(seed)를 옵션으로 받아, 동일 시드로 재현 가능한 더미 데이터 생성을 지원 (테스트
    용이성 목적)
- [ ] `DummyDataGenerator`가 생성 개수/시드/값 범위를 담은 옵션 구조체(`DummyDataOptions`)를
  입력받아 동작하도록 설계 (하드코딩된 개수 대신 호출부에서 조절 가능하게)

### Controller 계층

- [ ] `DummyDataController` (또는 기존 메인 메뉴 라우팅 구조에 맞는 명칭 — 예:
  `MainMenuController`의 하위 액션) 신설
  - 콘솔에서 "더미 데이터 생성" 메뉴 진입 시 사용자에게 생성 개수(시료 N개/주문 M개)와 시드값
    입력을 받는다 (미입력 시 기본값 사용 — 기본값은 4장 참고)
  - 입력값 검증 (음수/0 개수 등 처리 — 0개는 "생성 안 함"으로 허용하되 음수는 재입력 요청)
  - `DummyDataGenerator` 호출 후 결과를 Repository에 저장 (Repository의 기존 CRUD API 사용,
    저장 방식은 Phase 0에서 정의한 방식과 동일하게 유지)
  - 생성 결과 요약(생성된 시료 수/주문 수, 실패 건수 등)을 View에 전달

### View 계층

- [ ] 더미 데이터 생성 메뉴 화면 추가
  - 입력 프롬프트: "생성할 시료 개수", "생성할 주문 개수", "난수 시드 (미입력 시 자동)"
  - 결과 출력: 생성된 시료 ID 목록, 생성된 주문번호 목록(또는 개수 요약), 실패한 항목이 있다면
    사유(예: "등록된 시료가 없어 주문 생성 불가")
- [ ] 메인 메뉴에 "더미 데이터 생성" 항목 추가 (PRD 7.1 메뉴 목록에는 명시되어 있지 않은
  개발/테스트 편의 기능이므로, 운영 메뉴와 구분되도록 메뉴 문구에 "테스트용" 등 표기 — TODO:
  정식 메뉴 항목 번호에 포함할지, 별도의 개발자 전용 서브 메뉴로 숨길지는 사용자 확인 필요.
  기본안은 메인 메뉴 하단에 "9. 더미 데이터 생성 (테스트용)"처럼 별도 항목으로 노출)

## 4. 인터페이스/데이터 스키마

이 Phase는 `Sample`/`Order`/`ProductionJob`의 스키마를 변경하지 않는다 (PRD 8장 그대로 사용).
새로 추가되는 것은 아래 옵션/생성기 인터페이스뿐이다.

```cpp
// Model/DummyDataOptions.h
struct DummyDataOptions {
    int sampleCount = 5;          // 생성할 더미 시료 개수 (기본값)
    int orderCount = 10;          // 생성할 더미 주문 개수 (기본값)
    unsigned int seed = 0;        // 0이면 실행 시각 기반 자동 시드 사용
    double minYieldRate = 0.5;    // 더미 시료 수율 하한 (0.0 < min <= max <= 1.0)
    double maxYieldRate = 1.0;
    int minAvgProductionTime = 1; // 더미 시료 평균 생산시간 하한 (min/ea)
    int maxAvgProductionTime = 120;
    int minInitialStock = 0;      // 더미 시료 초기 재고 하한
    int maxInitialStock = 100;
    int minOrderQuantity = 1;     // 더미 주문 수량 하한
    int maxOrderQuantity = 50;
};

// Model/DummyDataGenerator.h
struct DummyDataResult {
    std::vector<std::string> createdSampleIds;
    std::vector<std::string> createdOrderIds;
    std::vector<std::string> failureReasons; // 예: 생성 불가 사유 누적
};

class DummyDataGenerator {
public:
    DummyDataGenerator(SampleRepository& sampleRepo, OrderRepository& orderRepo);

    // 옵션에 따라 더미 Sample/Order를 생성하고 Repository에 저장한 뒤 결과를 반환한다.
    // Sample: ID는 생성기가 "S-{n}" 후보를 스스로 순차 생성하고 SampleRepository::FindById로
    //         충돌을 검사해 결정한 뒤, Phase 1의 Sample::Create 검증/SampleRepository::Add를
    //         그대로 호출한다(자동 채번 기능이 없는 Phase 1을 그대로 재사용하지 않는다).
    // Order: Phase 2의 주문 접수 로직(OrderIdGenerator 포함)을 그대로 호출해 항상 RESERVED
    //         상태로만 생성한다.
    DummyDataResult Generate(const DummyDataOptions& options);
};
```

> 위 시그니처의 `SampleRepository`/`OrderRepository` 이름은 Phase 0 문서에서 확정된 실제
> Repository 클래스명을 따른다 (TODO: Phase 0 문서 확정 후 명칭 일치 여부 확인).

## 5. 의존 관계

- **선행 의존**: Phase 0(Repository/도메인 모델), Phase 1(시료 등록 검증 로직), Phase 2(주문
  접수 로직 및 채번 규칙), Phase 5(모니터링/메인 메뉴 요약 집계 — 정확히는 이 Phase의 완료
  기준을 확인하는 데만 의존하며, 더미 생성기 자체 구현이 Phase 5 코드를 직접 호출하지는 않는다).
- **후속 기대**: Phase 7(통합 마무리)에서 전체 시스템 테스트 시 더미 데이터 생성기를 이용해
  대량의 Sample/Order를 만들어 놓고 승인/생산/출고/모니터링 흐름을 검증하는 시나리오에 활용할
  수 있어야 한다. 따라서 이 Phase의 산출물은 "실행 후 콘솔 종료 → 재시작해도 생성된 데이터가
  JSON에 남아있어야" 한다 (Phase 0의 영속성 요구사항을 그대로 상속).

## 6. 핵심 비즈니스 로직 주의사항

이 Phase 자체는 PRD의 생산량 계산/상태 전이 같은 새로운 비즈니스 규칙을 추가하지 않는다. 다만
기존 규칙을 우회하지 않도록 다음을 반드시 지킨다.

- **시료 검증 규칙 재사용**: 더미 시료도 Phase 1에서 정의한 시료 등록 검증(수율 0.0~1.0 범위,
  평균 생산시간 양수 등)을 그대로 통과해야 한다. 생성기가 옵션으로 받은 범위 자체가 잘못돼도
  (예: `minYieldRate > maxYieldRate`) 검증에서 걸러지도록 한다.
- **"등록된 시료만 주문 가능" 규칙 (PRD 7.2/2)**: 더미 주문은 반드시 Repository에 이미 존재하는
  시료(직전에 생성한 더미 시료 포함)에 대해서만 생성한다. 등록된 시료가 하나도 없는 상태에서
  더미 주문만 생성하려는 시도는 실패 처리하고 사유를 결과에 남긴다.
- **주문 상태는 항상 `RESERVED`로만 생성 (확정)** (PRD 6.3 상태 흐름도, 7.3): 더미 생성기가
  `CONFIRMED`/`PRODUCING`/`RELEASE` 상태의 주문을 직접 만들어 넣거나, Phase 3/4의
  승인·거절·생산·출고 로직을 재실행해 상태를 전이시키는 기능은 이 Phase의 범위에 포함하지
  않는다. 다양한 상태의 더미 데이터가 필요하면 콘솔의 정규 메뉴(주문 승인/생산 라인/출고 처리)를
  통해 수동으로 전이시켜야 한다.
- **물리적 stock vs 가용 재고 혼동 금지 (PRD 7.4)**: 더미 시료 생성 시 설정하는 `stock`은
  물리적 재고이며, 승인 로직(Phase 3)의 "가용 재고" 개념과 무관하다. 더미 데이터 생성기는
  가용 재고 추적 자료구조를 직접 건드리지 않는다(존재 자체를 몰라도 된다).
- **시료 ID 채번 방식(확정)**: Phase 1은 시료 ID를 "사용자 직접 입력 + Model의 중복 검증"
  방식으로 확정했고 자동 채번 기능을 제공하지 않는다. 따라서 더미 생성기는 Phase 1의
  존재하지 않는 채번 기능을 호출하는 대신, `S-{n}` 형태의 후보 ID를 스스로 순차 생성하고
  `SampleRepository::FindById`로 기존 시료와의 충돌 여부를 직접 검사해 유일성을 확보한다
  (충돌 시 다음 후보로 이동). 이렇게 정한 ID만 Phase 1의 검증/저장 로직(`Sample::Create`,
  `SampleRepository::Add`)에 전달한다.
- **주문 ID 채번 방식**: Order는 Sample과 달리 Phase 2에 실제 자동 채번기(`OrderIdGenerator`,
  `ORD-YYYYMMDD-NNNN`)가 이미 존재하므로, 더미 생성기는 이 채번기를 그대로 호출해 주문번호를
  얻는다. 이 경우는 채번기 자체가 유일성을 보장하므로 Sample처럼 생성기가 별도의 충돌 검사
  로직을 만들 필요가 없다.

## 7. 테스트 계획 (tester 에이전트)

### 단위 테스트

- `DummyDataGenerator::Generate`
  - 정상 케이스: `sampleCount=5, orderCount=10`으로 호출 시 `createdSampleIds.size()==5`,
    `createdOrderIds.size()==10`, 모든 생성된 시료가 Repository에서 조회 가능한지 확인
  - 생성된 시료의 수율이 항상 `[minYieldRate, maxYieldRate]` 범위 내인지 확인 (경계값:
    `minYieldRate==maxYieldRate`로 고정값 생성 케이스)
  - 생성된 시료의 평균 생산시간/초기 재고가 지정 범위 내인지 확인
  - 생성된 주문의 `status`가 모두 `RESERVED`인지 확인
  - 생성된 주문의 `sampleId`가 모두 (생성 시점 기준) Repository에 존재하는 시료 ID 중 하나인지
    확인
  - 경계값: `sampleCount=0, orderCount=0` → 아무것도 생성되지 않고 정상 종료(에러 아님)
  - 경계값: 등록된 시료가 하나도 없는 상태에서 `orderCount>0`으로 호출 → 주문 생성 실패,
    `failureReasons`에 사유 기록, 예외로 프로그램이 죽지 않음
  - 동일 시드로 두 번 `Generate` 호출 시 (매번 새 Repository 인스턴스 기준) 생성되는 값의
    분포/개수가 재현되는지 확인. 시료 ID(`S-{n}` 순차 채번)는 Repository 초기 상태가 같다면
    완전히 동일하게 재현되어야 하므로 ID까지 비교 대상에 포함한다. 주문번호(`ORD-YYYYMMDD-NNNN`)
    는 실행 날짜에 좌우되므로 ID 문자열 자체의 재현은 검증 대상에서 제외하고, 수량/시료 참조
    등 "값의 재현"만 확인한다.
- 시료 ID 자체 채번/충돌 회피 확인: `SampleRepository`에 이미 `S-001`~`S-003`이 존재하는
  상태에서 `Generate`를 호출하면, 새로 생성되는 시료 ID가 기존 ID와 절대 겹치지 않고
  (`FindById`로 사전 존재 여부를 실제 확인) `S-004`부터 이어지는 등 일관된 규칙으로 채번되는지
  확인 (GMock으로 `SampleRepository::FindById`가 후보 ID마다 호출되는지도 함께 검증)
  - 경계값: 사전에 존재하는 ID들이 연속되지 않고 듬성듬성 비어 있는 경우(`S-001`, `S-003`만
    존재)에도 생성기가 이미 존재하는 후보를 건너뛰고 정상적으로 유일한 ID를 찾는지 확인
- 주문 ID 채번 확인: `DummyDataGenerator`가 생성한 주문번호가 Phase 2의 `OrderIdGenerator`가
  만든 형식(`ORD-YYYYMMDD-NNNN`)과 동일한지, 그리고 실제로 Phase 2의 채번기를 통해 발급되어
  기존 주문번호와 중복되지 않는지 확인

### 통합(시스템) 테스트

- 더미 데이터 생성 → 애플리케이션 재시작(Repository 재로드) → 생성된 시료/주문이 그대로
  조회되는지 확인 (Phase 0 영속성 요구사항 재검증)
- 더미 데이터 생성 후 메인 메뉴 요약(등록 시료 수/총 재고/전체 주문 수)이 생성 전 대비 정확히
  증가했는지 확인
- 더미 데이터 생성 후 모니터링 화면의 상태별 주문 건수(`RESERVED` 건수만 증가, `REJECTED`
  제외 집계에 영향 없음)와 시료별 재고 현황(신규 시료가 여유/부족/고갈 중 올바른 상태로
  표기되는지, 특히 초기 재고 0으로 생성된 시료가 "고갈"로 표시되는지)이 올바른지 확인
- 콘솔 메뉴 경로로 실제 입력(개수/시드)을 흉내 낸 E2E 시나리오: 메인 메뉴 → 더미 데이터 생성
  메뉴 진입 → 개수 입력 → 결과 출력 확인

## 8. 완료 조건 (Definition of Done)

- `DummyDataGenerator`(Model)와 이를 호출하는 Controller/View 메뉴가 구현되고 빌드된다.
- 더미 데이터 생성 기능이 Phase 0의 Repository CRUD API만을 통해 데이터를 추가하며, 별도의
  JSON 파일 직접 조작 코드가 없다.
- 더미 시료가 Phase 1의 검증 로직(`Sample::Create` 등)을 그대로 통과해 생성되며, ID는 더미
  생성기가 자체적으로 후보를 만들고 `SampleRepository::FindById`로 충돌을 확인해 결정한다
  (Phase 1에 존재하지 않는 자동 채번 기능 호출을 전제하지 않는다).
- 더미 주문이 Phase 2의 검증·채번 로직(`OrderIdGenerator` 포함)을 그대로 통과해 생성되며,
  항상 `RESERVED` 상태로만 생성된다(등록되지 않은 시료에 대한 더미 주문은 생성되지 않으며,
  더미 생성기가 승인/생산/출고 로직을 재실행해 다른 상태로 전이시키는 기능은 구현하지 않는다).
- 생성된 더미 데이터가 프로그램 재시작 후에도 유지된다.
- 메인 메뉴 요약과 모니터링 화면(Phase 5)이 더미 데이터 생성 직후 값을 정확히 반영한다.
- 7장 목록의 단위/통합 테스트가 GoogleTest/GMock으로 작성되어 통과한다 (`tester` 에이전트).
- `code-review` 에이전트가 Clean Code/함수 라인 수/SOLID/디자인 패턴 관점에서 지적한 사항이
  없거나 반영 완료됐다.
- `supervisor` 에이전트가 본 문서(Phase6.md)와 실제 구현/테스트 결과의 일치 여부를 검증하고
  이상 없음을 확인한다.
