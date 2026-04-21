# 전투 RadialLine HUD 구현 문서

## 개요

이 문서는 `UI_PlayerHUD` 안에 `RadialLine` 기반 전투 임팩트 HUD를 넣는 최신 기준안이다.

이번 문서에서는 아래만 다룬다.

- 실제 레포 안에 있는 `RadialLine` 텍스처만 사용
- `OnPlayerComboHit` 기준 burst 연출만 구현

즉, 이 문서의 목표는 아래 한 줄이다.

> `UI_PlayerHUD + Background + Announce RadialLine Texture + OnPlayerComboHit` 조합으로 만화식 방사형 집중선을 붙인다.

---

## [대체] 실제 사용하는 경로

이번 문서에서 실제로 쓰는 텍스처 경로는 아래 3개다.

- `Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLine_01_Additive_BC.png`
- `Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLine_02_Additive_BC.png`
- `Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLineAdditive_BC.png`

참고용 머티리얼 경로:

- `Client/Bin/Resources/Textures/UI/ingame/Announce/Materials/MI_UI_AddTex_Announce_01_RadialLine.props.txt`
- `Client/Bin/Resources/Textures/UI/ingame/Announce/Materials/MI_UI_AddTex_Announce_02_RadialLine.props.txt`
- `Client/Bin/Resources/Textures/UI/ingame/Announce/Materials/MI_UI_AddTex_Announce_RadialLine.props.txt`

## [대체] 왜 RadialLine으로 가는가

이번 연출은 `질주 속도감`보다 `타격 순간 집중선`에 가깝다.
지금 원하는 레퍼런스는 화면 중심 근처에서 바깥으로 퍼지는 방사형 임팩트 쪽에 더 가깝다.

그래서 1차 구현은 아래처럼 고정한다.

1. 화면 중심보다 약간 위를 burst 중심으로 잡는다.
2. `RadialLine` 3장을 서로 다른 크기와 회전값으로 겹친다.
3. `OnPlayerComboHit`가 들어오면 짧게 알파/스케일을 올렸다가 감쇠시킨다.

---

## [대체] 최종 수정 파일

이번 기준안에서 실제 수정할 파일은 아래 3개다.

1. `Client/Bin/Resources/Data/json/DT_Texture.json`
2. `Client/Public/UI_PlayerHUD.h`
3. `Client/Private/UI_PlayerHUD.cpp`

새 폴더 생성은 하지 않는다.
새 png 복사도 하지 않는다.

---

## [대체] 리소스 등록 방식

### 왜 `COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT`에 붙이는가

현재 `DT_Texture.json`은 동일한 `Id`에 여러 텍스처를 append 해서 SRV index를 늘리는 구조다.

이번 `RadialLine`은 성격상 `Announce` 계열 UI라서 아래 방식이 가장 자연스럽다.

- 기존 `COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT` 유지
- 뒤에 `RadialLine` 3장 append
- `UI_PlayerHUD`에서 해당 index만 직접 사용

새 enum이나 새 component type은 만들지 않는다.

### 기존 인덱스

현재 `COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT` 기본 인덱스는 아래다.

- `0` : `T_UI_Announce_Hit_Text_LOCL_0.png`
- `1` : `T_UI_Announce_Hit_Small_LOCL.png`
- `2` : `T_UI_Announce_KO_BC.png`

### 새 인덱스

이번 문서 기준으로 아래 3장을 append 한다.

- `3` : `T_UI_Announce_RadialLine_01_Additive_BC.png`
- `4` : `T_UI_Announce_RadialLine_02_Additive_BC.png`
- `5` : `T_UI_Announce_RadialLineAdditive_BC.png`

---

## [변경] `Client/Bin/Resources/Data/json/DT_Texture.json`

기존 `COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT` 항목들 바로 아래에 다음 3개를 추가한다.

```json
        {
            "Id": "COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT",
            "Path": "../../Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLine_01_Additive_BC.png",
            "Count": 1,
            "Level": "Static"
        },
        {
            "Id": "COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT",
            "Path": "../../Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLine_02_Additive_BC.png",
            "Count": 1,
            "Level": "Static"
        },
        {
            "Id": "COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT",
            "Path": "../../Client/Bin/Resources/Textures/UI/ingame/Announce/Textures/T_UI_Announce_RadialLineAdditive_BC.png",
            "Count": 1,
            "Level": "Static"
        }
```

