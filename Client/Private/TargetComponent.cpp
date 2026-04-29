#include "pch.h"
#include "TargetComponent.h"

#include "Bounding_Sphere.h"
#include "GameObject.h"
#include "Transform.h"
#include "Input_Manager.h" 
#include "Collider.h"
#include "CombatStat.h"
#include "EnemyCharacter.h"

TargetComponent::TargetComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

TargetComponent::TargetComponent(const TargetComponent& rhs)
    : Component(rhs)
{
}

HRESULT TargetComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT TargetComponent::Initialize(void* arg)
{
    CHECK_FAILED(Component::Initialize(arg), E_FAIL);

    _isLocked = false;
    _candidates.clear();

    Bounding_Sphere::FBoundingSphereDesc radarDesc{};
    radarDesc.radius = 15.f;

    _targetCollider = static_pointer_cast<Collider>(GAME->Clone_Component(ETOI(ELevelType::Static), Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, &radarDesc));
    _targetCollider->Set_IsActive(true);

    // 플레이어 공격처리
    _targetCollider->Set_CollisionPreset(Collision_Preset::Player_Target);

    return S_OK;
}

void TargetComponent::BeginPlay()
{
    Component::BeginPlay();

    if (_targetCollider && Get_Owner())
        _targetCollider->Set_Owner(Get_Owner());

}

void TargetComponent::Update_Targeting(float timeDelta)
{
    if (_targetCollider == nullptr) return;

    if (auto owner = Get_Owner())
    {
        Matrix matrix = Matrix::CreateTranslation(owner->Get_Transform()->Get_WorldPosition());
        _targetCollider->Update_Collider(matrix);

        GAME->Add_Collider(_targetCollider);

    }

    // 매 프레임 몬스터 갱신
    Update_Candiates();

    // 현재 락온 대상이 범위를 벗어나거나, 죽으면 자동 해제
    Refresh_LockedTarget();

    if (INPUT->KeyDown(KEY_TYPE::V))
    {
        LockOn_NearestTarget(true);
        return;
    }

    if (_isLocked)
    {
        Auto_SwitchToNearestTarget();
    }
    else
    {
        LockOn_NearestTarget(false);
    }
}

void TargetComponent::Late_Update(float timeDelta)
{
    if (_targetCollider)
    {
        GAME->Add_Collider(_targetCollider);
    }
}

void TargetComponent::Update_Candiates()
{
    _candidates.clear();
    if (_targetCollider == nullptr) return;
    const auto& overlapSet = _targetCollider->Get_OverlapSet();

    for (const auto& weakOtherCollider : overlapSet)
    {
        if (weakOtherCollider.expired())
            continue;

        Shared<Collider> otherCollider = weakOtherCollider.lock();
        Shared<GameObject> otherObj = otherCollider->Get_Owner();

        if (otherObj == nullptr || otherObj->Is_Destroy())
            continue;

        Shared<Character> enemyObj = dynamic_pointer_cast<EnemyCharacter>(otherObj);

         if (!enemyObj)
            continue;

        // 죽은 적은 락온 후보에서 제외한다.
        auto combatStat = enemyObj->Get_Component<CombatStat>();
        if (combatStat && combatStat->Is_Dead())
            continue;

        auto iter = find_if(_candidates.begin(), _candidates.end(),
            [&enemyObj](const Weak<Character> a)
            {
                return !a.expired() && a.lock() == enemyObj;
            });

        if (iter == _candidates.end())
            _candidates.push_back(enemyObj);

    }

}

void TargetComponent::Refresh_LockedTarget()
{
    if (_isLocked == false)
        return;

    if (_lockedTarget.expired())
    {
        Clear_Lock();
        return;
    }

    Shared<GameObject> lockedTarget = _lockedTarget.lock();
    if (lockedTarget == nullptr)
    {
        Clear_Lock();
        return;
    }

    if (Is_TargetInCandidates(lockedTarget) == false)
    {
        Clear_Lock();
    }
}

void TargetComponent::Auto_SwitchToNearestTarget()
{
    if (_isLocked == false)
        return;

    Shared<Character> nearestTarget = Find_NearestTarget(false);
    if (nearestTarget == nullptr)
        return;

    Shared<Character> currentLockedTarget = _lockedTarget.lock();
    if (currentLockedTarget == nearestTarget)
        return;

    _lockedTarget = nearestTarget;
    _isLocked = true;
}

void TargetComponent::Clear_Lock()
{
    _isLocked = false;
    _lockedTarget.reset();
}

void TargetComponent::LockOn_NearestTarget(bool currentTarget)
{
    Shared<Character> nearestTarget = Find_NearestTarget(currentTarget);

    if (nearestTarget == nullptr && currentTarget)
    {
        nearestTarget = Find_NearestTarget(false);
    }

    if (nearestTarget == nullptr)
    {
        Clear_Lock();
        return;
    }

    _lockedTarget = nearestTarget;
    _isLocked = true;
}

Shared<Character> TargetComponent::Find_NearestTarget(bool currentTarget)
{
    if (_candidates.empty())
        return nullptr;

    Shared<GameObject> owner = Get_Owner();
    if (owner == nullptr)
        return nullptr;

    auto ownerTransform = owner->Get_Transform();
    if (ownerTransform == nullptr)
        return nullptr;

    Shared<Character> currentLockedTarget = nullptr;
    if (_lockedTarget.expired() == false)
    {
        currentLockedTarget = _lockedTarget.lock();
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();

    float minDistance = FLT_MAX;
    Shared<Character> nearestTarget = nullptr;

    for (const auto& weakTarget : _candidates)
    {
        if (weakTarget.expired())
            continue;

        Shared<Character> targetPtr = weakTarget.lock();
        if (targetPtr == nullptr)
            continue;

        if (currentTarget && currentLockedTarget && targetPtr == currentLockedTarget)
            continue;

        auto targetTransform = targetPtr->Get_Transform();
        if (targetTransform == nullptr)
            continue;

        const Vec3 targetPos = targetTransform->Get_WorldPosition();
        const float distance = Vec3::Distance(targetPos, ownerPos);

        if (distance < minDistance)
        {
            minDistance = distance;
            nearestTarget = targetPtr;
        }
    }

    return nearestTarget;
}

bool TargetComponent::Is_TargetInCandidates(Shared<GameObject> target)
{
    if (target == nullptr)
        return false;

    for (const auto& weakCandidate : _candidates)
    {
        if (weakCandidate.expired())
            continue;

        if (weakCandidate.lock() == target)
            return true;
    }

    return false;
}

Shared<TargetComponent> TargetComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<TargetComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : TargetComponent");

        return nullptr;
    }

    return instance;
}

Shared<Component> TargetComponent::Clone(void* arg)
{
    auto clone = make_shared<TargetComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : TargetComponent");

        return nullptr;
    }

    return clone;
}

void TargetComponent::Free()
{
    _candidates.clear();

    Component::Free();
}
