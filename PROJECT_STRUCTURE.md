# Dx11_Naruto 프로젝트 구조

> **AI 어시스턴트는 매 대화 시작 시 이 파일을 반드시 읽을 것!**
> 마지막 갱신: 2026-03-09

---

## 솔루션 개요

DirectX 11 기반 3D 게임 엔진 + 나루토 게임 프로젝트.
멀티플레이어(IOCP 서버), ImGui 에디터, Protobuf 프로토콜, Behavior Tree AI 시스템 포함.

---

## 프로젝트 구성

| 프로젝트 | 타입 | 역할 | 네임스페이스 |
|---|---|---|---|
| `Engine` | DLL | 엔진 코어 (렌더링, 매니저, 컴포넌트 시스템) | `Engine` |
| `Client` | DLL | 게임 로직, 레벨, 네트워크, 리소스 로딩 | `Client` |
| `Game` | EXE | 게임 실행 진입점 (WinMain, 메인 루프) | — |
| `Editor` | EXE | ImGui 기반 에디터 (Hierarchy, Inspector, Scene View 등) | `Editor` |
| `AssimpTool` | EXE | Assimp 기반 모델 컨버터 (FBX → 커스텀 바이너리) | `Assimp` |
| `Server/GameServer` | EXE | IOCP 게임 서버 | `Server` |
| `Server/ServerCore` | LIB | IOCP 네트워크 코어 라이브러리 | — |
| `Server/Protobuf` | — | .proto 파일 및 생성된 코드 | `Protocol` |
| `EngineSDK` | — | Engine DLL 익스포트 헤더/라이브러리 모음 | — |

---

## 의존성 구조

```
Engine (DLL) ← 독립
    ↑
Client (DLL) ─── Engine + ServerCore + Protobuf
    ↑
Game   (EXE) ──── Engine.dll + Client.dll 동적 로드
Editor (EXE) ──── Engine.dll + Client.dll 동적 로드

AssimpTool (EXE) ── Assimp + Engine 헤더 참조 (DLL 비링크)

ServerCore (LIB) ← 독립
    ↑
GameServer (EXE) ── ServerCore + Protobuf
```

> [!IMPORTANT]
> - `Game`과 `Editor`는 `Client.dll`을 동적 로드 → **Game/Editor → Client 단방향**
> - Client에서 Game/Editor 프로젝트 코드 참조 불가
> - `EngineSDK/Include`에 Engine Public 헤더 복사, `EngineSDK/Lib`에 .lib 파일
> - `AssimpTool`은 Assimp만 링크, Engine/Client와 독립 빌드
> - Assimp 라이브러리는 `Engine/ThirdPartyLib/`에 수동 배치, 헤더는 `Engine/Public/Assimp/`에 배치

---

## vcpkg 종속성 (Manifest Mode)

| 패키지 | 용도 |
|---|---|
| `directxtk` | SimpleMath, DDSTextureLoader, WICTextureLoader |
| `effects11` | D3DX11Effect (셰이더 Effect 프레임워크) |
| `spdlog` | 로깅 (LOG_INFO/WARN/ERROR 매크로) |
| `nlohmann-json` | JSON 직렬화 (레벨, 프리팹, 리소스 테이블) |
| `magic-enum` | enum 리플렉션 |
| `imgui` | ImGui (docking, dx11, win32 바인딩) |
| `implot` | ImGui 그래프 |
| `imguizmo` | 3D 기즈모 (트랜스폼 조작) |
| `imgui-node-editor` | BehaviorTree 노드 에디터 |
| `protobuf` | 네트워크 직렬화 + ComponentID 정의 |
| `stduuid` | UUID 생성 (GameObject GUID) |

## 수동 라이브러리 (ThirdPartyLib)

| 라이브러리 | 위치 | 용도 |
|---|---|---|
| `assimp` | `Engine/ThirdPartyLib/` (lib), `Engine/Public/Assimp/` (헤더) | 3D 모델 임포트 (AssimpTool에서 사용) |

> [!NOTE]
> Assimp는 vcpkg manifest 모드에 문제가 있어 직접 다운로드 후 수동 배치 방식으로 전환됨.

---

## 디렉토리 구조

