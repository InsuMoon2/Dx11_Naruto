# JumpDash 잔상 프로토 구현 플랜

## Summary
- 구현 가능하다.
- 첫 버전은 `JumpDash` 전용으로 한정하고, `실제 캐릭터 복제`가 아니라 `Player 내부의 3D smear plane 레이어`로 만든다.
- 트리거는 상태 코드 하드코딩이 아니라 애니메이션 노티파이 스테이트로 건다.
- 범위는 `SK_CHR_NormalModel|CustomMan_Aerial_Dash_Start` 클립 1개 기준으로 고정한다.
- 이번 버전에서는 `파란 잔상 + 흰 스피드라인 + 알파 페이드`까지 구현하고, `본체 숨김/무기 숨김`은 2차로 미룬다.

## Key Changes
- 렌더러
  - `Blend` 패스에서 실제 알파 블렌드가 켜지도록 [Renderer.cpp](/d:/GitDesktop/Dx11_Naruto/Engine/Private/Renderer.cpp)를 수정한다.
  - 3D 투명 이펙트용으로 `DepthTest On + DepthWrite Off` 상태를 추가하고 `Render_Blend()`에서 사용한다.
- 셰이더
  - [Shader_Vtxtex.hlsl](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Shaders/Shader_Vtxtex.hlsl)에 `g_Alpha`를 추가하고 샘플된 텍스처 알파에 곱한다.
  - 이 셰이더를 잔상 plane, 스피드라인 plane 둘 다 재사용한다.
- Player
  - [Player.h](/d:/GitDesktop/Dx11_Naruto/Client/Public/Player.h), [Player.cpp](/d:/GitDesktop/Dx11_Naruto/Client/Private/Player.cpp)에 `JumpDash FX 상태`, `업데이트`, `렌더`를 추가한다.
  - `Player::Late_Update()`에서 FX가 활성화된 동안만 `ERenderGroup::Blend`에 자기 자신을 등록한다.
  - `Player::Render()`는 본체 렌더를 건드리지 않고 FX plane만 추가로 그린다.
- Anim Notify
  - 새 `AnimNotifyState` 타입 `ANS_JumpDashAfterImage`를 추가한다.
  - [TesetNormal.animnotify.json](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Data/json/AnimNotifies/TesetNormal.animnotify.json)에 `SK_CHR_NormalModel|CustomMan_Aerial_Dash_Start` 클립용 상태 노티파이를 추가한다.
- 텍스처
  - 프로토용 텍스처 2장을 추가한다.
  - `/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Textures/FX/T_FX_JumpDashSmear.png`
  - `/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Textures/FX/T_FX_JumpDashLine.png`
  - 텍스처는 데이터테이블에 넣지 않고 `Texture::Create()`로 Player가 직접 로드한다.

## Implementation
- FX 표현 방식
  - 파란 smear plane 3장
  - 흰 line plane 2장
  - 모두 플레이어 뒤쪽으로 배치
  - 방향은 `-dashDirection`
  - plane은 카메라를 향해 `LookAt()` 하되, 위치는 플레이어 월드 위치를 기준으로 고정
  - 지속시간은 `0.22s`
  - 알파는 `1.0 -> 0.0`
  - smear 크기 기본값은 `2.4 x 1.2`
  - line 크기 기본값은 `3.2 x 0.35`
- 트리거 정책
  - `ANS_JumpDashAfterImage::On_Begin()`에서 시작
  - `ANS_JumpDashAfterImage::On_End()`에서 강제 종료
  - `On_Tick()`은 사용하지 않는다
  - 즉 이번 버전은 “시작 시 1회 활성화 + Player 내부 타이머로 감쇠” 구조로 고정한다
- 적용 대상
  - `Player` 기반 공통 구현이므로 `MyPlayer`, `RemotePlayer` 둘 다 동일하게 적용된다
  - `Preview` 모드에서는 실행하지 않는다

