# Dx11_Naruto 프로젝트 구조

> AI 어시스턴트는 매 대화 시작 시 이 파일을 먼저 읽고 현재 구조를 기준으로 판단할 것.
> 마지막 갱신: 2026-04-11

## 문서 보강 규칙

- 기존 본문/코드/설명은 삭제하지 않는다.
- 최신안은 바로 아래에 `[대체]`, `[변경]`, `[추가]` 태그와 함께 다시 적는다.
- 변경된 파일은 무엇을 어떻게 수정하는지 코드와 함께 보여줘야 한다.
- 문서가 초안에서 상세판으로 확장될 때는 요약 대체가 아니라 보존 + 보강 방식으로 작성한다.
- 코드 예시는 축약하지 말고, 구현에 필요한 헤더/CPP 수준까지 가능한 한 완성형으로 제시한다.

---

## 프로젝트 개요

`Dx11_Naruto`는 DirectX 11 기반 게임 엔진 + 게임 클라이언트 + 에디터 + IOCP 서버 + Assimp 변환 도구로 구성된 멀티 프로젝트 솔루션이다.

현재 솔루션은 다음 축으로 움직인다.

- `Engine`: 렌더링, 오브젝트/컴포넌트, UI, BT, 카메라 시네마틱, 애니메이션/노티파이, 에셋/프리팹 등 공용 엔진 DLL
- `Client`: 실제 게임 로직, 플레이어/몬스터, 레벨, UI, 무기/커스터마이징, 스킬, 네트워크 DLL
- `Game`: 게임 실행 EXE
- `Editor`: ImGui 기반 에디터 EXE (시네마틱/애니메이션 뷰 포함)
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
├─ .editorconfig
├─ .gitignore
├─ AGENTS.md
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
| `Docs/` | 작업 문서, 설계 메모, 가이드라인 |
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

- 코어: `Base`, `GameInstance`, `Graphic_Device`, `PipeLine`, `ICommand`, `Level_Manager`, `Asset_Manager`
- 월드/오브젝트: `Level`, `Layer`, `GameObject`, `Object_Manager`, `ContainerObject`, `PartObject`
- 컴포넌트/프로토타입: `Component`, `Component_Factory`, `Prototype_Manager`, `Prefab_Manager`, `GameObject_Factory`
- 렌더링: `Shader`, `Texture`, `VIBuffer*`, `Mesh`, `Model`, `ModelMaterial`, `Model_BinaryLoader`, `Renderer`, `RenderTarget`
- UI: `UIObject`, `HUD`, `Panel`, `UI_Manager`, `UI_Text`, `Text_Renderer`, `UI_AnimPlayer`, `UI_AnimSerializer`, `UI_AnimTypes`, `UI_AnimUtility`
- AI: `BehaviorTree`, `Blackboard`, `BTNode*`, `BTTask_MoveTo`, `BTTask_Wait`
- 애니메이션: `Animation`, `Animation_Manager`, `Bone`, `Channel`
- 애니메이션 노티파이: `AnimNotify`, `AnimNotifyState`, `AnimNotify_Factory`, `AnimNotify_Serializer`, `AnimNotify_Types`
- 카메라: `Camera`, `Camera_Manager`, `Camera_Cinematic`, `CameraTrack_Player`, `CameraTrack_Serializer`, `Camera_Types`
- 씬/게임플레이: `Transform`, `Character`, `Controller`, `PlayerStart`, `Light`, `Light_Manager`
- 유틸/기반 서비스: `DebugDraw`, `Debug_Manager`, `DelegateHub`, `Delegate`, `Event_Manager`, `Input_Manager`, `Utils`

### Engine/Public 주요 헤더 묶음 (94 파일)

