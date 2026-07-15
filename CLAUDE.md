# CLAUDE.md

이 문서는 Claude Code가 SampleOrderSystem 프로젝트에서 작업할 때 참고하는 가이드입니다.

## 프로젝트 개요

자세한 내용은 [`docs/PRD.md`](docs/PRD.md) 참고. 요약:

**반도체 시료 생산주문관리 시스템** — 가상 반도체 회사 "S-Semi"의 시료(Sample) 주문/생산/출고
전 과정을 관리하는 콘솔 애플리케이션. 기존 엑셀/메모장 기반 관리의 한계(주문 처리 여부 추적 불가,
생산 일정 가시성 부재, 재고-생산 현황 불일치)를 해결하는 것이 목표.

- **대상 플랫폼**: 콘솔(Console) 애플리케이션, MVC 아키텍처
- **핵심 엔티티**
  - `Sample` (시료): id, name, avgProductionTime(평균 생산시간), yieldRate(수율), stock(재고)
  - `Order` (주문): orderId, sampleId, customerName, quantity, status, createdAt, updatedAt
  - `ProductionJob` (생산 큐 작업): orderId, sampleId, shortage(부족분), actualQuantity(실 생산량),
    estimatedTime, progress
- **주문 상태 흐름**: `RESERVED`(접수) → 승인/거절 판단
  - 거절 → `REJECTED` (모니터링 집계 제외)
  - 승인 + 재고 충분 → `CONFIRMED` (출고 대기)
  - 승인 + 재고 부족 → `PRODUCING` (생산 큐 등록, FIFO) → 생산 완료 시 `CONFIRMED`
  - `CONFIRMED` → 출고 처리 → `RELEASE`
- **주요 기능**: 시료 관리(등록/조회/검색), 시료 주문(예약), 주문 승인/거절, 모니터링(상태별 주문
  건수, 시료별 재고 현황: 여유/부족/고갈), 생산 라인(FIFO 큐, 진행 상황), 출고 처리
- **생산량 계산 공식**
  ```
  실 생산량 = ceil(부족분 / 수율)
  총 생산 시간 = 평균 생산시간 × 실 생산량
  ```
- **⚠️ 승인 시점 생산량 확정 규칙 (구현 시 놓치기 쉬운 핵심 규칙, PRD 7.4 참고)**
  - 위 계산은 **주문을 승인하는 시점의 재고만 고려**해 1회 확정하며, 생산 큐 대기 후 실제
    생산이 시작될 때 재고를 다시 계산하지 않는다. `ProductionJob`의 `shortage`/
    `actualQuantity`/`estimatedTime`은 승인 시점에 저장된 값을 그대로 사용한다.
  - 승인은 **주문이 들어온 순서대로 재고를 소진**하며 계산해야 한다. 물리적 재고가 남아있어도
    그 재고가 이미 앞선 주문의 출고분으로 예정돼 있으면, 뒤 주문은 그만큼을 가용 재고로 볼 수
    없다. 즉 시료의 물리적 `stock` 값을 승인 즉시 차감하는 방식으로는 이 규칙을 만족할 수
    없고, "아직 배정되지 않은 가용 재고"를 별도로 추적해야 한다. 물리적 `stock`은 실제 생산
    완료/출고 시점에만 변경한다.
  - 모니터링 화면에는 항상 물리적 `stock`(실제 재고)을 표시한다 — 승인 계산용 가용 재고와
    혼동하지 않는다.
  - 예시(PRD 7.4 참고): 재고 0·수율 0.5인 시료에 순서대로 50개씩 두 건이 주문되면, 두 주문
    모두 승인 시점 재고 0 기준으로 각각 실 생산량 100개(=ceil(50/0.5))로 확정된다(두 번째
    주문이 첫 번째 주문의 미래 생산량을 재고로 보지 않음).
- **Non-goals**: GUI/Web UI, 다중 라인 스케줄링(FIFO 단일 전략만), 인증/다중 사용자, 결제·배송 등
  물류 후속 프로세스

## 기술 스택 / 빌드 환경

- 언어: C++ (C++20, `LanguageStandard=stdcpp20`)
- IDE: Visual Studio (`.slnx` 솔루션, `.vcxproj` 프로젝트)
- 플랫폼 툴셋: v145
- 빌드 구성: Debug/Release × Win32/x64
- 결과물 타입: Console Application
- 빌드는 Visual Studio를 통해 진행한다. (커맨드라인으로 빌드할 경우 `msbuild SampleOrderSystem.slnx`류의 명령 사용)

## 아키텍처 원칙

- **MVC 패턴 준수**: Model / View / Controller 계층을 명확히 분리한다.
  - Model: `Sample`, `Order`, `ProductionJob` 등 도메인 데이터 및 로직 (재고 계산, 생산량 공식,
    상태 전이 규칙 등)
  - View: 콘솔 입출력 (메인 메뉴, 시료 목록, 주문 목록, 모니터링 화면, 생산 라인 화면 등)
  - Controller: 사용자 입력을 받아 Model을 조작하고 View를 갱신하는 흐름 제어
    (시료 관리 / 주문 접수 / 주문 승인·거절 / 모니터링 / 생산 라인 / 출고 처리 각각에 대응)
  - 계층 간 의존 방향을 지키고, View나 Controller에 도메인 로직(수율 계산, 상태 전이 등)이
    섞이지 않도록 한다.

