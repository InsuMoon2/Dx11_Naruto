# 스킬 컴포넌트 + 우하단 2슬롯 UI 구현안 (Dx11_Naruto 맞춤)

## 0. 목표

- 플레이어가 장착한 스킬 2개를 **화면 우하단 HUD**에 표시한다.
- 스킬 로직은 `SkillComponent`가 소유한다. (장착/쿨타임/발동)
- 1차 목표는 **동작 우선**: 아이콘 표시 + 쿨타임 차감 + 키입력 발동.
- 복잡한 연출(`SkillActor` 고도화, FSM Skill 상태 전환)은 2차 단계로 분리한다.

---

## 1. 현재 코드 기준 핵심 사실

- `COMPONENT_TYPE_TEXTURE_SKILL_ICON`은 이미 존재한다.
  - `Server/Protobuf/Protocol/Enum.proto`에 정의됨.
  - `TextureTable.csv`에 스킬 게이지/아이콘 2장 등록되어 있음.
- `ResourceLoader::Load_SkillTable`, `Load_Skills`, `Get_SkillData`는 함수 틀만 있고 런타임 조회는 미완성이다.
- `UI_PlayerSkill` 클래스는 비어 있다.
- 플레이어 입력/FSM은 `Idle/Run/Jump` 중심이며 스킬 입력/스킬 상태는 아직 없다.
- `ResourceLoader` 인스턴스는 로딩 단계(`Loader`) 수명에 묶여 있어, 런타임 전역 조회 저장소로 쓰기엔 부적합하다.

---

## 2. 아키텍처 결정 (이번 구현안)

### 2-1. 데이터 조회 경로

- `ResourceLoader`는 스킬 테이블(JSON) **파싱 전용**으로 제한한다.
- 파싱 결과 저장/조회는 `SkillDataManager`가 담당한다.
  - 저장: `ResourceLoader::Load_Skills()` -> `SkillDataManager::Register_Skill(...)`
  - 조회: `SkillComponent`, `UI_PlayerSkill` -> `SkillDataManager::Get_SkillData(...)`
- `SkillDataManager`는 런타임 읽기 전용 데이터 저장소 역할만 수행한다.
  - 쿨타임/장착/발동 상태는 절대 보관하지 않는다. (이 상태는 `SkillComponent` 책임)

### 2-2. 스킬 발동 경로

- `PlayerController::Update()`에서 스킬 키 입력(`Q`, `E`) 감지.
- `MyPlayer`가 가진 `SkillComponent`에 `Try_Activate(slot)` 호출.
- 이번 단계에서는 FSM에 `Skill` 상태를 강제하지 않는다.
  - 이동 FSM 구조를 깨지 않고 기능만 먼저 올리기 위함.

### 2-3. UI 갱신 경로

- `UI_PlayerHUD` 아래에 `UI_PlayerSkill` 패널 추가.
- `UI_PlayerSkill`은 `UI_SkillSlot` 2개를 자식으로 생성.
- `UI_PlayerSkill::Update()`에서 플레이어의 `SkillComponent`를 조회해:
  - 슬롯별 `SrvIndex` 반영
  - `CooldownRatio` 반영

### 2-4. 책임 분리 원칙

- `ResourceLoader`: 파일 파싱/검증/등록 호출만 담당
- `SkillDataManager`: 스킬 정적 메타데이터 저장/조회 담당
- `SkillComponent`: 플레이어별 동적 상태(장착, 쿨타임, 마나, 발동) 담당
- `UI`: 표시 전용 (입력/게임플레이 상태 변경 금지)

---

## 3. 데이터 포맷 (이번안)

> 주의: 현재 `ConvertResource.py`는 일반 리소스 테이블 형식(`Type/Id/Path/...`) 중심이다.  
> 스킬 데이터는 우선 **JSON 수동 관리**로 진행한다.

### 3-1. `FSkillData` 제안

```cpp
struct FSkillData
{
    int      skillID = 0;
    wstring  skillName = L"";
    uint32   srvIndex = 0;      // COMPONENT_TYPE_TEXTURE_SKILL_ICON 내부 SRV 인덱스
    float    coolDown = 0.f;
    int      manaCost = 0;
};
```

### 3-2. `SkillDataTable.json` 예시

경로: `Client/Bin/Resources/Data/json/SkillDataTable.json`

```json
{
  "Skill": [
    { "SkillID": 1001, "SkillName": "Rasengan", "SrvIndex": 2, "Cooldown": 8.0, "ManaCost": 30 },
    { "SkillID": 1002, "SkillName": "Chidori",  "SrvIndex": 3, "Cooldown": 8.0, "ManaCost": 30 }
  ]
}
```