| 묶음 | 파일 |
|---|---|
| 코어 | `Base.h`, `GameInstance.h`, `Graphic_Device.h`, `PipeLine.h`, `Timer.h`, `Timer_Manager.h`, `ICommand.h`, `Level_Manager.h`, `Asset_Manager.h` |
| 오브젝트/시스템 | `GameObject.h`, `Component.h`, `Level.h`, `Layer.h`, `Object_Manager.h`, `Prototype_Manager.h`, `Prefab_Manager.h`, `GameObject_Factory.h`, `Component_Factory.h`, `Event_Manager.h`, `ContainerObject.h`, `PartObject.h` |
| 렌더링 | `Shader.h`, `Texture.h`, `Renderer.h`, `RenderTarget.h`, `VIBuffer.h`, `VIBuffer_Rect.h`, `VIBuffer_Terrain.h`, `Mesh.h`, `Model.h`, `ModelMaterial.h`, `Model_BinaryLoader.h`, `ModelBin_Types.h`, `Vertex_Struct.h` |
| 카메라 | `Camera.h`, `Camera_Manager.h`, `Camera_Cinematic.h`, `CameraTrack_Player.h`, `CameraTrack_Serializer.h`, `Camera_Types.h` |
| 씬/게임플레이 | `Transform.h`, `Character.h`, `Controller.h`, `PlayerStart.h`, `Light.h`, `Light_Manager.h` |
| UI/텍스트 | `UIObject.h`, `HUD.h`, `Panel.h`, `UI_Manager.h`, `UI_Text.h`, `Text_Renderer.h`, `Text_Types.h`, `UI_AnimPlayer.h`, `UI_AnimSerializer.h`, `UI_AnimTypes.h`, `UI_AnimUtility.h` |
| BT/AI | `BehaviorTree.h`, `Blackboard.h`, `BTNode.h`, `BTRoot.h`, `BTComposite.h`, `BTTask.h`, `BTTask_MoveTo.h`, `BTTask_Wait.h`, `BTTask_SetAnimState.h`, `BTNode_Factory.h` |
| 애니메이션 | `Animation.h`, `Animation_Manager.h`, `Bone.h`, `Channel.h` |
| 애니메이션 노티파이 | `AnimNotify.h`, `AnimNotifyState.h`, `AnimNotify_Factory.h`, `AnimNotify_Serializer.h`, `AnimNotify_Types.h` |
| 공용 타입 | `Engine_Define.h`, `Engine_Enum.h`, `Engine_Function.h`, `Engine_Macro.h`, `Engine_Struct.h`, `Engine_Typedef.h`, `Property_Types.h`, `Reflection_Macro.h` |
| 사운드/입력/기타 | `Sound_Manager.h`, `Input_Manager.h`, `DebugDraw.h`, `Debug_Manager.h`, `Delegate.h`, `DelegateHub.h`, `Utils.h` |
| protobuf 노출 | `Enum.pb.h`, `Struct.pb.h`, `Protocol.pb.h` |

### Engine/Private 현재 주요 cpp (78 파일)

- 렌더/리소스: `Shader.cpp`, `Texture.cpp`, `Mesh.cpp`, `Model.cpp`, `ModelMaterial.cpp`, `Model_BinaryLoader.cpp`, `RenderTarget.cpp`, `Renderer.cpp`, `Asset_Manager.cpp`
- 사운드/입력/시스템: `Sound_Manager.cpp`, `Input_Manager.cpp`, `GameInstance.cpp`, `Graphic_Device.cpp`, `PipeLine.cpp`, `Event_Manager.cpp`, `DelegateHub.cpp`, `DebugDraw.cpp`
- UI/텍스트: `UIObject.cpp`, `UI_Manager.cpp`, `HUD.cpp`, `Panel.cpp`, `UI_Text.cpp`, `Text_Renderer.cpp`, `UI_AnimPlayer.cpp`, `UI_AnimSerializer.cpp`, `UI_AnimUtility.cpp`
- AI/BT: `BehaviorTree.cpp`, `Blackboard.cpp`, `BTComposite.cpp`, `BTRoot.cpp`, `BTNode.cpp`, `BTTask.cpp`, `BTTask_MoveTo.cpp`, `BTTask_Wait.cpp`, `BTTask_SetAnimState.cpp`, `BTNode_Factory.cpp`
- 애니메이션/노티파이: `Animation.cpp`, `Animation_Manager.cpp`, `Bone.cpp`, `Channel.cpp`, `AnimNotify.cpp`, `AnimNotifyState.cpp`, `AnimNotify_Factory.cpp`, `AnimNotify_Serializer.cpp`
- 카메라: `Camera.cpp`, `Camera_Manager.cpp`, `Camera_Cinematic.cpp`, `CameraTrack_Player.cpp`, `CameraTrack_Serializer.cpp`
- 팩토리/관리: `GameObject_Factory.cpp`, `Component_Factory.cpp`, `Prototype_Manager.cpp`, `Prefab_Manager.cpp`, `Level_Manager.cpp`, `Object_Manager.cpp`
- 씬/게임플레이: `Transform.cpp`, `Character.cpp`, `Controller.cpp`, `PlayerStart.cpp`, `Light.cpp`, `Light_Manager.cpp`
- 기타: `ContainerObject.cpp`, `PartObject.cpp`, `SimpleMath.cpp`, `Base.cpp`, `Layer.cpp`, `Level.cpp`, `VIBuffer.cpp`, `VIBuffer_Rect.cpp`, `VIBuffer_Terrain.cpp`, `Timer.cpp`, `Timer_Manager.cpp`, `ICommand.cpp`, `Delegate.cpp`, `Debug_Manager.cpp`, `Utils.cpp`, `Component.cpp`, `GameObject.cpp`

### Engine 주의사항

- `Input_Device`가 아니라 현재 엔진 입력 클래스는 `Input_Manager`다.
- 머티리얼 클래스는 `Material`이 아니라 `ModelMaterial`로 정리되어 있다.
- Assimp는 `Engine/Public/Assimp/` + `Engine/ThirdPartyLib/` 조합으로 유지된다.
- `EngineSDK/Include`, `EngineSDK/Lib`는 `UpdateLib.bat`로 동기화한다.
- 카메라 시네마틱 시스템이 `Camera_Cinematic`, `CameraTrack_Player`, `CameraTrack_Serializer`로 구성되어 있다.