## Code Skeleton
```cpp
// Client/Public/Player.h
class Player : public Character
{
    GENERATED_BODY(Player)

public:
    // JumpDash 잔상 이펙트 시작 요청이다. AnimNotifyState Begin에서 호출한다.
    void Start_JumpDashAfterImage();

    // JumpDash 잔상 이펙트 강제 종료 요청이다. AnimNotifyState End에서 호출한다.
    void Stop_JumpDashAfterImage();

protected:
    // JumpDash 잔상 상태를 매 프레임 감쇠/정리한다.
    void Update_JumpDashAfterImage(float timeDelta);

    // JumpDash 잔상 plane들을 Blend 패스에서 그린다.
    HRESULT Render_JumpDashAfterImage();

    // JumpDash 잔상용 컴포넌트를 준비한다.
    HRESULT Ready_JumpDashAfterImageResources();

protected:
    struct FJumpDashAfterImageState
    {
        // 현재 JumpDash 잔상 이펙트가 활성 상태인지 나타낸다.
        bool active = false;

        // 이펙트 누적 재생 시간이다.
        float elapsedSec = 0.f;

        // 이펙트 총 유지 시간이다.
        float durationSec = 0.22f;

        // 잔상을 배치할 때 기준으로 쓰는 대쉬 방향이다.
        Vec3 dashDirection = Vec3::Forward;

        // smear plane 시작 알파값이다.
        float smearStartAlpha = 0.85f;

        // line plane 시작 알파값이다.
        float lineStartAlpha = 0.95f;
    };

    // JumpDash 잔상 렌더 상태다.
    FJumpDashAfterImageState _jumpDashAfterImageState;

    // JumpDash 파란 smear plane 렌더용 텍스처다.
    Shared<Texture> _jumpDashSmearTexture;

    // JumpDash 흰 스피드라인 plane 렌더용 텍스처다.
    Shared<Texture> _jumpDashLineTexture;

    // JumpDash plane 렌더용 셰이더다.
    Shared<Shader> _jumpDashFxShader;

    // JumpDash plane 렌더용 사각형 버퍼다.
    Shared<VIBuffer_Rect> _jumpDashFxBuffer;
};
```

```cpp
// Client/Private/Player.cpp
void Player::Start_JumpDashAfterImage()
{
    // 기존 잔상을 즉시 덮어써도 되는 프로토 정책이다.
    _jumpDashAfterImageState.active = true;
    _jumpDashAfterImageState.elapsedSec = 0.f;

    Vec3 dashDir = _transformCom ? _transformCom->Get_WorldForward() : Vec3::Forward;
    dashDir.y = 0.f;

    if (dashDir.LengthSquared() <= FLT_EPSILON)
        dashDir = Vec3::Forward;

    dashDir.Normalize();
    _jumpDashAfterImageState.dashDirection = dashDir;
}

void Player::Stop_JumpDashAfterImage()
{
    _jumpDashAfterImageState.active = false;
    _jumpDashAfterImageState.elapsedSec = 0.f;
}

void Player::Update_JumpDashAfterImage(float timeDelta)
{
    if (!_jumpDashAfterImageState.active)
        return;

    _jumpDashAfterImageState.elapsedSec += timeDelta;

    if (_jumpDashAfterImageState.elapsedSec >= _jumpDashAfterImageState.durationSec)
    {
        Stop_JumpDashAfterImage();
    }
}

HRESULT Player::Render_JumpDashAfterImage()
{
    if (!_jumpDashAfterImageState.active)
        return S_OK;

    const float ratio = 1.f - (_jumpDashAfterImageState.elapsedSec / _jumpDashAfterImageState.durationSec);
    const Vec3 actorPos = _transformCom->Get_WorldPosition();
    const Vec3 backDir = -_jumpDashAfterImageState.dashDirection;

    // smear 3장 + line 2장을 고정 오프셋으로 그린다.
    // 각 plane 월드행렬 생성 -> g_WorldMatrix/g_ViewMatrix/g_ProjMatrix 바인딩
    // 텍스처 바인딩 -> g_Alpha 바인딩 -> Draw
    return S_OK;
}

void Player::Update(float timeDelta)
{
    Character::Update(timeDelta);

    if (_model)
        _model->Play_Animation(timeDelta, true);

    Update_JumpDashAfterImage(timeDelta);
}

void Player::Late_Update(float timeDelta)
{
    Character::Late_Update(timeDelta);

    if (_jumpDashAfterImageState.active)
        GAME->Add_RenderGroup(ERenderGroup::Blend, this->GetSharedPtr());
}

HRESULT Player::Render()
{
    CHECK_FAILED(Render_JumpDashAfterImage(), E_FAIL);
    return S_OK;
}
```

