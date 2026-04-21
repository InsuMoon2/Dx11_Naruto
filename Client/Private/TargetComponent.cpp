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

    if (INPUT->KeyDown(KEY_TYPE::V))
    {
        // 이미 타겟팅을 하고있으면
        if (_isLocked)
        {
            _isLocked = false;
            _lockedTarget.reset();
        }
        else // 없으면, 탐색
        {
            LockOn_NearestTarget();
        }
    }

    // 거리에서 벗어나거나, 타겟팅 대상이 죽으면 Lock 해제
    if (_isLocked)
    {
        if (_lockedTarget.expired())
        {
            _isLocked = false;
        }
        else
        {
            // 리스트에서 타겟이 벗어났는지
            Shared<GameObject> lockPtr = _lockedTarget.lock();
            bool isStillCandiates = false;

            for (const auto& weakCandiate : _candidates)
            {
                if (!weakCandiate.expired() && weakCandiate.lock() == lockPtr)
                {
                    isStillCandiates = true;
                    break;
                }
            }

            if (!isStillCandiates)
            {
                _isLocked = false;
                _lockedTarget.reset();
            }
        }
    }

}

void TargetComponent::LockOn_NearestTarget()
{
    if (_candidates.empty())
        return;

    Shared<GameObject> owner = Get_Owner();
    if (owner == nullptr) return;

    auto ownerTransform = owner->Get_Transform();
    CHECK_NULL(ownerTransform);

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();

    float minDistance = FLT_MAX;
    Shared<Character> nearestTarget {};

    for (const auto& weakTarget : _candidates)
    {
        if (weakTarget.expired())
            continue;

        Shared<Character> targetPtr = weakTarget.lock();
        auto targetTransform = targetPtr->Get_Transform();
        CHECK_NULL(targetTransform);

        Vec3 targetPos = targetTransform->Get_WorldPosition();

        float distance = Vec3::Distance(targetPos, ownerPos);

        if (distance < minDistance)
        {
            minDistance = distance;
            nearestTarget = targetPtr;
        }
    }

    if (nearestTarget != nullptr)
    {
        _lockedTarget = nearestTarget;
        _isLocked = true;
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