---

## Client 구조

### Client 핵심 역할

`Client`는 게임플레이 DLL이다.
현재 폴더 기준으로 아래 기능이 들어 있다.

- 플레이어 / 몬스터 / 카메라 / 지형 / 정적 메쉬 액터 / 무기
- 플레이어 상태 머신(FSM) + 스킬 상태
- 스킬 시스템 + HUD/UI
- 커스터마이징 (파츠, 프리뷰 플레이어)
- 로더 / 리소스 테이블 파서
- 서버 세션 / 패킷 처리 / 네트워크 매니저

### Client/Public 현재 헤더 목록 (65 파일)

| 묶음 | 파일 |
|---|---|
| 게임 오브젝트 | `Player.h`, `MyPlayer.h`, `RemotePlayer.h`, `PreviewPlayer.h`, `Player_CustomPart.h`, `Monster.h`, `Terrain.h`, `StaticMeshActor.h`, `Background.h`, `Weapon.h` |
| 카메라 | `Camera_Free.h`, `Camera_Target.h` |
| 컴포넌트 | `CombatStat.h`, `InputComponent.h`, `MovementComponent.h`, `PlayerController.h`, `AIController.h`, `Replicator.h`, `SkillComponent.h`, `AnimationStateComponent.h` |
| FSM | `IPlayerState.h`, `PlayerStateMachine.h`, `PlayerState_Idle.h`, `PlayerState_Run.h`, `PlayerState_Jump.h`, `PlayerState_DoubleJump.h`, `PlayerState_SuperJump.h`, `PlayerState_SuperJumpCharge.h`, `PlayerState_Dash.h`, `PlayerState_JumpDash.h`, `PlayerState_HeightLand.h`, `PlayerState_Attack.h`, `PlayerState_Skill.h` |
| 레벨/앱 | `MainApp.h`, `Loader.h`, `ResourceLoader.h`, `Level_MainTitle.h`, `Level_Loading.h`, `Level_Gameplay.h`, `Level_CharacterSetup.h` |
| UI | `UI_PlayerHUD.h`, `UI_PlayerStatus.h`, `UI_PlayerHP.h`, `UI_PlayerSkill.h`, `UI_SkillSlot.h`, `UI_LoadingProgressBar.h`, `UI_LoadingSpinner.h`, `UI_MainTitleMenuButton.h`, `UI_TabButton.h` |
| 네트워크 | `NetworkManager.h`, `ServerSession.h`, `Client_PacketHandler.h`, `IReplicable.h`, `Protocol_Wrapper.h` |
| 커스터마이징 | `Customizer_Manager.h` |
| 스킬/데이터 | `SkillDataManager.h`, `Spawn_Helper.h`, `Client_Defines.h`, `Client_Enum.h`, `Client_Macro.h`, `Client_Struct.h` |
| 테스팅 및 상태 | `ANS_Test.h`, `AN_Test.h`, `ANS_ComboWindow.h`, `ANS_CollisionEnable.h` |

### Client/Private 실제 구현 포인트 (59 파일)

- 플레이어: `Player.cpp`, `MyPlayer.cpp`, `RemotePlayer.cpp`, `PreviewPlayer.cpp`, `Player_CustomPart.cpp`
- 몬스터/AI: `Monster.cpp`, `AIController.cpp`
- FSM: `PlayerStateMachine.cpp`, `PlayerState_Idle.cpp`, `PlayerState_Run.cpp`, `PlayerState_Jump.cpp`, `PlayerState_DoubleJump.cpp`, `PlayerState_SuperJump.cpp`, `PlayerState_SuperJumpCharge.cpp`, `PlayerState_Dash.cpp`, `PlayerState_JumpDash.cpp`, `PlayerState_HeightLand.cpp`, `PlayerState_Attack.cpp`, `PlayerState_Skill.cpp`, `IPlayerState.cpp`
- 컴포넌트: `InputComponent.cpp`, `MovementComponent.cpp`, `CombatStat.cpp`, `SkillComponent.cpp`, `AnimationStateComponent.cpp`, `Replicator.cpp`, `PlayerController.cpp`
- 무기/커스터마이징: `Weapon.cpp`, `Customizer_Manager.cpp`
- 로딩: `Loader.cpp`, `ResourceLoader.cpp`
- 레벨: `Level_MainTitle.cpp`, `Level_Loading.cpp`, `Level_Gameplay.cpp`, `Level_CharacterSetup.cpp`, `MainApp.cpp`
- 네트워크: `NetworkManager.cpp`, `ServerSession.cpp`, `Client_PacketHandler.cpp`
- UI: `UI_PlayerHUD.cpp`, `UI_PlayerStatus.cpp`, `UI_PlayerHP.cpp`, `UI_PlayerSkill.cpp`, `UI_SkillSlot.cpp`, `UI_LoadingProgressBar.cpp`, `UI_LoadingSpinner.cpp`, `UI_MainTitleMenuButton.cpp`, `UI_TabButton.cpp`
- 오브젝트: `Terrain.cpp`, `StaticMeshActor.cpp`, `Background.cpp`, `Camera_Free.cpp`, `Camera_Target.cpp`
- 기타: `Spawn_Helper.cpp`, `SkillDataManager.cpp`, `ANS_Test.cpp`, `AN_Test.cpp`, `ANS_ComboWindow.cpp`, `ANS_CollisionEnable.cpp`

