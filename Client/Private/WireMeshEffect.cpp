#include "pch.h"
#include "WireMeshEffect.h"
#include "EffectComponent.h"
#include "GameObject_Factory.h"
#include "Model.h"
#include "GameInstance.h"

REGISTER_GAMEOBJECT(WireMeshEffect, Protocol::OBJECT_TYPE_WIRE_MESH_EFFECT)

WireMeshEffect::WireMeshEffect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

WireMeshEffect::WireMeshEffect(const WireMeshEffect& rhs)
    : GameObject(rhs)
    , _trackBoneName(rhs._trackBoneName)
    , _targetPosition(rhs._targetPosition)
    , _spawnWorldPosition(rhs._spawnWorldPosition)
    , _meshOriginalLength(rhs._meshOriginalLength)
    , _thickness(rhs._thickness)
    , _rotationOffset(rhs._rotationOffset)
    , _localOffset(rhs._localOffset)
{
}

HRESULT WireMeshEffect::Initialize_Prototype()
{
    return S_OK;
}

HRESULT WireMeshEffect::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL);

    auto* desc = static_cast<FWireMeshEffectDesc*>(arg);

    CHECK_FAILED(GameObject::Initialize(desc), E_FAIL);

    _ownerObj = desc->ownerObj;
    _trackBoneName = desc->trackBoneName;
    _targetPosition = desc->targetPosition;
    _meshOriginalLength = max(desc->meshOriginalLength, 0.001f);
    _thickness = desc->thickness;
    _rotationOffset = desc->rotationOffset;
    _localOffset = desc->localOffset;

    _spawnWorldPosition = Get_Transform()->Get_WorldPosition();

    CHECK_FAILED(Ready_Components(desc->effectAssetName), E_FAIL);

    return S_OK;
}

void WireMeshEffect::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
        _effectCom->Update(timeDelta);

    Vec3 handWorldPosition = _spawnWorldPosition;
    Try_GetHandWorldPosition(handWorldPosition);

    Vec3 wireDelta = _targetPosition - handWorldPosition;
    const float distance = wireDelta.Length();

    Get_Transform()->Set_WorldPosition(handWorldPosition);

    if (distance <= 0.001f)
    {
        if (_effectCom)
        {
            _effectCom->Set_RuntimeLocalTransform(
                _localOffset,
                _rotationOffset,
                Vec3(_thickness.x, 0.001f, _thickness.z));
        }

        return;
    }

    Vec3 lookDir = wireDelta;
    lookDir.Normalize();

    Get_Transform()->LookAt(handWorldPosition + lookDir);

    const float scaleLength = distance / max(_meshOriginalLength, 0.001f);

    if (_effectCom)
    {
        _effectCom->Set_RuntimeLocalTransform(
            _localOffset,
            _rotationOffset,
            Vec3(_thickness.x, scaleLength, _thickness.z));
    }
}

void WireMeshEffect::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_effectCom)
        _effectCom->Late_Update(timeDelta);
}

void WireMeshEffect::Set_TargetPosition(const Vec3& targetPosition)
{
    _targetPosition = targetPosition;
}

HRESULT WireMeshEffect::Ready_Components(const string& effectAssetName)
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = effectAssetName;
    playDesc.loopOverride = false;

    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

bool WireMeshEffect::Try_GetHandWorldPosition(Vec3& outHandWorldPosition) const
{
    auto owner = _ownerObj.lock();
    if (!owner)
        return false;

    auto ownerTransform = owner->Get_Transform();
    if (!ownerTransform)
        return false;

    auto model = owner->Get_Component<Model>();
    if (!model)
    {
        outHandWorldPosition = ownerTransform->Get_WorldPosition();
        return true;
    }

    const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(_trackBoneName);
    if (!socketMatrix)
    {
        outHandWorldPosition = ownerTransform->Get_WorldPosition();
        return true;
    }

    Matrix boneWorldMatrix = (*socketMatrix) * ownerTransform->Get_WorldMatrix();
    outHandWorldPosition = boneWorldMatrix.Translation();

    return true;
}

Shared<GameObject> WireMeshEffect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<WireMeshEffect>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : WireMeshEffect");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> WireMeshEffect::Clone(void* arg)
{
    auto clone = make_shared<WireMeshEffect>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : WireMeshEffect");
        return nullptr;
    }

    return clone;
}

void WireMeshEffect::Free()
{
    GameObject::Free();
}
