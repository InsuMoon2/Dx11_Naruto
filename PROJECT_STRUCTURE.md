# Dx11_Naruto 프로젝트 구조

> AI 어시스턴트는 매 대화 시작 시 이 파일을 먼저 읽고 현재 구조를 기준으로 판단할 것.
> 마지막 갱신: 2026-03-14

---

## 프로젝트 개요

`Dx11_Naruto`는 DirectX 11 기반 게임 엔진 + 게임 클라이언트 + 에디터 + IOCP 서버 + Assimp 변환 도구로 구성된 멀티 프로젝트 솔루션이다.

현재 솔루션은 다음 축으로 움직인다.

- `Engine`: 렌더링, 오브젝트/컴포넌트, UI, BT, 에셋/프리팹 등 공용 엔진 DLL
- `Client`: 실제 게임 로직, 플레이어/몬스터, 레벨, UI, 네트워크 DLL
- `Game`: 게임 실행 EXE
- `Editor`: ImGui 기반 에디터 EXE
- `Server`: `ServerCore` + `GameServer`
- `AssimpTool`: FBX/OBJ를 커스텀 바이너리로 바꾸는 변환 도구

---

## 솔루션 구성

| 프로젝트 | 경로 | 출력 형태 | 역할 |
|---|---|---|---|
| `Engine` | `Engine/Default/Engine.vcxproj` | DLL | 공용 엔진 코어 |
| `Client` | `Client/Default/Client.vcxproj` | DLL | 게임플레이 / 네트워크 / 로딩 |
| `Game` | `Game/Default/Game.vcxproj` | EXE | 런타임 실행 진입점 |
| `Editor` | `Editor/Default/Editor.vcxproj` | EXE | ImGui 기반 편집기 |
| `AssimpTool` | `AssimpTool/Default/AssimpTool.vcxproj` | EXE | Assimp 기반 모델 변환 툴 |
| `ServerCore` | `Server/ServerCore/Default/ServerCore.vcxproj` | LIB | IOCP 네트워크 공용 코어 |
| `GameServer` | `Server/GameServer/Default/GameServer.vcxproj` | EXE | 게임 서버 |

### 솔루션 폴더

- `00. Tools`: `Editor`, `AssimpTool`
- `01. Client`: `Client`
- `02. Engine`: `Engine`
- `03. Game`: `Game`
- `04. Server`: `ServerCore`, `GameServer`

### 주요 의존성

```text
Engine (DLL)
  └─ 독립 공용 엔진

Client (DLL)
  ├─ Engine
  ├─ ServerCore
  └─ Protobuf 코드

Game (EXE)
  ├─ Engine
  └─ Client

Editor (EXE)
  ├─ Engine
  └─ Client

GameServer (EXE)
  ├─ ServerCore
  └─ Protobuf 코드

AssimpTool (EXE)
  ├─ Engine/Public 의 Assimp 헤더 참조
  └─ Engine/ThirdPartyLib 의 assimp lib 사용
```

### 빌드 구성

- 공통 구성: `Debug`, `Debug_Unity`, `Release`
- Unity 빌드는 `x64/Debug_Unity` 산출물과 `unity_*.cpp` 생성 파일을 사용한다.

---

## 루트 디렉터리 구조

```text
Dx11_Naruto/
├─ AssimpTool/
├─ Client/
├─ Docs/
├─ Editor/
├─ Engine/
├─ EngineSDK/
├─ Game/
├─ Server/
├─ vcpkg_installed/
├─ DirectX11_Naruto.sln
├─ Directory.Build.props
├─ PROJECT_STRUCTURE.md
├─ UpdateLib.bat
└─ vcpkg.json
```

### 루트 폴더별 의미

