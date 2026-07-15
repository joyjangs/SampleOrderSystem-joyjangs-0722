# SampleOrderSystem

가상의 반도체 회사 **"S-Semi"** 의 시료(Sample) 주문 → 승인/거절 → 생산 → 출고 전 과정을 관리하는
콘솔 기반 시료 생산주문관리 시스템입니다. 엑셀/메모장 기반 관리의 한계(주문 처리 여부 추적 불가,
생산 일정 가시성 부재, 재고-생산 현황 불일치)를 해결하기 위해 만들어졌습니다.

자세한 배경/요구사항은 [`docs/PRD.md`](docs/PRD.md), Phase별 구현 계획은
[`docs/PLAN.md`](docs/PLAN.md)와 `docs/Phase{N}.md`를 참고하세요.

## 주요 기능

콘솔 메인 메뉴 기준:

| 번호 | 기능 | 설명 |
|---|---|---|
| 1 | 시료 관리 | 시료 등록 / 전체 조회 / 이름 검색 |
| 2 | 시료 주문 | 고객 주문 접수 (`RESERVED` 상태로 생성) |
| 3 | 주문 승인/거절 | 접수 순서대로 재고 확인 후 승인(`CONFIRMED`/`PRODUCING`) 또는 거절(`REJECTED`) |
| 4 | 모니터링 | 상태별 주문 건수(`REJECTED` 제외), 시료별 재고 현황(여유/부족/고갈) |
| 5 | 생산 라인 조회 | FIFO 생산 큐 진행 상황 조회, 경과 시간 기반 자동 완료 |
| 6 | 출고 처리 | `CONFIRMED` 주문 출고(`RELEASE`) 처리 |
| 7 | 더미 데이터 생성 (테스트용) | 개발/테스트 편의를 위한 시료·주문 더미 데이터 생성 |
| 0 | 종료 | |

### 핵심 도메인 규칙

- **실 생산량 = ceil(부족분 / 수율)**, **총 생산 시간 = 평균 생산시간 × 실 생산량**
- 생산량은 **승인 시점의 가용 재고**로 1회 확정되며, 이후 생산 큐 대기 중 재고가 바뀌어도 재계산하지
  않습니다. 승인은 반드시 **주문이 들어온 순서대로** 가용 재고를 소진하며 처리합니다. 자세한 내용과
  예시는 [`docs/PRD.md`](docs/PRD.md) 7.4절, [`CLAUDE.md`](CLAUDE.md)를 참고하세요.
- 모니터링 화면은 항상 **물리적 재고**(`Sample::stock`)를 보여주며, 승인 계산에 쓰이는 가용 재고와는
  별도로 관리됩니다.

## 아키텍처

MVC 패턴을 따르며, 각 계층은 다시 기능(feature) 단위 하위 폴더로 묶여 있습니다. 같은 기능의
Model/View/Controller는 세 계층에서 동일한 하위 폴더명으로 대칭을 이룹니다.

```
Model/ View/ Controller/
├─ Sample/       시료 등록/조회/검색
├─ Order/        주문 접수·승인/거절·출고 (Order, OrderService, OrderApprovalService,
│                OrderReleaseService, AvailableStockCalculator 등)
├─ Production/   생산 큐(FIFO)·생산 진행 상황
├─ Monitoring/   상태별 주문 집계, 시료별 재고 현황
└─ DummyData/    더미 데이터 생성기

Model/, Controller/ 루트에는 특정 기능에 속하지 않는 것만 둡니다:
  Model/Repository.h, Model/JsonFileRepository.h   (공용 Repository 템플릿)
  Controller/MainMenuController.*, ISubMenuController.h, View/MainMenuView.*
```

- **Model**: 도메인 데이터와 로직(재고 계산, 생산량 공식, 상태 전이 규칙 등)
- **View**: 콘솔 입출력만 담당, 도메인 계산 없음
- **Controller**: 사용자 입력을 받아 Model을 조작하고 View를 갱신하는 흐름 제어

### 데이터 영속성

`Common::JsonValue`로 직접 구현한 의존성 없는 JSON 파서를 사용해 `data/samples.json`,
`data/orders.json`, `data/production_jobs.json`에 저장/로드합니다. 진행 중이던 주문/생산 큐
상태도 애플리케이션 재시작 후 그대로 복원됩니다. JSON 스키마는
[`docs/DATA_SCHEMA.md`](docs/DATA_SCHEMA.md) 참고.

## 빌드 및 실행

- 언어: C++20 (`stdcpp20`)
- IDE / 툴셋: Visual Studio, 플랫폼 툴셋 v145
- 솔루션: `SampleOrderSystem.slnx`, 프로젝트: `SampleOrderSystem.vcxproj`
- 빌드 구성: Debug/Release × Win32/x64

Visual Studio에서 솔루션을 열어 빌드하거나, 커맨드라인에서 MSBuild로 빌드합니다.

```
msbuild SampleOrderSystem.slnx /p:Configuration=Debug /p:Platform=x64
```

실행 파일은 `x64\Debug\SampleOrderSystem.exe`(또는 해당 구성 경로)에 생성됩니다.

### 테스트

Debug 빌드에는 GoogleTest/GMock 기반 유닛/시스템 테스트가 같은 실행 파일에 포함되어 있으며,
`--test` 인자로 실행합니다.

```
SampleOrderSystem.exe --test
```

Release 빌드에는 테스트 프레임워크가 포함되지 않습니다.

## 디렉터리 구조

```
Common/        JSON 파서, Clock, 콘솔 입력 등 공용 유틸
Model/         도메인 로직 (기능별 하위 폴더, 위 아키텍처 절 참고)
View/          콘솔 입출력 (기능별 하위 폴더)
Controller/    흐름 제어 (기능별 하위 폴더)
Tests/         GoogleTest/GMock 유닛/통합 테스트
data/          Sample/Order/ProductionJob 영속 데이터 (JSON)
docs/          PRD, Phase별 구현 계획, 데이터 스키마 문서
packages/      GoogleTest/GMock NuGet 패키지
```