최종 사용 index는 아래처럼 고정한다.

- `3` : 가장 큰 바깥쪽 방사선
- `4` : 두 번째 방사선
- `5` : 중심 밀도를 보강하는 안쪽 방사선

---

## [대체] HUD 배치 규칙

이번 최신안은 화면 중앙보다 살짝 위에 집중선을 놓는다.

기준 배치:

- 중심 X = `uiRefWidth * 0.5f`
- 중심 Y = `uiRefHeight * 0.42f`

레이어 구성:

1. 큰 RadialLine
2. 중간 RadialLine
3. 작은 RadialLine

각 레이어는 아래 값만 다르게 준다.

- `sizeX`, `sizeY`
- `rotation`
- `alphaWeight`
- `scaleJitter`

이렇게 하면 단순히 한 장 띄우는 것보다 훨씬 만화 느낌이 잘 산다.

---

## [변경] `Client/Public/UI_PlayerHUD.h`

아래 코드는 이번 문서 기준 최종 헤더 예시다.

새로 추가하는 변수와 함수에는 역할 주석을 모두 넣었다.

```cpp
#pragma once

#include "HUD.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Player;
class UI_PlayerStatus;
class UI_PlayerSkill;
class UI_AnnounceCombo;
class UI_Targeting;
class Background;

class CombatStat;
class UI_PlayerHP;

DECLARE_DELEGATE(FOnHUDPlayerBound, Shared<Player>);

class UI_PlayerHUD : public HUD
{
    GENERATED_BODY(UI_PlayerHUD)

private:
    // 원격 플레이어 상태 UI를 유지하기 위한 엔트리
    struct FRemotePlayerStatusEntry
    {
        uint64 networkId = 0;
        Weak<Player> player;
        Weak<CombatStat> combat;
        Shared<UI_PlayerHP> hpBar;
        Shared<UI_Text> nameText;
    };

    // 방사형 집중선 한 장의 배치와 연출 값을 정의하는 레이어 정보
    struct FRadialLineLayer
    {
        // 실제 화면에 등록되는 Background UI
        Shared<Background> widget;

        // COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT 내부 SRV 인덱스
        uint32 textureIndex = 0;

        // HUD 기준 중심 배치 좌표
        float basePosX = 0.f;
        float basePosY = 0.f;

        // HUD 기준 기본 크기
        float baseSizeX = 0.f;
        float baseSizeY = 0.f;

        // 레이어별 기본 회전값
        float baseRotation = 0.f;

        // 레이어별 알파 가중치
        float alphaWeight = 1.f;

        // burst 중 미세한 스케일 흔들림 강도
        float scaleJitter = 0.f;

        // 각 레이어가 동일 타이밍으로 흔들리지 않게 만드는 위상값
        float phaseOffset = 0.f;
    };

public:
    explicit UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerHUD(const UI_PlayerHUD& rhs);
    virtual ~UI_PlayerHUD() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Update(float timeDelta) override;

    void        Bind_Player(Shared<Player> player);

public:
    void Add_RemotePlayer(Shared<Player> player);
    void Update_RemotePlayerStatusList();
    bool Has_RemotePlayerStatus(uint64 networkId) const;

    void Handle_RemotePlayerObjectSpawned(Shared<GameObject> obj);

    // 콤보 히트가 발생했을 때 방사형 집중선 burst를 시작하는 함수
    void On_PlayerComboHit(uint32 combo);

private:
    HRESULT     Ready_UI(void* arg);

    // RadialLine Background 레이어들을 생성하는 함수
    HRESULT     Ready_CombatRadialLines();

    // 현재 해상도 기준으로 레이어 기본 배치를 다시 계산하는 함수
    void        Reset_CombatRadialLineLayout();

    // burst 상태를 매 프레임 갱신하면서 알파와 스케일을 업데이트하는 함수
    void        Update_CombatRadialLineBurst(float timeDelta);

    // hold/fade 타이머를 기준으로 현재 알파를 계산하는 함수
    float       Compute_CombatRadialLineAlpha() const;

    // 콤보 수를 연출 강도로 환산하는 함수
    float       Compute_CombatRadialLineIntensity(uint32 combo) const;

    // 외부 이벤트를 받아 집중선 burst를 시작하는 함수
    void        Trigger_CombatRadialLineBurst(float intensity);

    // burst 종료 후 모든 레이어를 숨기는 함수
    void        Stop_CombatRadialLineBurst();

public:
    FOnHUDPlayerBound OnHUDPlayerBound;

private:
    Shared<UI_PlayerStatus>     _status;
    Shared<UI_PlayerSkill>      _skillPanel;
    Shared<UI_AnnounceCombo>    _announceCombo;
    Shared<UI_Targeting>        _targeting;

    // 방사형 집중선 레이어 목록
    vector<FRadialLineLayer>    _combatRadialLineLayers;

    // 원격 플레이어 스폰 이벤트 해제용 핸들
    FDelegateHandle             _remotePlayerSpawnedHandle = {};

    // 콤보 히트 이벤트 해제용 핸들
    FDelegateHandle             _comboHitHandle = {};

    // 원격 플레이어 상태 UI 목록
    vector<FRemotePlayerStatusEntry> _remotePlayerStatuses;

    // burst 강한 구간 유지 시간
    float                       _combatRadialLineHoldTimer = 0.f;

    // fade 구간 유지 시간
    float                       _combatRadialLineFadeTimer = 0.f;

    // 레이어 흔들림용 누적 시간
    float                       _combatRadialLinePhaseTime = 0.f;

    // 현재 burst 세기
    float                       _combatRadialLineIntensity = 1.f;

    // burst 활성 여부
    bool                        _isCombatRadialLineBurstActive = false;

private:
    // DT_Texture.json에 append 한 RadialLine 인덱스
    static constexpr uint32 COMBAT_RADIAL_LINE_TEXTURE_INDEX_A = 3;
    static constexpr uint32 COMBAT_RADIAL_LINE_TEXTURE_INDEX_B = 4;
    static constexpr uint32 COMBAT_RADIAL_LINE_TEXTURE_INDEX_C = 5;

    // burst가 가장 세게 보이는 유지 시간
    static constexpr float COMBAT_RADIAL_LINE_HOLD_TIME = 0.08f;

    // burst가 자연스럽게 감쇠되는 시간
    static constexpr float COMBAT_RADIAL_LINE_FADE_TIME = 0.28f;

public:
    static Shared<UI_PlayerHUD> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

---

## [변경] `Client/Private/UI_PlayerHUD.cpp`

### 1. include 추가

```cpp
#include "Background.h"
```

### 2. `Initialize()` 수정

```cpp
HRESULT UI_PlayerHUD::Initialize(void* arg)
{
    CHECK_FAILED(HUD::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_UI(arg), E_FAIL);

    OnHUDPlayerBound.Add(_status.get(), &UI_PlayerStatus::Bind_Player);
    OnHUDPlayerBound.Add(_skillPanel.get(), &UI_PlayerSkill::Bind_Player);

    _remotePlayerSpawnedHandle =
        GAME->Get_DelegateHub().OnRemotePlayerObjectSpawned.Add(
            this, &UI_PlayerHUD::Handle_RemotePlayerObjectSpawned);

    _comboHitHandle =
        GAME->Get_DelegateHub().OnPlayerComboHit.Add(
            this, &UI_PlayerHUD::On_PlayerComboHit);

    return S_OK;
}
```

### 3. `Update()` 수정

```cpp
void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);

    Update_RemotePlayerStatusList();
    Update_CombatRadialLineBurst(timeDelta);
}
```

### 4. `Ready_UI()` 마지막에 호출 추가

```cpp
    CHECK_FAILED(Ready_CombatRadialLines(), E_FAIL);