모든 프로젝트는 `Default/`, `Public/`, `Private/` 구조:
- `Default/` — pch, .vcxproj, 진입점 코드
- `Public/` — 헤더 파일 (.h)
- `Private/` — 소스 파일 (.cpp)
- `ThirdPartyLib/` — (Engine 전용) 수동 배치된 외부 라이브러리 (.lib)

---

## Engine 주요 클래스

### 코어 & 매니저

| 클래스 | 설명 |
|---|---|
| `GameInstance` | 엔진 싱글톤 (`GAME` 매크로). 모든 매니저 소유, 엔진 API 퍼사드 |
| `Graphic_Device` | D3D11 디바이스, 스왑체인, 백버퍼, 뷰포트 관리 |
| `Level_Manager` | 현재 레벨 관리, 레벨 전환 |
| `Object_Manager` | 레벨별 GameObject/Layer 저장, 생명주기(Update/Render) |
| `Prototype_Manager` | 프로토타입 등록/클론 (Component + GameObject) |
| `Prefab_Manager` | JSON 기반 프리팹 저장/로드/인스턴스화 |
| `Asset_Manager` | GUID 기반 에셋 메타(.meta) 관리, 경로↔GUID 변환, 타입별 목록 조회 |
| `Renderer` | 렌더 그룹 관리 (Priority, NonBlend, Blend, UI) |
| `PipeLine` | View/Proj 행렬, 카메라 위치 바인딩 |
| `Timer_Manager` / `Timer` | 프레임 델타 타임 계산 |
| `Input_Manager` | 키 입력 (`INPUT` 매크로) |
| `Event_Manager` | 이벤트 큐 (Create/Delete Object) |
| `Component_Factory` | 컴포넌트 팩토리 (타입ID → Creator 등록/생성) |
| `GameObject_Factory` | GameObject 타입별 팩토리 (`REGISTER_GAMEOBJECT` 매크로, 이름/타입 기반 생성) |
| `BTNode_Factory` | BT 노드 팩토리 |
| `Camera_Manager` | 카메라 관리 (등록, 활성 카메라 전환, Toggle) |
| `Light_Manager` | 라이트 관리 (Add/Get/Clear) |
| `UI_Manager` | UI 레이어 관리 (EUILayer별 UI 등록, Show/Hide/Toggle, 뷰포트 리사이즈 알림, 입력 차단 판정) |
| `Debug_Manager` | 디버그 관리 (현재 빈 껍데기) |
| `DelegateHub` | 전역 델리게이트 허브 (OnPlayerSpawned 등 이벤트 모음) |

### 기반 클래스

| 클래스 | 설명 |
|---|---|
| `Base` | 최상위 (레퍼런스 카운팅, `Free()`) |
| `GameObject` | 게임 오브젝트 (컴포넌트 소유, Update/Render 가상함수) |
| `Component` | 컴포넌트 기반 (`GENERATED_COMPONENT` 매크로로 ID 지정) |
| `Level` | 레벨 기반 (Layer 관리) |
| `Layer` | 같은 레이어의 GameObject 목록 |

### 렌더링 컴포넌트

| 클래스 | 설명 |
|---|---|
| `Shader` | HLSL Effect 파일 로드, 상수/텍스처 바인딩, Pass 실행 |
| `Texture` | DDS/WIC 텍스처 로드, SRV 제공 |
| `VIBuffer` | 정점/인덱스 버퍼 기반 |
| `VIBuffer_Rect` | 사각형 (UI용) |
| `VIBuffer_Terrain` | 하이트맵 기반 지형 메쉬 |
| `Mesh` | Assimp aiMesh 기반 메쉬 (VIBuffer 상속, VTXMESH 정점 사용) |
| `Model` | Assimp 모델 로드 컴포넌트 (다수 Mesh 소유, 렌더) |
| `Transform` | 위치/회전/스케일, 로컬/월드, 부모-자식 계층 |
| `RenderTarget` | 렌더 타겟 텍스처 |

### 게임플레이

