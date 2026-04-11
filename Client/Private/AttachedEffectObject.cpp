#include "pch.h"
#include "AttachedEffectObject.h"
#include "GameObject_Factory.h"
#include "EffectComponent.h"
#include "Model.h"

REGISTER_GAMEOBJECT(AttachedEffectObject, Protocol::OBJECT_TYPE_ATTACHED_EFFECT)

AttachedEffectObject::AttachedEffectObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

AttachedEffectObject::AttachedEffectObject(const AttachedEffectObject& rhs)
    : GameObject(rhs)
    , _effectAssetName(rhs._effectAssetName)
    , _loopOverride(rhs._loopOverride)
{
}

HRESULT AttachedEffectObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT AttachedEffectObject::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    auto* desc = static_cast<FAttachedEffectObjectDesc*>(arg);
    if (!desc)
        return E_FAIL;

    _effectAssetName = desc->effectAssetName;
    _loopOverride = desc->loopOverride;

    if (_effectAssetName.empty())
        return E_FAIL;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = _effectAssetName;
    playDesc.loopOverride = _loopOverride;

    CHECK_NULL(_effectCom, E_FAIL);
    CHECK_FAILED(_effectCom->Play_Effect(playDesc), E_FAIL);

    return S_OK;
}

void AttachedEffectObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
    {
        _effectCom->Update(timeDelta);

        if (_autoDestroyOnFinish && !_effectCom->Is_Playing())
        {
            Set_Destroy(true);
            return;
        }
    }
        
}

void AttachedEffectObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    // 뼈대 추적
    if (_isTrackingBone)
    {
        auto model = _targetModel;
        auto transform = _targetTransform.lock();

        if (model && transform)
        {
            const Matrix* boneMatrix = model->Get_SocketBoneMatrixPtr(_targetBoneName);
            if (boneMatrix)
            {
                Matrix boneWorldMatrix = (*boneMatrix) * transform->Get_WorldMatrix();

                Sync_AttachedTransform(boneWorldMatrix, _targetLocalOffset, _targetLocalRotation, _targetLocalScale);
            }
        }
        else
        {
            // 타겟이 파괴되면, 이펙트도 파괴
            Set_Destroy(true);
        }
    }


    if (_effectCom)
        _effectCom->Late_Update(timeDelta);
}

void AttachedEffectObject::Sync_AttachedTransform(
    const Matrix& boneWorldMatrix,
    const Vec3& localOffset,
    const Vec3& localRotation,
    const Vec3& localScale)
{
    CHECK_NULL(_transformCom);

    Matrix socketWorld = boneWorldMatrix;

    Vec3 socketScale;
    Vec3 socketPosition;
    Quat socketRotation;

    if (!socketWorld.Decompose(socketScale, socketRotation, socketPosition))
        return;

    const Matrix socketRotationMatrix = Matrix::CreateFromQuaternion(socketRotation);
    const Vec3 rotatedOffset = Vec3::TransformNormal(localOffset, socketRotationMatrix);

    const Quat localRotationQuat = Quat::CreateFromYawPitchRoll(
        XMConvertToRadians(localRotation.y),
        XMConvertToRadians(localRotation.x),
        XMConvertToRadians(localRotation.z));

    const Quat finalRotation = socketRotation * localRotationQuat;

    const Vec3 finalScale = localScale;

    _transformCom->Set_WorldPosition(socketPosition + rotatedOffset);
    _transformCom->Set_WorldRotation(finalRotation);
    _transformCom->Set_LocalScale(finalScale);
}

void AttachedEffectObject::Stop_AttachedEffect()
{
    if (_effectCom)
        _effectCom->Stop_Effect();

    Set_Destroy(true);
}

void AttachedEffectObject::Attach_To_Bone(Model* targetModel, Weak<Transform> targetTransform,
    const string& boneName, const Vec3& localOffset, const Vec3& localRotation, const Vec3& localScale)
{
    _targetModel = targetModel;
    _targetTransform = targetTransform;
    _targetBoneName = boneName;
    _targetLocalOffset = localOffset;
    _targetLocalRotation = localRotation;
    _targetLocalScale = localScale;

    _isTrackingBone = true;
}

HRESULT AttachedEffectObject::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);

    return S_OK;
}

Shared<GameObject> AttachedEffectObject::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<AttachedEffectObject>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : AttachedEffectObject");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> AttachedEffectObject::Clone(void* arg)
{
    auto clone = make_shared<AttachedEffectObject>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : AttachedEffectObject");
        return nullptr;
    }

    return clone;
}

void AttachedEffectObject::Free()
{
    GameObject::Free();
}