- `SrvIndex` 기준:
  - `0,1`: 게이지 베이스
  - `2,3`: `Skill_Icon0/1`

---

## 4. 구현 상세 (파일 단위)

## 4-1. Protobuf / Enum

- 파일: `Server/Protobuf/Protocol/Enum.proto`
- 작업:
  - `COMPONENT_TYPE_SKILL = 1008;` 추가
- 후속:
  - protobuf 재생성으로 `Engine/Public/Enum.pb.h` 반영

## 4-2. SkillDataManager + Skill 데이터 로더

- 파일(신규):
  - `Client/Public/SkillDataManager.h`
  - `Client/Private/SkillDataManager.cpp`
- 역할:
  - `skillID -> FSkillData` 보관
  - 등록/조회/초기화 API 제공
  - 런타임 상태는 보관하지 않음

```cpp
class SkillDataManager : public Base
{
    DECLARE_SINGLETON(SkillDataManager)

public:
    bool                Register_Skill(const FSkillData& data);
    const FSkillData*   Get_SkillData(int skillID) const;
    bool                Has_Skill(int skillID) const;
    void                Clear();

private:
    umap<int, FSkillData> _skillMap;
};
```

- 파일: `Client/Public/Client_Struct.h`
  - `FSkillData`를 `srvIndex`, `manaCost` 포함 구조로 확장
- 파일: `Client/Public/ResourceLoader.h`
  - `Load_SkillTable`, `Load_Skills`는 유지
  - `Get_SkillData`는 제거하거나 `SkillDataManager` 포워딩으로 축소
- 파일: `Client/Private/ResourceLoader.cpp`
  - `Load_Skills`에서 실제 파싱 후 `SkillDataManager`에 등록
- 파일: `Client/Private/Loader.cpp`
  - 리소스 로딩 시점에 `Load_SkillTable("../../Client/Bin/Resources/Data/json/SkillDataTable.json")` 호출

```cpp
HRESULT ResourceLoader::Load_Skills(const json& data)
{
    auto& mgr = *GET_SINGLE(SkillDataManager);
    mgr.Clear();

    for (const auto& item : data)
    {
        FSkillData skill;
        skill.skillID   = item["SkillID"];
        skill.skillName = Utils::ToWString(item.value("SkillName", string{}));
        skill.srvIndex  = item.value("SrvIndex", 0);
        skill.coolDown  = item.value("Cooldown", 0.f);
        skill.manaCost  = item.value("ManaCost", 0);

        mgr.Register_Skill(skill);
    }
    return S_OK;
}
```

## 4-3. SkillComponent 신규

- 파일(신규):
  - `Client/Public/SkillComponent.h`
  - `Client/Private/SkillComponent.cpp`
- 역할:
  - 슬롯 2개 장착
  - 쿨타임 감소(`Update`)
  - 발동 시 마나 소모 + 쿨타임 시작
  - 스킬 메타 조회는 `SkillDataManager` 사용
- 기본 인터페이스:

```cpp
class SkillComponent : public Component
{
    GENERATED_COMPONENT(SkillComponent, Protocol::COMPONENT_TYPE_SKILL)

public:
    struct FSkillComponentDesc
    {
        int slotSkillID[2] = { 0, 0 };
    };

    HRESULT Initialize(void* arg) override;
    void    Update(float dt) override;

    bool    Try_Activate(int slot);
    int     Get_EquippedSkillID(int slot) const;
    float   Get_CooldownRatio(int slot) const;
};
```

## 4-4. MyPlayer 연결

- 파일: `Client/Public/MyPlayer.h`, `Client/Private/MyPlayer.cpp`
- 작업:
  - `Shared<SkillComponent> _skill;` 추가
  - `Initialize()`에서 `Add_Component(COMPONENT_TYPE_SKILL, _skill, &desc)` 호출
  - 초기 장착 스킬 ID 2개 세팅

## 4-5. 입력 처리 (FSM 무침투 1차안)

- 파일: `Client/Public/InputComponent.h`, `Client/Private/InputComponent.cpp`
  - `skill1Down`, `skill2Down` 프레임 입력 추가 (`Q`, `E`)
- 파일: `Client/Private/PlayerController.cpp`
  - `_input->Update_Input(dt)` 이후:
    - `skill1Down`이면 `SkillComponent::Try_Activate(0)`
    - `skill2Down`이면 `SkillComponent::Try_Activate(1)`

## 4-6. UI_SkillSlot 신규

