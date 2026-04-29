#include "pch.h"
#include "Skill_Kamui.h"
#include "GameObject_Factory.h"
#include "GameObject.h"
#include "Collider.h"
#include "Character.h"
#include "MyPlayer.h"
#include "EffectComponent.h"
#include "Transform.h"
#include "Utils.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_Kamui, Protocol::OBJECT_TYPE_SKILL_KAMUI, "SkillSpawn");

Skill_Kamui::Skill_Kamui(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject(device, context)
{
}

Skill_Kamui::Skill_Kamui(const Skill_Kamui& rhs)
    : SkillObject(rhs)
    , _baseDamage(rhs._baseDamage)
    , _finalDamage(rhs._finalDamage)
    , _midHitLaunchForce(rhs._midHitLaunchForce)
    , _midHitLaunchUp(rhs._midHitLaunchUp)
    , _finalHitLaunchForce(rhs._finalHitLaunchForce)
    , _finalHitLaunchUp(rhs._finalHitLaunchUp)
    , _currentHitStep(rhs._currentHitStep)
    , _finalExplosionSoundFile(rhs._finalExplosionSoundFile)
    , _tickHitSoundFile(rhs._tickHitSoundFile)
    , _hasPlayedFinalExplosionSound(rhs._hasPlayedFinalExplosionSound)
{
}

HRESULT Skill_Kamui::Initialize_Prototype()
{
    _lifetime = 1.0f; 
    _maxHitCount = 10;
    _hitInterval = 0.10f;
    _hitLaunchForce = 0.f;

    _colliderRadius = 1.8f;
    _collisionPreset = Collision_Preset::Player_Attack;

    return SkillObject::Initialize_Prototype();
}

HRESULT Skill_Kamui::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject::Initialize(arg), E_FAIL);

    auto* desc = static_cast<FSkillObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    if (_transformCom)
        _transformCom->Set_WorldPosition(desc->spawnPosition);

    if (_collider)
        _collider->Set_IsActive(true);

    _currentHitStep = 0;
    _lastAppliedStepByTarget.clear();
    _hasPlayedFinalExplosionSound = false;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = "Kamui";
    playDesc.loopOverride = false;

    if (_effectCom)
        CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void Skill_Kamui::Update(float timeDelta)
{
    SkillObject::Update(timeDelta);

    if (Is_Destroy())
        return;

    const int32 nextStep = min(_maxHitCount, static_cast<int32>(_elapsedTime / _hitInterval) + 1);
    _currentHitStep = nextStep;
}

void Skill_Kamui::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (Is_Destroy())
        return;

    auto* character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    // 카무이는 접촉 시작 순간의 흡입/찢김 느낌이 중요하므로 첫 overlap에서 전용 히트 파티클을 먼저 재생한다.
    Spawn_KamuiHitEffect(character);
    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kamui::OnStayOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnStayOverlap(self, other);

    if (Is_Destroy())
        return;

    auto* character = Find_HitCharacter(other);
    if (!character)
        return;

    auto otherOwner = other->Get_Owner();
    CHECK_NULL(otherOwner);

    Process_MultiHit(character, otherOwner.get());
}

void Skill_Kamui::OnEndOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnEndOverlap(self, other);

    if (!other)
        return;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return;

    _lastAppliedStepByTarget.erase(otherOwner.get());
}

Character* Skill_Kamui::Find_HitCharacter(Shared<Collider> other)
{
    if (!other || Is_Destroy())
        return nullptr;

    auto otherOwner = other->Get_Owner();
    if (!otherOwner)
        return nullptr;

    if (otherOwner == Get_Owner())
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void Skill_Kamui::Spawn_KamuiHitEffect(Character* hitted)
{
    // 카무이에 닿은 대상의 중심보다 약간 위에서 이펙트를 터뜨려 얼굴/상체 쪽 임팩트가 더 잘 보이게 만든다.
    CHECK_NULL(hitted);

    if (_hitEffectAssetName.empty())
        return;

    auto targetTransform = hitted->Get_Transform();
    if (!targetTransform)
        return;

    Vec3 hitEffectPosition = targetTransform->Get_WorldPosition();
    hitEffectPosition.y += 1.0f;

    Spawn_Effect_Once(_hitEffectAssetName, hitEffectPosition, Vec3(1.1f));
}

void Skill_Kamui::Process_MultiHit(Character* hitted, GameObject* targetKey)
{
    CHECK_NULL(hitted);
    CHECK_NULL(targetKey);

    if (_currentHitStep <= 0 || _currentHitStep > _maxHitCount)
        return;

    if (_lastAppliedStepByTarget[targetKey] == _currentHitStep)
        return;

    const bool isFinalHit = (_currentHitStep == _maxHitCount);

    if (!isFinalHit)
    {
        if (!Apply_Skill_Hit(hitted, _baseDamage, _midHitLaunchForce, _midHitLaunchUp))
            return;

        // 카무이 지속 흡입은 틱 데미지가 들어갈 때마다 작은 히트음을 재생해서 연속 타격감을 준다.
        GAME->Play_Sound(_tickHitSoundFile, ESoundChannel::Effect, 0.3f);
    }
    else
    {
        if (!_hasPlayedFinalExplosionSound)
        {
            // 마지막 폭발 타이밍에는 생성/흡입 단계와 구분되는 전용 폭발 사운드를 1회 재생한다.
            GAME->Play_Sound(_finalExplosionSoundFile, ESoundChannel::Effect, 0.45f);
            _hasPlayedFinalExplosionSound = true;
        }

        FDamageEvent damageEvent{};
        damageEvent.damage = _finalDamage;
        damageEvent.damageCauser = Get_Owner();
        damageEvent.launchPower = _finalHitLaunchForce;
        damageEvent.launchUp = _finalHitLaunchUp;
        damageEvent.forceHitRestart = true;
        damageEvent.hitReactionType = EHitReactionType::Launch;

        Vec3 damageDir = Vec3::Zero;
        if (_transformCom)
        {
            damageDir = _transformCom->Get_WorldForward();
            damageDir.y = 0.f;
            damageDir = Utils::Safe_Normalize(damageDir, Vec3::Zero);
        }

        if (damageDir.LengthSquared() > FLT_EPSILON)
        {
            damageEvent.damageDir = damageDir;
            damageEvent.hasCustomDir = true;
        }

        hitted->TakeDamage(damageEvent);

        auto myPlayer = dynamic_pointer_cast<MyPlayer>(Get_Owner());
        if (myPlayer)
            myPlayer->Add_ComboHit();
    }

    _lastAppliedStepByTarget[targetKey] = _currentHitStep;
    ++_hitCount;
}

Shared<GameObject> Skill_Kamui::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_Kamui>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_Kamui");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_Kamui::Clone(void* arg)
{
    auto clone = make_shared<Skill_Kamui>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_Kamui");
        return nullptr;
    }

    return clone;
}

void Skill_Kamui::Free()
{
    SkillObject::Free();
}