```cpp
// Client/Public/ANS_JumpDashAfterImage.h
class ANS_JumpDashAfterImage final : public AnimNotifyState
{
public:
    string Get_TypeName() const override;

    // JumpDash 시작 클립 구간에 진입하면 Player 잔상 FX를 켠다.
    void On_Begin(const FAnimNotifyContext& context) override;

    // 이번 프로토에서는 Tick 기반 제어를 하지 않는다.
    void On_Tick(const FAnimNotifyContext& context) override;

    // JumpDash 시작 클립 구간을 벗어나면 Player 잔상 FX를 정리한다.
    void On_End(const FAnimNotifyContext& context) override;
};
```

```cpp
// Client/Private/ANS_JumpDashAfterImage.cpp
REGISTER_ANIM_NOTIFY_STATE(ANS_JumpDashAfterImage)

void ANS_JumpDashAfterImage::On_Begin(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto player = dynamic_pointer_cast<Player>(context.owner->GetSharedPtr<Player>());
    if (!player)
        return;

    player->Start_JumpDashAfterImage();
}

void ANS_JumpDashAfterImage::On_Tick(const FAnimNotifyContext& context)
{
}

void ANS_JumpDashAfterImage::On_End(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto player = dynamic_pointer_cast<Player>(context.owner->GetSharedPtr<Player>());
    if (!player)
        return;

    player->Stop_JumpDashAfterImage();
}
```

```hlsl
// Client/Bin/Shaders/Shader_Vtxtex.hlsl
float4x4 g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
Texture2D g_Texture;
float g_Alpha;

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = g_Texture.Sample(DefaultSampler, In.vTexcoord);
    Out.vColor.a *= g_Alpha;
    return Out;
}
```

```json
// Client/Bin/Resources/Data/json/AnimNotifies/TesetNormal.animnotify.json
{
  "clip_name": "SK_CHR_NormalModel|CustomMan_Aerial_Dash_Start",
  "display_fps": 30,
  "notify_states": [
    {
      "start_sec": 0.0,
      "duration_sec": 0.22,
      "track_index": 0,
      "type": "ANS_JumpDashAfterImage",
      "payload": {}
    }
  ]
}
```

## Test Plan
- `Jump -> Shift` 입력으로 `JumpDash` 진입 시 smear/line plane이 0.22초 동안 보인다.
- `Run`, `Jump`, `DoubleJump`, `Dash`에서는 이 FX가 보이지 않는다.
- `JumpDash` 도중 착지/상태 전환이 빨라져도 `On_End()` 또는 내부 타이머로 FX가 정리된다.
- `MyPlayer`와 `RemotePlayer` 모두 같은 클립 재생 시 동일한 FX가 보인다.
- plane이 불투명 사각형처럼 보이지 않고 텍스처 알파대로 자연스럽게 섞인다.
- plane이 캐릭터 뒤에 남고, 깊이 테스트 때문에 벽 뒤로 튀어나오지 않는다.

## Assumptions
- 첫 버전 범위는 `JumpDash`만이다.
- 정확한 원본 재현보다 “비슷한 읽힘”을 우선한다.
- 본체 숨김/무기 숨김/실루엣 복제는 이번 범위에서 제외한다.
- 노티파이 대상 클립은 [TestPlayer2.prefab.json](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Data/json/Prefabs/TestPlayer2.prefab.json#L597) 기준 `SK_CHR_NormalModel|CustomMan_Aerial_Dash_Start`로 고정한다.
- 플레이어 모델은 현재 [DT_Model.json](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Data/json/DT_Model.json#L18) 의 `Model_TestModel -> TesetNormal.meshbin` 기준으로 작업한다.