| 경로 | 설명 |
|---|---|
| `AssimpTool/` | 모델/머티리얼 변환 도구 |
| `Client/` | 게임 클라이언트 DLL + 런타임 리소스 |
| `Docs/` | 작업 문서, 설계 메모, 수업 정리 |
| `Editor/` | 에디터 실행 파일 및 소스 |
| `Engine/` | 엔진 DLL 소스 |
| `EngineSDK/` | 엔진 공개 헤더/라이브러리 복사본 |
| `Game/` | 게임 실행 EXE |
| `Server/` | 서버 관련 프로젝트, 프로토콜, protobuf 배포본 |
| `vcpkg_installed/` | manifest mode 패키지 설치 결과 |

---

## 공통 폴더 규칙

대부분의 Visual Studio 프로젝트는 아래 3분할 구조를 따른다.

- `Default/`: `.vcxproj`, `pch`, 리소스 파일, 진입점
- `Public/`: 헤더 파일
- `Private/`: 소스 파일
- `Bin/`: 실행/빌드 산출물, 리소스 DLL, cso, 런타임 데이터

예외:

- `Server/Protobuf`: protoc, 생성 코드, 원본 `.proto`
- `Engine/ThirdPartyLib`: 수동 배치한 assimp 라이브러리
- `EngineSDK/Include`, `EngineSDK/Lib`: 배포용 엔진 SDK

---

## Engine 구조

### Engine 핵심 역할

`Engine`은 프로젝트 전반에서 공유하는 공용 엔진 DLL이다.
현재 구조상 크게 아래 묶음으로 볼 수 있다.

- 코어: `Base`, `GameInstance`, `Graphic_Device`, `PipeLine`
- 월드/오브젝트: `Level`, `Layer`, `GameObject`, `Object_Manager`
- 컴포넌트/프로토타입: `Component`, `Component_Factory`, `Prototype_Manager`, `Prefab_Manager`
- 렌더링: `Shader`, `Texture`, `VIBuffer*`, `Mesh`, `Model`, `ModelMaterial`, `Model_BinaryLoader`
- UI: `UIObject`, `HUD`, `Panel`, `UI_Manager`, `UI_Text`, `Text_Renderer`
- AI: `BehaviorTree`, `Blackboard`, `BTNode*`, `BTTask_MoveTo`, `BTTask_Wait`
- 유틸/기반 서비스: `Asset_Manager`, `DebugDraw`, `DelegateHub`, `Event_Manager`, `AnimNotify_Factory`

### Engine/Public 주요 헤더 묶음

| 묶음 | 파일 |
|---|---|
| 코어 | `Base.h`, `GameInstance.h`, `Graphic_Device.h`, `PipeLine.h`, `Timer.h`, `Timer_Manager.h` |
| 오브젝트 시스템 | `GameObject.h`, `Component.h`, `Level.h`, `Layer.h`, `Object_Manager.h`, `Prototype_Manager.h`, `Prefab_Manager.h`, `GameObject_Factory.h`, `Component_Factory.h`, `Event_Manager.h` |
| 렌더링 | `Shader.h`, `Texture.h`, `Renderer.h`, `RenderTarget.h`, `VIBuffer.h`, `VIBuffer_Rect.h`, `VIBuffer_Terrain.h`, `Mesh.h`, `Model.h`, `ModelMaterial.h`, `Model_BinaryLoader.h`, `Vertex_Struct.h` |
| 씬/게임플레이 기반 | `Transform.h`, `Camera.h`, `Camera_Manager.h`, `Character.h`, `Controller.h`, `PlayerStart.h`, `Light.h`, `Light_Manager.h` |
| UI/텍스트 | `UIObject.h`, `HUD.h`, `Panel.h`, `UI_Manager.h`, `UI_Text.h`, `Text_Renderer.h`, `Text_Types.h` |
| BT | `BehaviorTree.h`, `Blackboard.h`, `BTNode.h`, `BTRoot.h`, `BTComposite.h`, `BTTask.h`, `BTTask_MoveTo.h`, `BTTask_Wait.h`, `BTNode_Factory.h` |
| 공용 타입 | `Engine_Define.h`, `Engine_Enum.h`, `Engine_Function.h`, `Engine_Macro.h`, `Engine_Struct.h`, `Engine_Typedef.h`, `Property_Types.h`, `Reflection_Macro.h` |
| 기타 | `DebugDraw.h`, `Debug_Manager.h`, `Delegate.h`, `DelegateHub.h`, `Utils.h`, `AnimNotify_Factory.h` |
| protobuf 노출 | `Enum.pb.h`, `Struct.pb.h`, `Protocol.pb.h` |