## 핵심 구현 요소

- **데이터 영속성**: JSON을 이용해 Sample/Order/ProductionJob 데이터를 저장/로드한다.
  애플리케이션 재시작 후에도 데이터가 유지되어야 하며 CRUD 연산을 포함한다. 특히 **주문 정보와
  생산 라인(생산 큐) 정보는 진행 중이던 상태까지 포함해 재시작 후 그대로 복원**되어야 한다.
  (JSON 파싱/직렬화는 외부 라이브러리 없이 `Common::JsonValue`로 직접 구현 — 별도 PoC에서
  검증된 "의존성 없는 자체 JSON 파서" 방식을 재사용한 것이며, Phase 0에서 확정됨)
- **데이터 모니터링 시스템**: 상태별 주문 건수(`REJECTED` 제외)와 시료별 재고 현황
  (여유/부족/고갈)을 콘솔에서 실시간 조회할 수 있어야 한다.
- **더미 데이터 생성기**: 테스트/개발 편의를 위해 Sample/Order 더미 데이터를 생성해 DB(JSON)에
  추가하는 기능을 둔다.
- 위 세 항목(MVC 스켈레톤, 영속성, 모니터링, 더미 데이터)은 선행 PoC(별도 Repository)에서
  검증된 구조/기법을 그대로 활용해 구현한다.

## 코딩 원칙

- Clean Code: 함수는 짧고 단일 책임을 갖도록 작성한다. (함수당 라인 수를 과도하게 늘리지 않는다)
- SOLID 원칙을 지킨다.
- 적절한 디자인 패턴을 상황에 맞게 사용한다. (남용 금지, 과설계 지양)

## 사용 Agent

`.claude/agents/`에 정의된 커스텀 서브에이전트를 사용한다.

- **code-review** (`.claude/agents/code-review.md`): 구현된 코드에 대해 다음을 검토
  - Clean Code 여부
  - 함수별 라인 수 적절성
  - SOLID 원칙 준수 여부
  - 적절한 디자인 패턴 사용 여부
  - 코드를 직접 수정하지 않고 리뷰 결과만 보고
- **tester** (`.claude/agents/tester.md`): 구현된 코드에 대해
  - GoogleTest/GMock으로 Unit Test 및 시스템(통합) 테스트 작성 (패키지는
    `packages/gmock.1.11.0/`에 이미 준비됨)
  - 실제 빌드 및 테스트 수행, 결과 보고
- **phase-planner** (`.claude/agents/phase-planner.md`): `docs/PLAN.md`의 Phase 구성을 읽고
  각 Phase를 `docs/Phase{number}.md`로 구체화 (작업 체크리스트, 인터페이스/스키마, 테스트
  계획, 완료 조건 등). 코드는 작성하지 않고 계획 문서만 작성
- **supervisor** (`.claude/agents/supervisor.md`): `phase-planner`/`code-review`/`tester`가
  작업을 한 건 마칠 때마다, 사용자가 그 결과를 직접 확인하기 전에 먼저 `CLAUDE.md`/
  `docs/PRD.md`/`docs/PLAN.md`에 실제로 부합하는지 확인해 요약해준다. 보고 내용을 그대로
  믿지 않고 파일을 직접 열어보며 검증한다. 단, 빌드·테스트 실행은 각 에이전트(특히 tester)
  본인의 책임이므로 supervisor가 중복으로 재실행하지 않고, 보고된 로그/결과가 실제 코드와
  일치하는지 대조하는 방식으로 확인한다. 코드/문서를 직접 수정하지 않는다. 전체 Phase가
  끝난 뒤 한 번에 하는 최종 감사가 아니라 매 에이전트 호출 직후 바로 수행한다.

각 Phase에 착수하기 전 `phase-planner`로 `docs/Phase{number}.md`를 먼저 구체화하고, 기능
구현이 끝나면 `code-review` → `tester` 순으로 호출해 품질을 검증한다. **이 세 에이전트를
호출할 때마다 바로 뒤이어 `supervisor`를 호출**해 그 산출물이 문서에 부합하는지 요약을 받고,
사용자는 그 요약을 보고 다음 단계로 진행할지 판단하는 것을 기본 흐름으로 한다.

## 작업 시 유의사항

- 구현에 앞서 [`docs/PRD.md`](docs/PRD.md) 내용을 우선 확인하고, 이 문서의 "프로젝트 개요" 섹션과
  어긋나지 않는지 확인한다.
- 새 기능 구현 시 MVC 계층 분리를 깨지 않는지 확인한다.
- 데이터 저장/로드 관련 코드는 JSON 직렬화 방식을 일관되게 유지한다.
- 주문 상태 전이(`RESERVED`/`REJECTED`/`PRODUCING`/`CONFIRMED`/`RELEASE`)는 PRD의 상태 흐름도를
  벗어나지 않도록 구현한다. `REJECTED`는 모니터링 집계에서 제외한다.
- 생산량/재고 관련 계산은 PRD 8~9장의 공식(실 생산량 = ceil(부족분 / 수율) 등)과 FIFO 큐 전략을
  그대로 따른다.
