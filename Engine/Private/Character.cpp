#include "pch.h"
#include "Character.h"
#include "Controller.h"
#include "MovementComponent.h"
#include "Shader.h"

Character::Character(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : ContainerObject(device, context)
{
}

Character::Character(const Character& rhs)
    : ContainerObject(rhs)
{
}

Character::~Character()
{
}

HRESULT Character::Initialize_Prototype()
{
    ContainerObject::Initialize_Prototype();

    return S_OK;
}

HRESULT Character::Initialize(void* arg)
{
    ContainerObject::Initialize(arg);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    _hitColorRemainTime = 0.f;


    return S_OK;
}

void Character::BeginPlay()
{
    ContainerObject::BeginPlay();


}

void Character::Priority_Update(float timeDelta)
{
    ContainerObject::Priority_Update(timeDelta);
}

void Character::Update(float timeDelta)
{
    ContainerObject::Update(timeDelta);

    Update_HitColor(timeDelta);
}

void Character::Late_Update(float timeDelta)
{
    ContainerObject::Late_Update(timeDelta);
}

HRESULT Character::Render()
{
    ContainerObject::Render();

    return S_OK;
}

HRESULT Character::Bind_Lights()
{

    return S_OK;
}

const wchar_t* Character::Resolve_HitSoundFile(int32 hitSound)
{
    switch (hitSound)
    {
    case 1:
        return L"Hand_Hit.wav";
    default:
        return nullptr;
    }
}

bool Character::Try_PlayDirectHitSoundFile(const FDamageEvent& damageEvent)
{
    if (damageEvent.hitSoundFile.empty())
        return false;

    const wstring soundFile = Utils::ToWString(damageEvent.hitSoundFile);
    GAME->Play_Sound(soundFile, ESoundChannel::Effect, 0.4f);
    return true;
}

void Character::Play_HitSoundFromDamageEvent(const FDamageEvent& damageEvent)
{
    if (Try_PlayDirectHitSoundFile(damageEvent))
        return;

    if (damageEvent.hitSound <= 0)
        return;

    const wchar_t* soundFile = Resolve_HitSoundFile(damageEvent.hitSound);
    if (!soundFile)
        return;

    GAME->Play_Sound(soundFile, ESoundChannel::Effect, 0.4f);
}

void Character::TakeDamage(const FDamageEvent& damageEvent)
{
    if (damageEvent.launchPower > 0.f || damageEvent.launchUp > 0.f)
    {
        Vec3 knockDir = Vec3::Zero;

        if (damageEvent.hasCustomDir)
        {
            knockDir = damageEvent.damageDir;
            knockDir.y = 0.f;
            knockDir = Utils::Safe_Normalize(knockDir);
        }
        else if (damageEvent.damageCauser)
        {
            Vec3 causerPos = damageEvent.damageCauser->Get_Transform()->Get_WorldPosition();
            Vec3 myPos = _transformCom->Get_WorldPosition();

            knockDir = myPos - causerPos;
            knockDir.y = 0.f;
            knockDir = Utils::Safe_Normalize(knockDir, Vec3::Forward);
        }

        // 援ы븳 諛⑺뼢媛믪뿉 ?곗씠???곸슜
        Vec3 launchVelocity = knockDir * damageEvent.launchPower;
        launchVelocity.y = damageEvent.launchUp;

        auto movement = Get_Component<MovementComponent>();
        if (movement)
        {
            movement->Launch(launchVelocity, false, true);
        }
    }

    auto& hub = GAME->Get_DelegateHub();

    hub.OnDamaged.Broadcast(
        static_pointer_cast<Character>(GetSharedPtr()), damageEvent.damage, damageEvent.damageCauser);

    // ?먯떇 ?대옒?ㅻ퀎 ?꾩슜 泥섎━ (FSM ?곹깭泥섎━)
    OnDamaged(damageEvent);
}

void Character::OnDamaged(const FDamageEvent& damageEvent)
{
     if (damageEvent.damage > 0.f)
    {
        Play_HitSoundFromDamageEvent(damageEvent);
        Start_HitColor();
    }
}

void Character::OnDead(const FDamageEvent& damageEvent)
{
    auto& hub = GAME->Get_DelegateHub();
    hub.OnDead.Broadcast(static_pointer_cast<Character>(GetSharedPtr()), damageEvent.damageCauser);
}

HRESULT Character::Ready_Components()
{
    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    return S_OK;
}

void Character::Start_HitColor()
{
    _hitColorRemainTime = _hitColorDuration;
}

void Character::Update_HitColor(float timeDelta)
{
    if (_hitColorRemainTime <= 0.f)
        return;

    _hitColorRemainTime -= timeDelta;

    if (_hitColorRemainTime < 0.f)
        _hitColorRemainTime = 0.f;
}

float Character::Get_HitColorStrength() const
{
    if (_hitColorDuration <= FLT_EPSILON)
        return 0.f;

    const float normalizedRemain = _hitColorRemainTime / _hitColorDuration;
    return _hitColorMaxStrength * max(0.f, min(1.f, normalizedRemain));
}

HRESULT Character::Bind_HitColor_ShaderParams(const Shared<Shader>& shader) const
{
    CHECK_NULL(shader, E_FAIL);

    const Vec4 hitColor = _hitColor;
    const float hitColorStrength = Get_HitColorStrength();

    CHECK_FAILED(shader->Bind_RawValue("g_HitColor", &hitColor, sizeof(Vec4)), E_FAIL);
    CHECK_FAILED(shader->Bind_RawValue("g_HitColorStrength", &hitColorStrength, sizeof(float)), E_FAIL);

    return S_OK;
}

void Character::Free()
{
    ContainerObject::Free();
}