### Engine/Private 현재 주요 cpp

실제 구현 파일 기준으로 다음 기능이 들어 있다.

- 렌더/리소스: `Shader.cpp`, `Texture.cpp`, `Mesh.cpp`, `Model.cpp`, `ModelMaterial.cpp`, `Model_BinaryLoader.cpp`, `RenderTarget.cpp`
- UI/텍스트: `UIObject.cpp`, `UI_Manager.cpp`, `HUD.cpp`, `Panel.cpp`, `UI_Text.cpp`, `Text_Renderer.cpp`
- AI/BT: `BehaviorTree.cpp`, `Blackboard.cpp`, `BTComposite.cpp`, `BTRoot.cpp`, `BTTask*.cpp`
- 팩토리/관리: `GameObject_Factory.cpp`, `Component_Factory.cpp`, `Prototype_Manager.cpp`, `Prefab_Manager.cpp`, `Asset_Manager.cpp`
- 시스템: `Input_Manager.cpp`, `Event_Manager.cpp`, `DelegateHub.cpp`, `DebugDraw.cpp`

### Engine 주의사항

- `Input_Device`가 아니라 현재 엔진 입력 클래스는 `Input_Manager`다.
- 머티리얼 클래스는 `Material`이 아니라 `ModelMaterial`로 정리되어 있다.
- Assimp는 `Engine/Public/Assimp/` + `Engine/ThirdPartyLib/` 조합으로 유지된다.
- `EngineSDK/Include`, `EngineSDK/Lib`는 `UpdateLib.bat`로 동기화한다.

---

## Client 구조

### Client 핵심 역할

`Client`는 게임플레이 DLL이다.
현재 폴더 기준으로 아래 기능이 들어 있다.

- 플레이어 / 몬스터 / 카메라 / 지형 / 정적 메쉬 액터
- 플레이어 상태 머신(FSM)
- 스킬 시스템 + HUD/UI
- 로더 / 리소스 테이블 파서
- 서버 세션 / 패킷 처리 / 네트워크 매니저

### Client/Public 현재 헤더 목록 기준 묶음

| 묶음 | 파일 |
|---|---|
| 게임 오브젝트 | `Player.h`, `MyPlayer.h`, `RemotePlayer.h`, `Monster.h`, `Terrain.h`, `StaticMeshActor.h`, `Background.h` |
| 카메라 | `Camera_Free.h`, `Camera_Target.h` |
| 컴포넌트 | `CombatStat.h`, `InputComponent.h`, `MovementComponent.h`, `PlayerController.h`, `AIController.h`, `Replicator.h`, `SkillComponent.h` |
| FSM | `IPlayerState.h`, `PlayerStateMachine.h`, `PlayerState_Idle.h`, `PlayerState_Run.h`, `PlayerState_Jump.h`, `PlayerState_DoubleJump.h`, `PlayerState_SuperJump.h` |
| 레벨/앱 | `MainApp.h`, `Loader.h`, `ResourceLoader.h`, `Level_MainTitle.h`, `Level_Loading.h`, `Level_Gameplay.h` |
| UI | `UI_PlayerHUD.h`, `UI_PlayerStatus.h`, `UI_PlayerHP.h`, `UI_PlayerSkill.h`, `UI_SkillSlot.h`, `UI_LoadingProgressBar.h`, `UI_LoadingSpinner.h`, `UI_MainTitleText.h` |
| 네트워크 | `NetworkManager.h`, `ServerSession.h`, `Client_PacketHandler.h`, `IReplicable.h`, `Protocol_Wrapper.h` |
| 기타 | `Spawn_Helper.h`, `SkillDataManager.h`, `Client_Defines.h`, `Client_Enum.h`, `Client_Macro.h`, `Client_Struct.h` |