| 클래스 | 설명 |
|---|---|
| `Camera` | 카메라 (뷰/프로젝션 행렬 → PipeLine 설정) |
| `Character` | 캐릭터 기반 (Engine 레이어) |
| `Controller` | 컨트롤러 기반 |
| `UIObject` | UI 오브젝트 기반 (직교 투영, FUIDesc, Visibility, EUILayer, zOrder) |
| `HUD` | UIObject 상속 컨테이너. 자식 UI 생성/등록(`Create_Child`), 포커스 불가 |
| `Panel` | HUD 상속. 포커스 가능한 UI 패널 |
| `PlayerStart` | 스폰 포인트 (멀티 스폰 인덱스 지원) |
| `Light` | 라이트 오브젝트 (Directional/Point, FLightDesc 보유) |

### BT (Behavior Tree)

| 클래스 | 설명 |
|---|---|
| `BehaviorTree` | BT 컴포넌트 (Blackboard 소유) |
| `Blackboard` | 키-값 데이터 저장 |
| `BTNode` / `BTRoot` | 노드 기반/루트 |
| `BTComposite` | Sequence, Selector |
| `BTTask` | 태스크 기반 |
| `BTTask_MoveTo` / `BTTask_Wait` | 이동/대기 태스크 |

### 유틸리티

| 클래스/파일 | 설명 |
|---|---|
| `Utils` | wstring ↔ string 변환 |
| `Delegate.h` | 이벤트 델리게이트 시스템 |
| `AnimNotify_Factory` | 애니메이션 노티파이 팩토리 |
| `ICommand` | Undo/Redo 커맨드 인터페이스 (Execute/Undo/Redo 순수 가상) |
| `Vertex_Struct.h` | 정점 구조체 정의 (`VTXTEX`, `VTXNORTEX`, `VTXMESH`) + InputLayout |
| `Property_Types.h` | 프로퍼티 타입 정의 (리플렉션 시스템용) |
| `Reflection_Macro.h` | 리플렉션 매크로 (컴포넌트 프로퍼티 자동 노출) |
| `Engine_Define.h` | 엔진 공통 정의 |
| `Engine_Enum.h` | 엔진 열거형 |
| `Engine_Function.h` | 엔진 유틸 함수 |
| `Engine_Macro.h` | 엔진 매크로 모음 |
| `Engine_Struct.h` | 엔진 구조체 (`FLightDesc` 등) |
| `Engine_Typedef.h` | 타입 별칭 |
| `Enum.pb.h` / `Struct.pb.h` / `Protocol.pb.h` | Protobuf 코드젠 (ComponentID, ObjectType, 패킷 등) |

---

## Client 주요 클래스

### 게임 오브젝트

| 클래스 | 설명 |
|---|---|
| `Player` | 기본 플레이어 |
| `MyPlayer` | 로컬 플레이어 (네트워크) |
| `RemotePlayer` | 원격 플레이어 |
| `Monster` | 몬스터 (AIController + BT) |
| `Terrain` | 지형 (Shader + Texture + VIBuffer_Terrain) |
| `Camera_Free` | 자유 카메라 |
| `Camera_Target` | 타겟 추적 카메라 (오프셋 + 스무딩 팔로우) |
| `Background` | UI 배경 |
| `StaticMeshActor` | 정적 메쉬 오브젝트 (GUID 기반 Model + Shader, 프리팹/레벨에서 사용) |

### UI 시스템 (Client)

| 클래스 | 부모 | 설명 |
|---|---|---|
| `UI_PlayerHUD` | `HUD` | 플레이어 전체 HUD 루트. `UI_PlayerStatus` 자식 보유, `Bind_Player()` |
| `UI_PlayerStatus` | `Panel` | 플레이어 상태 패널 (HP바 + 배경). 자식으로 `UI_PlayerHP` 보유 |
| `UI_PlayerHP` | `UIObject` | HP 비율 바 (Shader + Texture + VIBuffer_Rect, `Set_Ratio()`) |
| `UI_PlayerSkill` | `Panel` | **스킬 패널 (현재 빈 껍데기, 구현 예정)** |

### 플레이어 상태 머신 (FSM)

| 클래스 | 설명 |
|---|---|
| `IPlayerState` | 플레이어 상태 인터페이스 (Enter/Update/Exit 순수 가상) |
| `PlayerStateMachine` | FSM 컴포넌트 (`COMPONENT_TYPE_PLAYER_STATE`). 상태 등록/전환, InputComponent + MovementComponent 참조 |
| `PlayerState_Idle` | Idle 상태 구현 |
| `PlayerState_Run` | Run 상태 구현 |
| `PlayerState_Jump` | Jump 상태 구현 |

