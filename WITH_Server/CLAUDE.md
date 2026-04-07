# 프로젝트 개요 (Project Overview)
본 프로젝트는 고성능 **TaskGraph 기반 ECS(Entity Component System)** 프레임워크와
이를 활용한 **멀티플레이 소울라이크 게임** 개발을 목표로 합니다. 단순한 기능 구현을 넘어,
엔진 레벨에서의 데이터 지향 설계(DOD)와 하드웨어 성능을 극한으로 끌어올리는 아키텍처를 지향합니다.

### 🎯 핵심 개발 목표 (Development Goals)
1.  **자동 병렬화 (Auto-Parallelization):** ECS의 데이터 독립성과 TaskGraph 자료구조를 결합하여 실행 로직(System)의 동적/자동 병렬 스케줄링을 구현합니다.
2.  **데이터 지향 설계 (DOD):** Cache Miss를 최소화하는 SparseInt/DenseArray 구조를 기반으로 데이터 레이아웃을 설계합니다.
3.  **멀티스레드 최적화:** False Sharing 방지, Lock-free 자료구조 활용, 동기화 오버헤드 최소화를 통해 코어 수에 비례하는 성능 확장을 추구합니다.
4.  **서버 퍼포먼스:** 높은 처리량(Throughput)과 낮은 지연 시간(Latency)을 위해 네트워크 패킷 및 데이터 관리 오버헤드를 물리적 한계까지 최적화합니다.

### ⚠️ 설계 원칙 (Design Principles for AI)\
1. 계층별 설계 원칙 (Layered Design Principles)
-	Core Engine Layer (ECS, TaskGraph etc.)
	- Data-oriented Design (DOD): 메모리 레이아웃 최적화, 캐시 효율성, SoA(Structure of Arrays) 패턴을 우선합니다.
	- Zero-Abstraction Cost: 성능 오버헤드가 있는 추상화는 지양하며, 하드웨어 친화적인 로우 레벨 코드를 작성합니다.

-	Gameplay Content Layer (Systems, Abilities, AI)
	- High-Level API: 복잡한 내부 로직은 캡슐화하고, 콘텐츠 개발자가 의도를 명확히 드러낼 수 있는 선언적(Declarative) API를 제공합니다.
	- Readability Over Micro-optimization: 콘텐츠 로직에서는 극단적인 최적화보다 **유지보수성(Maintainability)과 도메인 로직의 명확성**을 우선합니다.

2. 콘텐츠 개발 가이드라인 (Content Development Rules)
-	의도의 명확성 (Expressiveness): 함수명과 변수명은 구현 방법(How)가 아닌 **게임 플레이 목적(What/Why)**을 나타내는 도메인 용어를 사용합니다.
-	상태 격리 (State Isolation): 컨텐츠 로직은 엔진 내부의 복잡한 상태에 직접 접근하는 대신, 정해진 컴포넌트 인터페이스나 메시징 시스템을 통해서만 상호작용합니다.
-	안전한 추상화 (Safe Abstraction): 성능 임계점이 낮은 컨텐츠 로직에서는 std::function이나 Virtual Function등 가독성을 높이는 추상화 도구를 제한적으로 허용하되, 이를 엔진 코어와 명확히 분리합니다.


# 기술 스택 (Tech Stack)
-	**Core:** C++20
-	**Data Tool:** .NET Framework 8.0 (C#)
-	**Style:** `.editorconfig` 및 `.clang-format` (Allman 스타일) 준수

# 실행 및 빌드 명령어 (Commands)
-	**포맷팅 적용:** `clang-format -i <file>` (코드 수정 후 필요 실행)

# 코딩 컨벤션 (Coding Conventions)
### 1. 네이밍 (Naming)
- **Class/Struct/Function:** `PascalCase`
- **Variables/Parameters:** `camelCase`
- **Private Members:** `_underbarPrefix` (예: `_health`)
- **Constants:** `kPascalCase` (예: `kMaxPlayer`)

### 2. 상세 규칙
- **중괄호:** Allman 스타일 (새 줄에 `{` 배치)
- **포인터/참조자:** 타입 중심 정렬 (`T* ptr`, `Entity& ref`)
- **헤더 보호:** `#pragma once` 사용
- **제어문:** `if`, `for`, `while` 키워드 뒤에 공백 삽입

# 작업 지침 (Strict Rules)
1. **Context Awareness:** 엔진 코드를 건드릴 때는 성능 통계를, 콘텐츠 코드를 건드릴 때는 가독성 통계를 우선하라.
2. **Auto-Formatting:** 모든 작업의 마지막 단계는 반드시 `clang-format` 실행이어야 한다.
3. **No Abbreviations:** 변수명에서 임의의 축약어 사용을 금하며, 의도가 드러나는 명확한 단어를 선택하라.
4. **Safety:** 멀티스레드 환경을 고려하여 공유 자원 접근 시 엔진에서 제공하는 TaskGraph 인터페이스를 준수하라.