### Client 현재 특징

- 레벨 클래스는 `Level_Logo`가 아니라 `Level_MainTitle`, `Level_Loading`, `Level_Gameplay`다.
- 로딩은 `Loader` + `ResourceLoader` 이원 구조다.
- UI는 플레이어 HUD 외에도 로딩 스피너/진행바, 메인 타이틀 텍스트까지 실제 파일이 존재한다.
- `SkillComponent`와 `SkillDataManager`가 현재 클라이언트 구조의 중요한 축이다.

### Client/Private 실제 구현 포인트

- 플레이어: `Player.cpp`, `MyPlayer.cpp`, `RemotePlayer.cpp`
- 몬스터/AI: `Monster.cpp`, `AIController.cpp`
- FSM: `PlayerStateMachine.cpp`, `PlayerState_*.cpp`
- 로딩: `Loader.cpp`, `ResourceLoader.cpp`
- 네트워크: `NetworkManager.cpp`, `ServerSession.cpp`, `Client_PacketHandler.cpp`
- UI: `UI_Player*.cpp`, `UI_SkillSlot.cpp`, `UI_Loading*.cpp`, `UI_MainTitleText.cpp`

---

## Game 구조

`Game`은 실행 진입점 EXE다.
실제 소스는 작고, `Client::MainApp`에 위임하는 형태다.

### 폴더 구조

```text
Game/
├─ Default/
├─ Public/
└─ Private/
```

### 주요 파일

| 파일 | 설명 |
|---|---|
| `Game/Default/Game.cpp` | WinMain 진입점 |
| `Game/Public/MainApp.h` | Game 프로젝트용 앱 래퍼 |
| `Game/Private/MainApp.cpp` | 초기화/업데이트/렌더 루프 |
| `Game/Public/Game_Defines.h` | 공통 정의 |
| `Game/Public/Game_Enum.h` | 게임 전용 enum |
| `Game/Public/Game_Macro.h` | 게임 전용 매크로 |

---

## Editor 구조

### Editor 핵심 역할

`Editor`는 ImGui 기반 편집 도구다.
현재 실제 파일 기준으로 아래 구성이 존재한다.

- 에디터 앱/싱글톤/매니저
- Scene/Game/Hierarchy/Inspector/Content Browser/Console/BT/Prefab/Profiler 뷰
- 인스펙터 확장 (`Transform`, `CombatStat`, `BehaviorTree`, `Texture`, `Model`, `Reflection`)
- Undo/Redo 시스템 (`CommandHistory`, `Action_Command`, `Property_Command`)

### Editor/Public 주요 헤더 묶음

| 묶음 | 파일 |
|---|---|
| 코어 | `Editor_MainApp.h`, `Editor_Manager.h`, `EditorInstance.h`, `ImGui_Manager.h`, `Editor_Logger.h`, `Notification_Manager.h` |
| 윈도우 | `EditorWindow.h`, `Scene_View.h`, `Game_View.h`, `Hierarchy.h`, `Inspector.h`, `Content_Browser.h`, `Console_View.h`, `BehaviorTree_View.h`, `Prefab_View.h`, `Profiler_View.h` |
| 인스펙터 | `Inspector_Factory.h`, `Component_Inspector.h`, `Transform_Inspector.h`, `CombatStat_Inspector.h`, `BehaviorTree_Inspector.h`, `Texture_Inspector.h`, `Model_Inspector.h`, `Reflection_Inspector.h` |
| 레벨/기타 | `Level_Editor.h`, `Level_Serializer.h`, `Editor_Camera_Free.h`, `PlayerSession_Manager.h` |
| Undo/Redo | `CommandHistory.h`, `Action_Command.h`, `Property_Command.h` |
| 공용 | `Editor_Define.h`, `Editor_Enum.h`, `Editor_Macro.h`, `Editor_Struct.h`, `IconsFontAwesome6.h` |

### Editor 런타임 출력물