### 클라이언트 정의 헤더

| 파일 | 설명 |
|---|---|
| `Client_Defines.h` | Client 네임스페이스 공통 정의 |
| `Client_Enum.h` | Client 전용 열거형 (`EPlayerState`: Idle, Run, Jump) |
| `Client_Macro.h` | Client 전용 매크로 |
| `Protocol_Wrapper.h` | Protobuf 헤더 래퍼 (pch 격리용) |

### 컴포넌트

| 클래스 | 설명 |
|---|---|
| `CombatStat` | 전투 스탯 (HP, ATK 등) |
| `MovementComponent` | 이동 처리 |
| `InputComponent` | 입력 처리 |
| `PlayerController` | 플레이어 입력 → 이동 연결 |
| `AIController` | AI 제어 (BT 실행) |
| `Replicator` | 네트워크 복제 |

### 레벨 & 로딩

| 클래스 | 설명 |
|---|---|
| `Level_MainTitle` | 메인 타이틀 레벨 (기존 Level_Logo에서 변경) |
| `Level_Loading` | 로딩 레벨 (비동기 로드 관리) |
| `Level_Gameplay` | 게임플레이 레벨 |
| `Loader` | 비동기 리소스/프로토타입 로딩 (별도 쓰레드) |
| `ResourceLoader` | JSON 테이블 파싱 → 셰이더/텍스처/지형 등록 |

### 네트워크

| 클래스 | 설명 |
|---|---|
| `NetworkManager` | 네트워크 싱글톤 (`namespace Client`) |
| `ServerSession` | 서버 세션 |
| `Client_PacketHandler` | 패킷 생성/처리 |

### 기타

| 클래스 | 설명 |
|---|---|
| `Spawn_Helper` | 빌더 패턴 스폰 헬퍼 |
| `IReplicable` | 복제 인터페이스 |

---

## Game (EXE)

| 파일 | 설명 |
|---|---|
| `Game.cpp` (Default/) | WinMain, 메인 루프, 메시지 처리 |
| `MainApp.h/cpp` | Initialize/Update/Render 루프 (`Client::MainApp` 위임) |
| `Game_Defines.h` | Game 프로젝트 공통 정의 |
| `Game_Enum.h` | Game 전용 열거형 |
| `Game_Macro.h` | Game 전용 매크로 |

---

## Editor (EXE)

### 에디터 코어

| 클래스 | 설명 |
|---|---|
| `Editor_MainApp` | 에디터 메인 앱 |
| `Editor_Manager` | 에디터 전체 관리 (윈도우, 선택 객체, 메뉴바) |
| `EditorInstance` | 에디터 싱글톤 |
| `ImGui_Manager` | ImGui 초기화/렌더 |
| `Editor_Camera_Free` | 에디터 전용 자유 카메라 (Camera_Free 상속, 에디터 씬 뷰 전용) |
| `Level_Editor` | 에디터 전용 빈 레벨 |
| `Level_Serializer` | JSON 레벨 저장/로드 |

### 에디터 윈도우

| 클래스 | 설명 |
|---|---|
| `EditorWindow` | 에디터 윈도우 기반 클래스 |
| `Scene_View` | 3D 씬 뷰 (ImGuizmo 연동) |
| `Game_View` | 게임 뷰 |
| `Hierarchy` | 오브젝트 계층구조 트리 |
| `Inspector` | 컴포넌트 인스펙터 |
| `Content_Browser` | 리소스 탐색기 |
| `Console_View` | 로그 콘솔 |
| `BehaviorTree_View` | BT 노드 에디터 |
| `Prefab_View` | 프리팹 관리 |
| `Profiler_View` | 성능 프로파일러 |

### 인스펙터

| 클래스 | 설명 |
|---|---|
| `Inspector_Factory` | 인스펙터 레지스트리 |
| `Component_Inspector` | 인스펙터 인터페이스 |
| `Transform_Inspector` | Transform 인스펙터 |
| `CombatStat_Inspector` | CombatStat 인스펙터 |
| `BehaviorTree_Inspector` | BT 인스펙터 |
| `Texture_Inspector` | 텍스처 인스펙터 |
| `Reflection_Inspector` | 리플렉션 기반 자동 인스펙터 |

