#include "pch.h"
#include "StretchingMeshEffect.h"
#include "EffectComponent.h"
#include "GameObject_Factory.h"
#include "Model.h"

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

    _meshOriginalLength = desc->meshOriginalLength;
    _thickness = desc->thickness;

    _ownerObj = desc->ownerObj;
    _trackBoneName = desc->trackBoneName;
    _originalWorldPos = Get_Transform()->Get_WorldPosition();

    //_currentTargetPos = _originalWorldPos;

    CHECK_FAILED(Ready_Components(desc->effectAssetName), E_FAIL);

    return S_OK;
}

void StretchingMeshEffect::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
        _effectCom->Update(timeDelta);

    Vec3 currentTargetPos = _originalWorldPos;

    if (auto owner = _ownerObj.lock())
    {
        auto model = owner->Get_Component<Model>();
        if (model)
        {
            const Matrix* socketMatrix = model->Get_SocketBoneMatrixPtr(_trackBoneName);
            if (socketMatrix)
            {
                Matrix boneWorldMatrix = (*socketMatrix) * owner->Get_Transform()->Get_WorldMatrix();
                currentTargetPos = boneWorldMatrix.Translation();
            }
        }
    }

    Vec3 start = _originalWorldPos; 
    Vec3 end   = currentTargetPos;
    Vec3 dir = end - start;
    float distance = dir.Length();

    if (distance > 0.001f)
    {
        Get_Transform()->LookAt(end);

        float scaleZ = distance / _meshOriginalLength;
        Get_Transform()->Set_LocalScale(_thickness.x, _thickness.y, scaleZ);
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
    auto pInstance = make_shared<StretchingMeshEffect>(device, context);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create StretchingMeshEffect");
        return nullptr;
    }
    return pInstance;
}

Shared<GameObject> StretchingMeshEffect::Clone(void* arg)
{
    auto pClone = make_shared<StretchingMeshEffect>(*this);
    if (FAILED(pClone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone StretchingMeshEffect");
        return nullptr;
    }
    return pClone;
}

void StretchingMeshEffect::Free()
{
    GameObject::Free();
}

NS_END