`Editor/Bin/`에는 현재 `Editor.exe`와 `EditorApp.exe` 산출물이 함께 보인다. 런타임 DLL로 `Engine.dll`, protobuf DLL, assimp DLL 등이 같이 배치된다.

---

## AssimpTool 구조

### 목적

`AssimpTool`은 FBX/OBJ를 읽어 커스텀 바이너리와 머티리얼 출력물로 변환하는 도구다.
엔진 런타임과 분리되어 있으며, Assimp에 직접 의존한다.

### 현재 파일 구조

| 경로 | 파일 |
|---|---|
| `AssimpTool/Public` | `Assimp_Macro.h`, `BinaryWriter.h`, `Converter.h`, `ConverterTypes.h` |
| `AssimpTool/Private` | `Converter.cpp` |
| `AssimpTool/Default` | `AssimpTool.cpp`, `BinaryWriter.cpp`, `pch.*`, `.vcxproj` |

### 포인트

- `Converter.cpp`가 변환 핵심
- `BinaryWriter`는 바이너리 출력 유틸
- assimp DLL은 `AssimpTool/Bin/`에 같이 둔다

---

## Server 구조

## GameServer

현재 `Server/GameServer`는 서버 게임 로직을 포함한다.

### Public

- `GameSession.h`
- `GameRoom.h`
- `GameObject.h`
- `Player.h`
- `Monster.h`
- `Server_PacketHandler.h`
- `Server_Macro.h`
- `Server_Typedef.h`

### Private

- `GameSession.cpp`
- `GameRoom.cpp`
- `GameObject.cpp`
- `Player.cpp`
- `Monster.cpp`
- `Server_PacketHandler.cpp`

## ServerCore

현재 `Server/ServerCore`는 IOCP 네트워크 공용 라이브러리다.

### 핵심 파일

- 연결/이벤트: `IocpCore`, `IocpEvent`, `Listener`, `Session`, `Service`
- 버퍼: `RecvBuffer`, `SendBuffer`, `BufferReader`, `BufferWriter`
- 유틸: `NetAddress`, `SocketUtils`, `ThreadManager`
- 공통: `Core_Global`, `Core_Macro`, `Core_Pch`, `Core_TLS`, `Core_Types`

## Protobuf

`Server/Protobuf`에는 다음이 함께 있다.

- 원본 프로토콜: `Protocol/Enum.proto`, `Protocol/Struct.proto`, `Protocol.proto`, `GenProto.bat`
- 생성 코드 배포본: `Bin/Enum.pb.*`, `Protocol.pb.*`, `Struct.pb.*`
- protobuf 자체 include/lib/protoc 배포 파일

주의:

- `Server/Protobuf/include`는 상당히 크며, 대부분 protobuf 배포본이다.
- 보통 실제 작업 포인트는 `Server/Protobuf/Protocol/*.proto`와 생성된 `Bin/*.pb.*`다.

---

## EngineSDK 구조

```text
EngineSDK/
├─ Include/
└─ Lib/
```

| 경로 | 설명 |
|---|---|
| `EngineSDK/Include` | Engine 공개 헤더 복사본 |
| `EngineSDK/Lib` | Engine 라이브러리 복사본 |

`UpdateLib.bat`는 Engine 빌드 후 이 폴더를 갱신하는 용도다.

---

## 리소스 구조

실제 런타임 리소스는 대부분 `Client/Bin/Resources` 아래에 있다.

### 핵심 구조

```text
Client/Bin/Resources/
├─ Data/
│  ├─ csv/
│  ├─ json/
│  │  ├─ BehaviorTrees/
│  │  ├─ EditorSettings/
│  │  ├─ Levels/
│  │  └─ Prefabs/
│  └─ Temp/
├─ Fonts/
├─ Models/
├─ Textures/
├─ Thumbnails/
└─ Shaders/
```

### Data 폴더