```

### 5. 새 구현 코드 추가

```cpp
void UI_PlayerHUD::On_PlayerComboHit(uint32 combo)
{
    Trigger_CombatRadialLineBurst(Compute_CombatRadialLineIntensity(combo));
}

HRESULT UI_PlayerHUD::Ready_CombatRadialLines()
{
    Reset_CombatRadialLineLayout();

    for (size_t i = 0; i < _combatRadialLineLayers.size(); ++i)
    {
        auto& layer = _combatRadialLineLayers[i];

        Background::FBackgroundDesc desc{};
        desc.posX = layer.basePosX;
        desc.posY = layer.basePosY;
        desc.sizeX = layer.baseSizeX;
        desc.sizeY = layer.baseSizeY;
        desc.zOrder = _zOrder + 0.007f + (static_cast<float>(i) * 0.0001f);
        desc.levelIndex = _levelIndex;
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_ANNOUNCE_HIT;
        desc.textureIndex = layer.textureIndex;

        layer.widget = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::Overlay,
            &desc);
        CHECK_NULL(layer.widget, E_FAIL);

        layer.widget->Set_UIRotationZ(layer.baseRotation);
        layer.widget->Set_UIOpacity(0.f);
        layer.widget->Set_Visibility(false);
    }

    return S_OK;
}