### Client 현재 특징

- 레벨 클래스는 `Level_MainTitle`, `Level_Loading`, `Level_Gameplay`, `Level_CharacterSetup` 4종이다.
- 로딩은 `Loader` + `ResourceLoader` 이원 구조다.
- UI는 플레이어 HUD 외에도 로딩 스피너/진행바, 메인 타이틀 메뉴 버튼, 탭 버튼까지 실제 파일이 존재한다.
- `SkillComponent`와 `SkillDataManager`가 현재 클라이언트 구조의 중요한 축이다.
- `PlayerState_Skill`이 추가되어 데이터 드리븐 스킬 FSM이 가능하다.
- `Weapon`, `Player_CustomPart`, `Customizer_Manager`가 캐릭터 커스터마이징/무기 시스템을 구성한다.
- `PreviewPlayer`는 에디터 캐릭터 셋업 프리뷰용 플레이어 클래스다.
- `AnimationStateComponent`가 스킬 애니메이션 상태 전환의 핵심 컴포넌트다.

---

## Game 구조

`Game`은 실행 진입점 EXE다.
실제 소스는 작고, `Client::MainApp`에 위임하는 형태다.

### 폴더 구조

```text
Game/
├─ Bin/
├─ Default/
├─ Private/
└─ Public/
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
- Scene/Game/Hierarchy/Inspector/Content Browser/Console/BT/Prefab/Profiler/Animation/UI Animation/Cinematic 뷰
- 인스펙터 확장 (`Transform`, `CombatStat`, `BehaviorTree`, `Texture`, `Model`, `Reflection`, `AnimNotify`, `AnimNotifyState`, `AnimationState`, `PlayerStateMachine`, `Skill`)
- 시퀀서 어댑터 (`AnimSequencerAdapter`, `CameraSequencerAdapter`, `UI_AnimSequencerAdapter`)
- Undo/Redo 시스템 (`CommandHistory`, `Action_Command`, `Property_Command`)

### Editor/Public 주요 헤더 묶음 (52 파일)

| 묶음 | 파일 |
|---|---|
| 코어 | `Editor_MainApp.h`, `Editor_Manager.h`, `EditorInstance.h`, `ImGui_Manager.h`, `Editor_Logger.h`, `Notification_Manager.h`, `Editor_Helper.h` |
| 윈도우 | `EditorWindow.h`, `Scene_View.h`, `Game_View.h`, `Hierarchy.h`, `Inspector.h`, `Content_Browser.h`, `Console_View.h`, `BehaviorTree_View.h`, `Prefab_View.h`, `Profiler_View.h`, `Animation_View.h`, `UI_Animation_View.h`, `Cinematic_View.h` |
| 인스펙터 | `Inspector_Factory.h`, `Component_Inspector.h`, `Transform_Inspector.h`, `CombatStat_Inspector.h`, `BehaviorTree_Inspector.h`, `Texture_Inspector.h`, `Model_Inspector.h`, `Reflection_Inspector.h`, `AnimNotify_Inspector.h`, `AnimNotifyState_Inspector.h`, `AnimNotify_Inspector_Factory.h`, `AnimationState_Inspector.h`, `PlayerStateMachine_Inspector.h`, `Skill_Inspector.h` |
| 시퀀서/어댑터 | `AnimSequencerAdapter.h`, `Editor_SequencerAdapterBase.h`, `UI_AnimSequencerAdapter.h`, `CameraSequencerAdapter.h`, `UI_AnimTypes.h` |
| 레벨/기타 | `Level_Editor.h`, `Level_Serializer.h`, `Editor_Camera_Free.h`, `PlayerSession_Manager.h`, `Prefab_PreviewCameraSettings.h` |
| Undo/Redo | `CommandHistory.h`, `Action_Command.h`, `Property_Command.h` |
| 공용 | `Editor_Define.h`, `Editor_Enum.h`, `Editor_Macro.h`, `Editor_Struct.h`, `IconsFontAwesome6.h` |

### Editor/Private 주요 cpp (48 파일)

- 뷰: `Scene_View.cpp`, `Game_View.cpp`, `Hierarchy.cpp`, `Inspector.cpp`, `Content_Browser.cpp`, `Console_View.cpp`, `BehaviorTree_View.cpp`, `Prefab_View.cpp`, `Profiler_View.cpp`, `Animation_View.cpp`, `UI_Animation_View.cpp`, `Cinematic_View.cpp`
- 인스펙터: `Inspector_Factory.cpp`, `Component_Inspector.cpp`, `Transform_Inspector.cpp`, `CombatStat_Inspector.cpp`, `BehaviorTree_Inspector.cpp`, `Texture_Inspector.cpp`, `Model_Inspector.cpp`, `Reflection_Inspector.cpp`, `AnimNotify_Inspector.cpp`, `AnimNotifyState_Inspector.cpp`, `AnimNotify_Inspector_Factory.cpp`, `AnimationState_Inspector.cpp`, `PlayerStateMachine_Inspector.cpp`, `Skill_Inspector.cpp`
- 시퀀서: `AnimSequencerAdapter.cpp`, `CameraSequencerAdapter.cpp`, `UI_AnimSequencerAdapter.cpp`, `ImSequencer.cpp`, `Editor_SequencerAdapterBase.cpp`
- 코어: `Editor_MainApp.cpp`, `Editor_Manager.cpp`, `EditorInstance.cpp`, `ImGui_Manager.cpp`, `Editor_Logger.cpp`, `Notification_Manager.cpp`, `Editor_Helper.cpp`
- 기타: `Level_Editor.cpp`, `Level_Serializer.cpp`, `Editor_Camera_Free.cpp`, `PlayerSession_Manager.cpp`, `Prefab_PreviewCameraSettings.cpp`, `CommandHistory.cpp`, `Action_Command.cpp`, `Property_Command.cpp`, `EditorWindow.cpp`, `DebugDraw.cpp`

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

### Public (8 파일)

- `GameSession.h`
- `GameRoom.h`
- `GameObject.h`
- `Player.h`
- `Monster.h`
- `Server_PacketHandler.h`
- `Server_Macro.h`
- `Server_Typedef.h`

### Private (6 파일)

- `GameSession.cpp`
- `GameRoom.cpp`
- `GameObject.cpp`
- `Player.cpp`
- `Monster.cpp`
- `Server_PacketHandler.cpp`

## ServerCore

현재 `Server/ServerCore`는 IOCP 네트워크 공용 라이브러리다.

### 핵심 파일 (Public 17 파일)

- 연결/이벤트: `IocpCore.h`, `IocpEvent.h`, `Listener.h`, `Session.h`, `Service.h`
- 버퍼: `RecvBuffer.h`, `SendBuffer.h`, `BufferReader.h`, `BufferWriter.h`
- 유틸: `NetAddress.h`, `SocketUtils.h`, `ThreadManager.h`
- 공통: `Core_Global.h`, `Core_Macro.h`, `Core_Pch.h`, `Core_TLS.h`, `Core_Types.h`

## Protobuf

`Server/Protobuf`에는 다음이 함께 있다.

- 원본 프로토콜: `Protocol/Enum.proto`, `Protocol/Struct.proto`, `Protocol/Protocol.proto`, `Protocol/GenProto.bat`
- 생성 코드 배포본: `Bin/Enum.pb.*`, `Protocol.pb.*`, `Struct.pb.*`
- protobuf 자체 include/lib/protoc 배포 파일

주의:

- `Server/Protobuf/include`는 상당히 크며, 대부분 protobuf 배포본이다.
- 보통 실제 작업 포인트는 `Server/Protobuf/Protocol/*.proto`와 생성된 `Bin/*.pb.*`다.

### [추가] 현재 서버/동기화 실제 흐름 메모

#### [추가] 1. ServerCore 계층 역할

- `Server/ServerCore/Public/Session.h`, `Server/ServerCore/Private/Session.cpp`
  - `Session`이 소켓 송수신, `PacketSession`이 `[size][id][payload]` 패킷 조립을 담당한다.
- `Server/ServerCore/Public/Service.h`, `Server/ServerCore/Private/Service.cpp`
  - `ClientService`, `ServerService`가 세션 팩토리와 IOCP 등록을 관리한다.
- `Server/ServerCore/Public/Listener.h`, `Server/ServerCore/Private/Listener.cpp`
  - 서버 accept 등록과 새 세션 연결 완료 처리를 담당한다.
- `Server/ServerCore/Public/IocpCore.h`, `Server/ServerCore/Private/IocpCore.cpp`
  - `GetQueuedCompletionStatus` 기반 이벤트 디스패치 코어다.

#### [추가] 2. GameServer 진입 흐름

- `Server/GameServer/Default/GameServer.cpp`
  - `127.0.0.1:7777`로 `ServerService`를 열고, 메인 루프에서 `IocpCore::Dispatch(0)`와 `GRoom->Update(timeDelta)`를 반복한다.
- `Server/GameServer/Private/GameSession.cpp`
  - 세션 연결 후 바로 룸 입장시키지 않고, 패킷 수신 시 `Server_PacketHandler::HandlePacket()`으로 위임한다.
- `Server/GameServer/Private/Server_PacketHandler.cpp`
  - 현재 실제 수신 패킷은 `C_EnterGame`, `C_Move` 두 개가 중심이다.
- `Server/GameServer/Private/GameRoom.cpp`
  - 실제 플레이어 생성, 기존 오브젝트 전송, 브로드캐스트, 퇴장 처리, 몬스터 스폰/권한 판정을 담당한다.

#### [추가] 3. 현재 입장/생성 동기화 순서

1. 클라이언트가 `C_EnterGame` 전송
   - `Client/Private/Level_Gameplay.cpp`
   - `Client/Private/Client_PacketHandler.cpp`
2. 서버가 `GameRoom::Enter_GameRoom()` 호출
   - `Server/GameServer/Private/Server_PacketHandler.cpp`
   - `Server/GameServer/Private/GameRoom.cpp`
3. 서버가 새 플레이어를 만들고 세션의 `playerId`를 배정
   - `Server/GameServer/Private/Player.cpp`
4. 서버가 본인용 `S_MyPlayer` 전송 후, 기존 플레이어/몬스터 목록을 `S_AddObject`로 전송
5. 마지막에 `Add_Player()`를 호출해 다른 클라이언트들에게도 새 플레이어를 브로드캐스트

#### [추가] 4. 현재 이동/상태 동기화 방식

- 플레이어
  - `Client/Private/MyPlayer.cpp`에서 주기적으로 `Build_NetworkInfo()` 후 `C_Move`를 전송한다.
  - `Server/GameServer/Private/Server_PacketHandler.cpp`에서 플레이어 패킷은 항상 `session->Get_PlayerId()`로 objectId를 강제한다.
  - `Server/GameServer/Private/GameRoom.cpp`에서 받은 `ObjectInfo`를 그대로 저장하고 `S_Move`로 전체 브로드캐스트한다.
  - `Client/Private/RemotePlayer.cpp`에서 수신 위치/회전을 보간 적용한다.

- 애니메이션 상태
  - `Client/Private/AnimationStateComponent.cpp`가 `ObjectInfo.object_state`, `move_dir`, `anim_phase`, `anim_force_restart`, `attack_profile`, `attack_combo_index`를 쓰고 읽는다.
  - 즉 현재 애니메이션 동기화는 별도 패킷이 아니라 `S_Move/C_Move` 안의 `ObjectInfo` 필드에 합쳐져 있다.

- 몬스터
  - 현재 서버 authoritative BT가 아니라, "권한 클라이언트 1명"이 몬스터 BT/애니 상태를 계산하고 서버로 릴레이한다.
  - 권한 판정 기준은 `Client/Private/Client_PacketHandler.cpp`의 "가장 작은 playerId를 가진 플레이어"다.
  - `Client/Private/Monster.cpp`
    - 권한 클라는 `_networkDriven == false` 상태로 로컬 BT를 계속 돌린다.
    - 일정 주기마다 `Build_NetworkInfo()` + `C_Move`로 몬스터 상태를 서버에 보낸다.
  - `Server/GameServer/Private/GameRoom.cpp`
    - 몬스터 `C_Move`는 권한 플레이어가 아니면 무시한다.
    - 통과하면 몬스터 `ObjectInfo`를 갱신하고 `S_Move`로 브로드캐스트한다.
  - 비권한 클라는 `Client/Private/Monster.cpp::Sync()`로 서버 값만 따라가는 꼭두각시 모드다.

#### [추가] 5. 현재 서버 몬스터 스폰 구조

- `Server/GameServer/Private/GameRoom.cpp`
  - 첫 `Enter_GameRoom()` 시 `Ensure_LevelMonstersSpawned()`를 호출한다.
  - `../../../Client/Bin/Resources/Data/json/Levels/[20260420]Tutorial.level.json`에서 몬스터 배치 정보를 읽는다.
  - JSON `gameObjects` 중 `object_type == MONSTER`만 골라 서버 `Monster`를 생성한다.
- `Server/GameServer/Private/Monster.cpp`
  - 서버용 `Monster` 클래스에는 자체 상태 머신/회전/원운동 예제 구현이 남아 있다.
- [변경]
  - 하지만 현재 `GameRoom::Update()`에서는 서버 몬스터 AI를 실제로 돌리지 않고 있다.
  - 따라서 지금 메인 경로 기준 몬스터 동기화는 "서버 시뮬레이션"보다 "권한 클라 릴레이"가 맞다.

#### [추가] 6. 현재 구조상 주의 포인트

- `Server/ServerCore/Private/Service.cpp`
  - `CloseService()`가 아직 TODO 상태라 종료/정리 루틴이 완성되지 않았다.
- `Client/Public/Replicator.h`, `Client/Private/Replicator.cpp`
  - `Replicator` 컴포넌트는 존재하지만 transform 동기화 TODO가 남아 있어 현재 멀티플레이 주 동기화 경로는 아니다.
- `Client/Private/Level_Gameplay.cpp`
  - 서버 모드 진입 시 로컬 몬스터를 먼저 제거하고 서버 `S_AddObject` 기준으로 다시 생성하는 구조다.
- `Server/Protobuf/Protocol/Struct.proto`
  - 현재 네트워크 핵심 복제 단위는 `ObjectInfo` 하나이며, 위치/회전/스탯/상태/장비 파츠가 모두 여기에 모여 있다.

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
├─ .asset_cache.json
├─ Data/
│  ├─ ConvertResource.py
│  ├─ ConvertXLSX.py
│  ├─ MapTools/
│  ├─ csv/
│  ├─ json/
│  └─ xlsx/
├─ Fonts/
├─ Materials/
├─ Models/
├─ StaticMesh/
├─ Textures/
├─ Thumbnails/
└─ Shaders/
```

### Data 폴더

| 경로 | 설명 |
|---|---|
| `Data/csv` | 원본 테이블 (`ShaderTable`, `TextureTable`, `TerrainTable`, `ModelTable`, `SkillDataTable`, `StaticLevelComTable`) |
| `Data/xlsx` | 원본 엑셀 시트 (ConvertXLSX.py 로 csv→json 변환 소스) |
| `Data/json` | 변환 결과 + 수동 JSON |
| `Data/json/AnimNotifies` | 애니메이션 노티파이 데이터 JSON |
| `Data/json/BehaviorTrees` | BT 에디터/런타임 JSON |
| `Data/json/Cinematics` | 시네마틱 카메라 트랙 JSON |
| `Data/json/Levels` | 레벨 JSON |
| `Data/json/Prefabs` | 프리팹 JSON |
| `Data/json/EditorSettings` | 에디터 설정 |
| `Data/json/UIAnimations` | UI 애니메이션 키프레임 JSON |
| `Data/MapTools` | 맵 도구용 스크립트/데이터 |

### Data/json 주요 파일

| 파일 | 설명 |
|---|---|
| `DT_Shader.json` | 셰이더 데이터 테이블 |
| `DT_Texture.json` | 텍스처 데이터 테이블 |
| `DT_Terrain.json` | 지형 데이터 테이블 |
| `DT_Model.json` | 모델 데이터 테이블 |
| `DT_SkillData.json` | 스킬 데이터 테이블 |
| `DT_StaticLevel.json` | 스태틱 레벨 테이블 |
| `DT_GameObject.json` | 게임 오브젝트 프로토타입 데이터 테이블 |
| `StaticLevelComTable.json` | 스태틱 레벨 컴포넌트 테이블 |
| `*_mesh_guid_map.json` | 맵별 메쉬 GUID 매핑 (KonohaVillage, ExamStadium 등) |

### 셰이더

| 확장자 | 파일 |
|---|---|
| `.hlsl` (소스) | `Shader_UI.hlsl`, `Shader_VtxAnimMesh.hlsl`, `Shader_VtxMesh.hlsl`, `Shader_VtxNorTex.hlsl`, `Shader_VtxStaticMesh.hlsl`, `Shader_Vtxtex.hlsl` |
| `.cso` (컴파일) | `Shader_UI.cso`, `Shader_VtxAnimMesh.cso`, `Shader_VtxMesh.cso`, `Shader_VtxNorTex.cso`, `Shader_VtxStaticMesh.cso`, `Shader_Vtxtex.cso` |

### 리소스 참고사항

- `.meta` 파일이 함께 존재하며 에셋 GUID 관리와 연결된다.
- `Materials/` 폴더는 모델 머티리얼 관련 리소스 저장소다.
- `StaticMesh/` 폴더는 정적 메쉬 바이너리 저장소다.
- `Shader_VtxAnimMesh`가 스켈레탈 애니메이션 메쉬 전용 셰이더다.

---

## 문서 폴더 구조

현재 `Docs/`에는 다음 문서가 있다.

| 파일 | 설명 |
|---|---|
| `BTSkillmd.md` | BT 시스템 관련 스킬 설정 가이드 |
| `JumpDash 잔상.md` | 점프 대시 시 잔상 효과 구현 관련 |
| `PlayerSkill_BTSkill_가이드라인md.md` | 플레이어 스킬 및 BT 스킬 가이드라인 |
| `monster_bt_animation_guide.md` | 몬스터 BT + 애니메이션 통합 가이드 |
| `player_name_input_guide.md` | 플레이어 이름 입력 구현 가이드 |
| `skill_object_guideline.md` | 스킬 오브젝트 시스템 설계 문서 |
| `에디터_노티파이_비활성화_가이드.md` | 에디터 노티파이 비활성화 관련 가이드 |
| `지형타기_네비메시_하이브리드_가이드라인.md` | 충돌 메시 레이캐스트 + NavMesh 하이브리드 지형타기 설계 가이드 |

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

### 스킬/애니메이션 시스템 수정

- `Client/Public/SkillComponent.h`, `AnimationStateComponent.h`, `PlayerState_Skill.h`
- `Client/Private/SkillComponent.cpp`, `AnimationStateComponent.cpp`, `PlayerState_Skill.cpp`
- `Client/Bin/Resources/Data/json/DT_SkillData.json`
- `Engine/Public/AnimNotify*.h`, `Animation.h`

### 카메라/시네마틱 수정

- `Engine/Public/Camera_Cinematic.h`, `CameraTrack_Player.h`, `CameraTrack_Serializer.h`, `Camera_Types.h`
- `Editor/Public/Cinematic_View.h`, `CameraSequencerAdapter.h`
- `Client/Bin/Resources/Data/json/Cinematics/`

### 커스터마이징/장비 수정

- `Client/Public/Weapon.h`, `Player_CustomPart.h`, `Customizer_Manager.h`, `Level_CharacterSetup.h`, `PreviewPlayer.h`
- `Client/Private/` 대응 cpp 파일들

---

## 현재 구조 기준 메모

- 최신 레벨 명칭은 `MainTitle`, `Loading`, `Gameplay`, `CharacterSetup` 4종이다.
- Client에는 `ResourceLoader`와 `Loader`가 함께 존재하므로 로딩 경로를 볼 때 둘 다 확인해야 한다.
- Engine 쪽 모델 시스템은 `Mesh` + `Model` + `ModelMaterial` + `Model_BinaryLoader` 조합으로 보는 것이 현재 구조에 맞다.
- Engine 입력 클래스명은 `Input_Manager`다.
- UI 관련 코드는 Engine 공용 UI 기반과 Client 구체 UI가 분리되어 있다.
- Server/Protobuf는 배포본이 커서, 구조 문서를 읽을 때는 `Protocol` 원본 폴더를 우선 기준으로 삼는 편이 좋다.
- 카메라 시네마틱 시스템(`Camera_Cinematic` + `CameraTrack_Player` + `CameraTrack_Serializer`)과 에디터 `Cinematic_View`가 시네마틱 편집을 지원한다.
- 스킬은 데이터 드리븐 방식으로 `DT_SkillData.json` → `SkillDataManager` → `SkillComponent` → `AnimationStateComponent` → `PlayerState_Skill` 흐름으로 작동한다.
- **[추가: 이펙트/노티파이 관례]** 나선환과 같은 무한 루프 애니메이션 재생 시 노티파이(`ANS`)의 중복 호출/스폰을 방지하기 위해, 향후 핵심 스킬 이펙트의 생성 및 생명주기 관리는 `AnimNotify`에 의존하기보다 `PlayerState_Skill`(FSM)의 `Enter`/`Exit` 단에서 자체 관리 및 파괴하도록 구조를 분리/발전시키는 방향을 갖는다.
- **[추가: AN_SpawnParticle 중복 방지]** 루프 클립에 배치된 `AN_SpawnParticle`은 `FAnimNotifyContext::wrapped`가 `true`일 때(`context.wrapped`) `Execute()`를 스킵하여 매 루프마다 중복 스폰되는 현상을 방지한다.
- **[추가: ANS_SpawnParticle]** `Client/Public/ANS_SpawnParticle.h`, `Client/Private/ANS_SpawnParticle.cpp` 신규 추가. `On_Begin`에서 `AttachedEffectObject` 스폰 후 `Weak<AttachedEffectObject> _spawnedEffect`로 보관, `On_End`에서 `Stop_AttachedEffect()`로 정리. Loop → AttackEnd 전환 시 이펙트 잔재 제거 목적.
- **[추가: ANS On_End 보장 원칙]** `Model::Reset_AnimationSequenceState()`에서 `Stop_AllNotifyStates`를 `true`로 호출하여, 강제 인터럽트 시에도 활성 ANS에 `On_End`가 반드시 전달되도록 수정. "Begin이 불렸으면 End는 반드시 불린다"는 ANS 설계 계약을 엔진 레벨에서 보장.
- `Weapon`은 `PartObject` 기반으로 캐릭터 본(소켓)에 부착된다.
- `Player_CustomPart`와 `Customizer_Manager`는 캐릭터 파츠 교체 시스템의 핵심이다.

---

## AI 작업 규칙 메모

이 프로젝트에서 구조를 판단할 때 우선순위는 아래와 같다.

1. 실제 `.sln`과 `.vcxproj`
2. 각 프로젝트의 `Public/`, `Private/`, `Default/`
3. `Client/Bin/Resources/Data/json` 같은 런타임 데이터 폴더
4. 이 문서 `PROJECT_STRUCTURE.md`

문서와 실제 파일이 다르면 항상 실제 파일을 기준으로 다시 확인하고 이 문서를 갱신할 것.

- [추가] Codex가 직접 실제 코드 파일을 수정할 때는, 사용자가 따로 요청하지 않은 설명용 주석이나 장황한 안내 주석을 임의로 덧붙이지 않는다.
- [추가] 다만 이 레포의 `AGENTS.md`처럼 새 변수/함수 주석을 강제하는 상위 작업 규칙이 있는 경우에는 그 규칙을 우선 적용한다.
