#include "pch.h"
#include "StretchingMeshEffect.h"
#include "EffectComponent.h"
#include "GameObject_Factory.h"
#include "Model.h"
#include "Debug_Manager.h"
#include "GameInstance.h"

NS_BEGIN(Client)

REGISTER_GAMEOBJECT(StretchingMeshEffect, Protocol::OBJECT_TYPE_STRETCHING_MESH_EFFECT);

StretchingMeshEffect::StretchingMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

StretchingMeshEffect::StretchingMeshEffect(const StretchingMeshEffect& rhs)
    : GameObject(rhs)
    , _meshOriginalLength(rhs._meshOriginalLength)
    , _thickness(rhs._thickness)
{
}

HRESULT StretchingMeshEffect::Initialize_Prototype()
{
    return S_OK;
}

HRESULT StretchingMeshEffect::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL);
    FStretchingMeshDesc* desc = static_cast<FStretchingMeshDesc*>(arg);

    CHECK_FAILED(GameObject::Initialize(desc), E_FAIL);

    _meshOriginalLength = max(desc->meshOriginalLength, 0.001f);
    _thickness = desc->thickness;
    _rotationOffset = desc->rotationOffset;
    _localOffset = desc->localOffset;

    _ownerObj = desc->ownerObj;
    _trackBoneName = desc->trackBoneName;

    _spawnWorldPos = Get_Transform()->Get_WorldPosition();
    _currentTargetPos = _spawnWorldPos;

    // 플레이어 위치 기준으로 세팅 해보기
    if (auto owner = _ownerObj.lock())
        _ownerSpawnWorldPos = owner->Get_Transform()->Get_WorldPosition();
    else
        _ownerSpawnWorldPos = _spawnWorldPos;

    CHECK_FAILED(Ready_Components(desc->effectAssetName), E_FAIL);

    return S_OK;
}

void StretchingMeshEffect::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
        _effectCom->Update(timeDelta);

    Vec3 handWorldPos = _spawnWorldPos;

    Vec3 ownerWorldPos = _ownerSpawnWorldPos;
    Quat ownerWorldRot = Quat::Identity;

    if (auto owner = _ownerObj.lock())
    {
        ownerWorldPos = owner->Get_Transform()->Get_WorldPosition();
        ownerWorldRot = owner->Get_Transform()->Get_WorldRotation();

        auto model = owner->Get_Component<Model>();
        if (model)
        {
            const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(_trackBoneName);
            if (socketMatrix)
            {
                Matrix boneWorldMatrix = (*socketMatrix) * owner->Get_Transform()->Get_WorldMatrix();
                handWorldPos = boneWorldMatrix.Translation();
            }
        }
    }

    Vec3 moveDelta = ownerWorldPos - _ownerSpawnWorldPos;


    const float distance = moveDelta.Length();

    Get_Transform()->Set_WorldPosition(handWorldPos);

    const Vec3 childLocalRotation = _rotationOffset;

    if (distance <= 0.001f)
    {
        Get_Transform()->Set_WorldRotation(ownerWorldRot);

        if (_effectCom)
        {
            _effectCom->Set_RuntimeLocalTransform(
                _localOffset,
                _rotationOffset,
                Vec3(_thickness.x, 0.001f, _thickness.z));
        }

        return;
    }

    Vec3 lookDir = -moveDelta;
    lookDir.Normalize();

    const Vec3 lookTarget = handWorldPos + lookDir;

    Get_Transform()->LookAt(lookTarget);

    const float safeOriginalLength = max(_meshOriginalLength, 0.001f);
    const float scaleLength = distance / safeOriginalLength;

    if (_effectCom)
    {
        _effectCom->Set_RuntimeLocalTransform(
            _localOffset,
            _rotationOffset,
            Vec3(_thickness.x, scaleLength, _thickness.z));
    }
}



void StretchingMeshEffect::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_effectCom)
        _effectCom->Late_Update(timeDelta);
}

HRESULT StretchingMeshEffect::Ready_Components(const string& effectAssetName)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = effectAssetName;
    playDesc.loopOverride = false;

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

Shared<GameObject> StretchingMeshEffect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<StretchingMeshEffect>(device, context);
    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create StretchingMeshEffect");
        return nullptr;
    }
    return instance;
}

Shared<GameObject> StretchingMeshEffect::Clone(void* arg)
{
    auto clone = make_shared<StretchingMeshEffect>(*this);
    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone StretchingMeshEffect");
        return nullptr;
    }
    return clone;
}

void StretchingMeshEffect::Free()
{
    GameObject::Free();
}

NS_END