- 파일(신규):
  - `Client/Public/UI_SkillSlot.h`
  - `Client/Private/UI_SkillSlot.cpp`
- 패턴:
  - `UI_PlayerHP`와 동일한 렌더 흐름
  - `COMPONENT_TYPE_TEXTURE_SKILL_ICON` 바인딩
  - `Set_SrvIndex`, `Set_CooldownRatio` 제공

## 4-7. UI_PlayerSkill 구현

- 파일: `Client/Public/UI_PlayerSkill.h`, `Client/Private/UI_PlayerSkill.cpp`
- 작업:
  - `Panel` 파생 실제 구현
  - 자식 `UI_SkillSlot` 2개 생성
  - `Update()`에서 `MyPlayer`의 `SkillComponent`를 찾아 슬롯 데이터 반영

## 4-8. UI_PlayerHUD에 패널 추가 (우하단)

- 파일: `Client/Public/UI_PlayerHUD.h`, `Client/Private/UI_PlayerHUD.cpp`
- 작업:
  - `Shared<UI_PlayerSkill> _skillPanel;` 추가
  - `Initialize()`에서 `Create_Child<UI_PlayerSkill>()`
  - 위치를 우하단 기준으로 계산

```cpp
const float marginRight = 140.f;
const float marginBottom = 90.f;

float x = GAME->Get_WindowWidth() - marginRight;
float y = GAME->Get_WindowHeight() - marginBottom;

_skillPanel->Get_Transform()->Set_LocalPosition(x, y, _zOrder);
```

---

## 5. 단계별 구현 순서

### Phase A: 데이터 + 컴포넌트

1. `Enum.proto`에 `COMPONENT_TYPE_SKILL` 추가
2. `SkillDataManager` 신규 추가
3. `FSkillData`/`ResourceLoader` 완성 (로더는 매니저에 등록만 수행)
4. `SkillDataTable.json` 작성 및 로드 연결
5. `SkillComponent` 생성 및 `MyPlayer` 장착

### Phase B: 입력 + 발동

1. `InputComponent`에 `Q/E` 프레임 입력 추가
2. `PlayerController`에서 `Try_Activate(slot)` 호출
3. 쿨타임/마나 동작 검증

### Phase C: UI 우하단 2슬롯

1. `UI_SkillSlot` 구현
2. `UI_PlayerSkill` 구현
3. `UI_PlayerHUD`에 자식 패널 연결 및 우하단 배치
4. 슬롯 아이콘/쿨타임 시각 반영 검증

---

## 6. 검증 체크리스트

- 게임 시작 후 우하단에 슬롯 2개 표시되는가
- 슬롯 0/1이 각각 SRV 2/3 아이콘을 표시하는가
- `Q/E` 입력 시 스킬 발동되고 쿨타임이 시작되는가
- 쿨타임 중 재발동이 차단되는가
- 쿨타임 종료 후 재발동 가능한가
- 해상도 변경(`1600x900` 외)에서도 우하단 정렬이 유지되는가

---

## 7. 2차 확장 (선택)

- FSM에 `EPlayerState::Skill` 복구 및 `PlayerState_Skill` 추가
- `SkillActor`(투사체/범위형) 분기 구현
- 쿨타임 오버레이 셰이더 추가
- `ConvertResource.py`에 `SkillDataTable.csv -> SkillDataTable.json` 전용 변환 규칙 추가

---

## 8. 매니저 도입 가이드라인

### 8-1. SkillDataManager를 쓰는 조건 (현재 프로젝트는 해당)

- 로더 수명과 런타임 조회 수명이 다를 때
- 같은 데이터를 여러 시스템(`SkillComponent`, UI, AI)이 읽을 때
- 데이터가 읽기 중심이며 전역 참조가 필요할 때
- 책임 분리(파싱 vs 저장/조회)가 필요할 때

### 8-2. 매니저를 쓰지 않아도 되는 조건

- 데이터 사용처가 1곳이고 로더 생존 구간 안에서만 쓰일 때
- 임시 프로토타입이며 런타임 조회 수명 문제가 없을 때

### 8-3. 금지 규칙

- `SkillDataManager`에 쿨타임/현재마나/입력상태 저장 금지
- `SkillComponent`가 JSON 파일 직접 파싱 금지
- UI가 매니저 데이터 수정 금지 (읽기만)

### 8-4. 의존 방향 고정

`ResourceLoader` -> `SkillDataManager` -> (`SkillComponent`, `UI_PlayerSkill`)

- 반대 방향 의존(`SkillComponent`가 `ResourceLoader` 직접 참조)은 만들지 않는다.