void UI_PlayerHUD::Reset_CombatRadialLineLayout()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    const float centerX = uiRefWidth * 0.5f;
    const float centerY = uiRefHeight * 0.42f;

    _combatRadialLineLayers.clear();
    _combatRadialLineLayers.reserve(3);

    {
        FRadialLineLayer layer{};
        layer.textureIndex = COMBAT_RADIAL_LINE_TEXTURE_INDEX_A;
        layer.basePosX = centerX;
        layer.basePosY = centerY;
        layer.baseSizeX = 920.f;
        layer.baseSizeY = 920.f;
        layer.baseRotation = 0.f;
        layer.alphaWeight = 0.82f;
        layer.scaleJitter = 0.05f;
        layer.phaseOffset = 0.f;
        _combatRadialLineLayers.push_back(layer);
    }

    {
        FRadialLineLayer layer{};
        layer.textureIndex = COMBAT_RADIAL_LINE_TEXTURE_INDEX_B;
        layer.basePosX = centerX;
        layer.basePosY = centerY;
        layer.baseSizeX = 760.f;
        layer.baseSizeY = 760.f;
        layer.baseRotation = 18.f;
        layer.alphaWeight = 0.62f;
        layer.scaleJitter = 0.08f;
        layer.phaseOffset = 0.9f;
        _combatRadialLineLayers.push_back(layer);
    }

    {
        FRadialLineLayer layer{};
        layer.textureIndex = COMBAT_RADIAL_LINE_TEXTURE_INDEX_C;
        layer.basePosX = centerX;
        layer.basePosY = centerY;
        layer.baseSizeX = 620.f;
        layer.baseSizeY = 620.f;
        layer.baseRotation = -12.f;
        layer.alphaWeight = 0.44f;
        layer.scaleJitter = 0.06f;
        layer.phaseOffset = 1.7f;
        _combatRadialLineLayers.push_back(layer);
    }
}

void UI_PlayerHUD::Update_CombatRadialLineBurst(float timeDelta)
{
    if (!_isCombatRadialLineBurstActive)
        return;

    _combatRadialLinePhaseTime += timeDelta;

    if (_combatRadialLineHoldTimer > 0.f)
    {
        _combatRadialLineHoldTimer = max(0.f, _combatRadialLineHoldTimer - timeDelta);
    }
    else
    {
        _combatRadialLineFadeTimer = max(0.f, _combatRadialLineFadeTimer - timeDelta);
    }

    const float alpha = Compute_CombatRadialLineAlpha();

    if (_combatRadialLineHoldTimer <= 0.f && _combatRadialLineFadeTimer <= 0.f)
    {
        Stop_CombatRadialLineBurst();
        return;
    }

    for (auto& layer : _combatRadialLineLayers)
    {
        if (!layer.widget)
            continue;

        const float phase = (_combatRadialLinePhaseTime * 12.f) + layer.phaseOffset;
        const float jitterScale = 1.f + (sinf(phase) * layer.scaleJitter * _combatRadialLineIntensity);

        layer.widget->Set_Visibility(true);
        layer.widget->Set_UIPosition(layer.basePosX, layer.basePosY);
        layer.widget->Set_UIRotationZ(layer.baseRotation);
        layer.widget->Set_UIScale(
            layer.baseSizeX * jitterScale,
            layer.baseSizeY * jitterScale);
        layer.widget->Set_UIOpacity(alpha * layer.alphaWeight);
    }
}