| 경로 | 설명 |
|---|---|
| `Data/csv` | 원본 테이블 (`ShaderTable`, `TextureTable`, `TerrainTable`, `ModelTable`, `SkillDataTable`, `StaticLevelComTable`) |
| `Data/json` | 변환 결과 + 수동 JSON |
| `Data/json/BehaviorTrees` | BT 에디터/런타임 JSON |
| `Data/json/Levels` | 레벨 JSON |
| `Data/json/Prefabs` | 프리팹 JSON |
| `Data/json/EditorSettings` | 에디터 설정 |
| `Data/Temp` | 리소스 변환용 임시 스크립트 |

### 현재 눈에 띄는 리소스 포인트

- 스킬 아이콘과 UI 텍스처가 매우 많다.
- `.meta` 파일이 함께 존재하며 에셋 GUID 관리와 연결된다.
- `Client/Bin/Shaders/`가 아니라 현재 셰이더 소스는 `Client/Bin/Shaders/`에 `.hlsl`, 컴파일 결과는 `Client/Bin/*.cso` 조합이다.
- 현재 빌드 출력물에는 `Shader_UI.cso`, `Shader_VtxMesh.cso`, `Shader_VtxNorTex.cso`, `Shader_VtxStaticMesh.cso`, `Shader_Vtxtex.cso`가 보인다.

---

## 문서 폴더 구조

현재 `Docs/`에는 다음 문서가 있다.

| 파일 | 설명 |
|---|---|
| `assimp_converter_plan.md` | Assimp 변환 툴 관련 문서 |
| `fmodel_lobbymap_leveljson_guide.md` | FModel 레벨 JSON 가이드 |
| `skill_component_ui_2slot_plan.md` | 스킬 컴포넌트 + UI 계획 |
| `bone_review_plan_9m_day1_day2.md` | 본/스키닝 수업 정리 |
| `bone_review_plan_9m_day2_day3.md` | Animation/Channel 수업 정리 |

---

## 현재 구조 기준 작업 시 자주 보는 위치

### 게임플레이 수정

- `Client/Public`, `Client/Private`
- `Client/Private/Loader.cpp`
- `Client/Private/ResourceLoader.cpp`
- `Client/Bin/Resources/Data/json`

### 엔진 수정

- `Engine/Public`, `Engine/Private`
- `Engine/Public/Engine_*.h`
- `Engine/Public/Vertex_Struct.h`
- `Engine/Public/Model*.h`

### 에디터 수정

- `Editor/Public`, `Editor/Private`
- `Editor/Private/Inspector*.cpp`
- `Editor/Private/*_View.cpp`

### 서버 수정

- `Server/GameServer/Public`, `Private`
- `Server/ServerCore/Public`, `Private`
- `Server/Protobuf/Protocol/*.proto`

---

## 현재 구조 기준 메모

- 최신 레벨 명칭은 `MainTitle`, `Loading`, `Gameplay` 축이다.
- Client에는 `ResourceLoader`와 `Loader`가 함께 존재하므로 로딩 경로를 볼 때 둘 다 확인해야 한다.
- Engine 쪽 모델 시스템은 `Mesh` + `Model` + `ModelMaterial` + `Model_BinaryLoader` 조합으로 보는 것이 현재 구조에 맞다.
- Engine 입력 클래스명은 `Input_Manager`다.
- UI 관련 코드는 Engine 공용 UI 기반과 Client 구체 UI가 분리되어 있다.
- Server/Protobuf는 배포본이 커서, 구조 문서를 읽을 때는 `Protocol` 원본 폴더를 우선 기준으로 삼는 편이 좋다.

---

## AI 작업 규칙 메모

이 프로젝트에서 구조를 판단할 때 우선순위는 아래와 같다.

1. 실제 `.sln`과 `.vcxproj`
2. 각 프로젝트의 `Public/`, `Private/`, `Default/`
3. `Client/Bin/Resources/Data/json` 같은 런타임 데이터 폴더
4. 이 문서 `PROJECT_STRUCTURE.md`

문서와 실제 파일이 다르면 항상 실제 파일을 기준으로 다시 확인하고 이 문서를 갱신할 것.