### Undo/Redo 시스템

| 클래스 | 설명 |
|---|---|
| `CommandHistory` | Undo/Redo 스택 관리 (ICommand 기반, 최대 100개 이력) |
| `Action_Command` | 범용 람다 기반 Undo/Redo 커맨드 (ICommand 상속) |
| `Property_Command` | 프로퍼티 값 변경 Undo/Redo (EPropertyType 기반 역직렬화) |

### 기타

| 클래스 | 설명 |
|---|---|
| `Notification_Manager` | 토스트 알림 |
| `Editor_Logger` | spdlog 에디터 통합 |
| `PlayerSession_Manager` | 플레이어 세션 에디터 관리 |
| `IconsFontAwesome6.h` | FontAwesome 6 아이콘 유니코드 상수 |

### 에디터 정의 헤더

| 파일 | 설명 |
|---|---|
| `Editor_Define.h` | 에디터 공통 정의 |
| `Editor_Enum.h` | 에디터 열거형 |
| `Editor_Macro.h` | 에디터 매크로 |
| `Editor_Struct.h` | 에디터 구조체 |

---

## AssimpTool (EXE)

모델 컨버터 — Assimp으로 FBX/OBJ 읽고 커스텀 바이너리로 변환.
Assimp 헤더는 `Engine/Public/Assimp/`에서, 라이브러리는 `Engine/ThirdPartyLib/`에서 참조.

| 파일 | 설명 |
|---|---|
| `pch.h` | Assimp 헤더, SimpleMath, spdlog, Engine_Macro.h 포함 |
| `Assimp_Macro.h` | Engine_Macro.h 참조 (LOG_INFO 등 공유) |
| `Converter.h/cpp` | FBX → 커스텀 변환 핵심 (`ReadAssetFile`) |
| `AssimpTool.cpp` | 진입점 (main) |

### 빌드 설정
- **추가 포함 디렉터리**: `../../Engine/Public/` (Assimp 헤더 접근용)
- **추가 라이브러리 디렉터리**: `../../Engine/ThirdPartyLib/`
- **추가 종속성**: `assimp-vc143-mtd.lib` (Debug) / `assimp-vc143-mt.lib` (Release)
- **추가 옵션**: `/utf-8` (C/C++ 명령줄, Unicode 지원용)

> [!IMPORTANT]
> `assimp-vc143-mt.dll`을 실행 폴더(`AssimpTool/Bin/`)에 수동 복사 필요.

---

## Server

### GameServer

| 클래스 | 설명 |
|---|---|
| `GameSession` | 클라이언트 1개 연결 세션 |
| `GameSessionManager` | 전체 세션 관리 (`GSessionManager` 전역) |
| `Server_PacketHandler` | 서버 패킷 핸들/생성 |

### ServerCore (LIB)

IOCP 기반 네트워크 코어:
`IocpCore`, `IocpEvent`, `Session`, `Listener`, `Service`,
`NetAddress`, `SocketUtils`, `ThreadManager`,
`SendBuffer`, `RecvBuffer`, `BufferReader`, `BufferWriter`

---

## 레벨 & 로딩 흐름

### ELevelType 인덱스
```
Loading = 0, Static = 1, MainTitle = 2, GamePlay = 3
```

### 로딩 흐름
```
Ready_StartLevel(MainTitle)
  └─ Level_Loading 생성
       └─ Loader::Loading_For_MainTitleLevel() [비동기 쓰레드]
            ├─ Register_Components()     → Static 레벨에 컴포넌트/오브젝트 등록
            ├─ Initialize_BT_Nodes()
            ├─ ResourceLoader::Load_ShaderTable()
            ├─ ResourceLoader::Load_TerrainTable()
            └─ ResourceLoader::Load_TextureTable()
  └─ Level_MainTitle 생성

[Space 입력]
  └─ Level_Loading 생성
       └─ Loader::Loading_For_GamePlay() [비동기 쓰레드]
            ├─ ShaderTable / TerrainTable / TextureTable 재로드
            ├─ Player, Terrain, Camera_Free 프로토타입 등록
  └─ Level_Gameplay 생성
```