float UI_PlayerHUD::Compute_CombatRadialLineAlpha() const
{
    if (_combatRadialLineHoldTimer > 0.f)
        return 1.f;

    if (_combatRadialLineFadeTimer <= 0.f)
        return 0.f;

    const float ratio = _combatRadialLineFadeTimer / COMBAT_RADIAL_LINE_FADE_TIME;
    return clamp(ratio, 0.f, 1.f);
}

float UI_PlayerHUD::Compute_CombatRadialLineIntensity(uint32 combo) const
{
    const uint32 safeCombo = min<uint32>(combo, 6);
    const float intensity = 1.f + ((static_cast<float>(safeCombo) - 1.f) * 0.06f);
    return clamp(intensity, 1.f, 1.3f);
}

void UI_PlayerHUD::Trigger_CombatRadialLineBurst(float intensity)
{
    _isCombatRadialLineBurstActive = true;
    _combatRadialLineHoldTimer = COMBAT_RADIAL_LINE_HOLD_TIME;
    _combatRadialLineFadeTimer = COMBAT_RADIAL_LINE_FADE_TIME;
    _combatRadialLinePhaseTime = 0.f;
    _combatRadialLineIntensity = intensity;

    for (auto& layer : _combatRadialLineLayers)
    {
        if (!layer.widget)
            continue;

        layer.widget->Set_Visibility(true);
    }
}

void UI_PlayerHUD::Stop_CombatRadialLineBurst()
{
    _isCombatRadialLineBurstActive = false;
    _combatRadialLineHoldTimer = 0.f;
    _combatRadialLineFadeTimer = 0.f;
    _combatRadialLinePhaseTime = 0.f;
    _combatRadialLineIntensity = 1.f;

    for (auto& layer : _combatRadialLineLayers)
    {
        if (!layer.widget)
            continue;

        layer.widget->Set_UIOpacity(0.f);
        layer.widget->Set_Visibility(false);
    }
}
```

### 6. `Free()` 수정

```cpp
void UI_PlayerHUD::Free()
{
    auto& hub = GAME->Get_DelegateHub();

    if (_remotePlayerSpawnedHandle.IsValid())
    {
        hub.OnRemotePlayerObjectSpawned.Remove(_remotePlayerSpawnedHandle);
        _remotePlayerSpawnedHandle.Reset();
    }

    if (_comboHitHandle.IsValid())
    {
        hub.OnPlayerComboHit.Remove(_comboHitHandle);
        _comboHitHandle.Reset();
    }

    HUD::Free();
}
```

---

## [대체] 연출 결과

이 문서대로 구현하면 화면에서는 아래 느낌이 난다.

- 화면 중심 근처에서 방사형 집중선이 터짐
- 메인 1장만 보이는 게 아니라 3장이 겹쳐서 밀도가 생김
- 콤보 hit 순간 짧게 강하게 보였다가 바로 감쇠
- 기존 `UI_AnnounceCombo`와 같은 전투성 HUD 흐름 안에 자연스럽게 붙음

---

## [대체] 체크리스트

구현 후 확인 순서는 아래다.

1. `DT_Texture.json`에 `RadialLine` 3장이 append 되었는지 확인
2. `UI_PlayerHUD.h`에 `FRadialLineLayer`와 관련 상태 변수가 들어갔는지 확인
3. `UI_PlayerHUD.cpp`에서 `Background.h` include, `OnPlayerComboHit` 연결, burst 함수들이 들어갔는지 확인
4. 콤보 hit 시 방사형 집중선이 화면 중심 근처에서 잠깐 터지는지 확인
5. 연출이 너무 강하면 `alphaWeight`, `baseSizeX`, `baseSizeY`, `COMBAT_RADIAL_LINE_FADE_TIME`만 우선 조절

---

## 마무리

이번 문서는 `실존하는 RadialLine 경로만` 기준으로 정리한 최종안이다.

핵심만 다시 요약하면:

- 새 폴더 안 만듦
- 새 png 복사 안 함
- `Announce RadialLine` 텍스처 3장만 append
- `UI_PlayerHUD` 안에서 burst 처리

즉, 이번 구현은 `문서상 가정 경로`가 아니라 `현재 레포에서 바로 쓸 수 있는 경로`만 기준으로 작업하면 된다.