> [!WARNING]
> `Ready_StartLevel(GamePlay)` 직접 호출 금지 — Logo 단계에서 Static 레벨 등록이 선행되어야 함

---

## 컴포넌트 시스템

### 등록 방식
- `Component_Factory`에 `Register<T>(levelIndex)`: 자동으로 프로토타입 생성 + 등록
- Shader/Texture 등 초기화 인자가 필요한 것은 람다 Creator 사용
- ComponentID는 **Protobuf enum** (`Protocol::COMPONENT_TYPE_XXX`) 사용

### 매크로
```cpp
GENERATED_COMPONENT(ClassName, Protocol::COMPONENT_TYPE_XXX)
// → StaticTypeID(), Get_ComponentID() 자동 생성
```

### 등록 위치
- `Register_Components()` in `Loader.cpp` — Static 레벨(1)에 등록
- Static 레벨에 등록된 컴포넌트는 **모든 레벨에서 공유**

### 현재 등록된 컴포넌트 (Static 레벨)
CombatStat, Replicator, VIBuffer_Rect, MovementComponent,
InputComponent, BehaviorTree, PlayerController, AIController

---

## 주요 구조체

### FLightDesc (Engine_Struct.h)
```cpp
struct FLightDesc {
    ELightType  type;       // Directional, Point
    Vec4        direction;
    Vec4        position;
    float       range;
    Color       diffuse;
    Color       ambient;
    Color       specular;
};
```

---

## 리소스 구조

### 데이터 테이블 (CSV → JSON 변환)
```
Client/Bin/Resources/Data/
├── csv/
│   ├── ShaderTable.csv       → 셰이더 경로 + InputLayout 정의
│   ├── TextureTable.csv      → 텍스처 경로 + 타입
│   ├── TerrainTable.csv      → 지형 설정
│   └── StaticLevelComTable.csv → Static 레벨 컴포넌트 테이블
├── json/                     → CSV에서 변환된 JSON
│   ├── ShaderTable.json
│   ├── TextureTable.json
│   ├── TerrainTable.json
│   └── StaticLevelComTable.json
└── ConvertResource.py        → CSV → JSON 변환 스크립트
```

### 리소스 디렉토리
```
Client/Bin/Resources/
├── Textures/
│   ├── Terrain/     → 지형 텍스처
│   ├── Logo/        → 로고 이미지
│   ├── Player/      → 플레이어 텍스처
│   ├── Explosion/   → 이펙트
│   ├── SkyBox/      → 스카이박스
│   ├── Snow/
│   ├── UI/
│   │   ├── Dialog/Textures/     → 다이얼로그/구매/보상 UI 텍스처 (78개+)
│   │   ├── Loading_Screen/      → 로딩 스크린 UI
│   │   ├── MainTitle/           → 메인 타이틀 UI
│   │   ├── PlayerStats/         → 플레이어 스탯 UI (HP바, 상태바 등)
│   │   └── Skill_Icon/          → 스킬 아이콘 텍스처
│   ├── Default0/1.dds/.jpg → 기본 텍스처
│   └── Clip_Icon, Folder_Icon 등 → 에디터 아이콘
├── Models/          → 3D 모델 (Fiona, ForkLift, Gaara, NarutoTest, Rock, Test, Tong, map)
├── Fonts/           → FontAwesome 등
└── Data/            → 위 참조
```

> [!NOTE]
> 각 에셋에는 `.meta` 파일이 자동 생성됨 (Asset_Manager의 GUID 시스템). 예: `Default0.dds.meta`.

> [!NOTE]
> 셰이더 파일(.hlsl/.fx)은 별도 폴더가 아닌 ShaderTable.json에 경로가 기록되어 ResourceLoader가 로드.
> Effects11 프레임워크 사용 (technique11 / pass).

---

## 정점 구조체 (Vertex_Struct.h)

| 이름 | 구성 | 용도 |
|---|---|---|
| `VTXTEX` | Position(Vec3) + TexCoord(Vec2) | UI, 단순 텍스처 |
| `VTXNORTEX` | Position(Vec3) + Normal(Vec3) + TexCoord(Vec2) | 라이팅 지원 메쉬 (Terrain 등) |
| `VTXMESH` | Position(Vec3) + Normal(Vec3) + Tangent(Vec3) + TexCoord(Vec2) | 3D 모델 메쉬 (Assimp 로드) |

---

## 렌더 그룹

```cpp
enum class ERenderGroup { Priority, NonBlend, Blend, UI, END };
```
- **Priority**: 스카이박스 등 먼저 그릴 것
- **NonBlend**: 불투명 (Terrain, 캐릭터 등)
- **Blend**: 반투명
- **UI**: 2D UI (직교 투영)

## UI 레이어 (EUILayer)

```cpp
enum class EUILayer { HUD, Navigation, Popup, System, Overlay, END };
```
- **HUD**: 항상 표시 (체력바, 스킬)
- **Navigation**: 퀘스트 화살표, 정보 전달용
- **Popup**: 인벤토리, 상점, 일시정지
- **System**: 알림
- **Overlay**: 페이드 인/아웃, 로딩

---

## 핵심 매크로

| 매크로 | 용도 |
|---|---|
| `GAME` | `GameInstance::GetInstance()` |
| `INPUT` | `Input_Manager::GetInstance()` |
| `EVENT` | `Event_Manager::GetInstance()` |
| `ETOI(ENUM)` | enum → unsigned int 캐스트 |
| `CHECK_NULL` | null 체크 + 로그 + return |
| `CHECK_FAILED` | HRESULT 실패 체크 + 로그 + return |
| `LOCK_WP` | weak_ptr lock + 실패 시 로그 + return |
| `GENERATED_BODY(Name)` | `StaticClassName()` + 이름 자동 설정 |
| `GENERATED_COMPONENT(Name, ID)` | `StaticTypeID()` + `Get_ComponentID()` 자동 생성 |
| `NS_BEGIN` / `NS_END` | namespace 열기/닫기 |
| `ENGINE_DLL` | DLL export/import |

---

## 타입 별칭

```cpp
using Shared = std::shared_ptr<T>;
using Weak   = std::weak_ptr<T>;
using Unique = std::unique_ptr<T>;
using umap   = std::unordered_map<K,V>;
using uset   = std::unordered_set<T>;

// SimpleMath
using Vec2/Vec3/Vec4, Matrix, Quat, Color, Ray, Plane;
using ComPtr = Microsoft::WRL::ComPtr<T>;

// DX 타입 별칭
using Device = ID3D11Device;
using DeviceContext = ID3D11DeviceContext;
using SwapChain = IDXGISwapChain;
// ...기타
```

---

## 네트워크 흐름

```
Player::Late_Update()
  → Client::NetworkManager::Send_Packet()
  → Client_PacketHandler::Make_C_Move()
  ─────────────────────────────→ GameSession::OnRecvPacket()
                                  → Server_PacketHandler::HandlePacket()
                                  → Handle_C_Move()
                                  → GSessionManager::Broadcast(Make_S_Move())
  ←─────────────────────────────── 모든 클라이언트
  Client_PacketHandler::Handle_S_Move()
  → 해당 objectId GameObject 위치 갱신
```

### 패킷 ID
```cpp
enum PacketID {
    S_Test = 1, S_EnterGame = 2, S_MyPlayer = 3,
    S_AddObject = 4, S_RemoveObject = 5, S_Move = 6,
    C_Move = 50,
};
```

---

## 미해결 이슈

- [x] `Client_PacketHandler` → Client 프로젝트로 이동 완료
- [ ] `Handle_S_Move`에서 ObjectManager 연동 (다른 플레이어 위치 갱신)
- [ ] `Handle_S_AddObject` / `Handle_S_RemoveObject` 클라이언트 구현
- [ ] 이동 동기화 패킷 throttle (매 프레임 전송 → 주기적 전송)
- [ ] `Model` 컴포넌트 GENERATED_COMPONENT 매크로 정식 적용 (현재 주석 처리됨)
- [ ] `UI_PlayerSkill` 구현 (현재 빈 껍데기, Panel 상속만 있음) — 데이터 테이블 기반 스킬 아이콘 자동 매핑 검토 중

---

## UpdateLib.bat

Engine 빌드 후 `EngineSDK/Include`와 `EngineSDK/Lib`로 헤더/라이브러리를 복사하는 배치 파일.
