#include "pch.h"
#include "EffectComponent.h"

#include "EffectAsset_Serializer.h"

EffectComponent::EffectComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

EffectComponent::EffectComponent(const EffectComponent& rhs)
    : Component(rhs)
{
}

HRESULT EffectComponent::Initialize_Prototype()
{
    return Component::Initialize_Prototype();
}

HRESULT EffectComponent::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

void EffectComponent::Update(float timeDelta)
{
    if (!_isPlaying)
        return;

    _lifeSpan += timeDelta;

    // 이번 프레임에 모든 레이어가 종료됐는지
    bool allDone = true;

    for (auto& layer : _layers)
    {
        if (layer.finished)
            continue;

        // 레이어가 살아있음
        allDone = false;

        if (_lifeSpan < layer.desc.base.startDelay)
            continue;

        if (!layer.started)
        {
            layer.started = true;
            layer.elapsed = 0.f;
        }

        layer.elapsed += timeDelta;

        // Duration
        if (layer.desc.base.duration > 0.f && layer.elapsed >= layer.desc.base.duration)
        {
            if (layer.desc.base.loop || _desc.loopOverride)
            {
                layer.elapsed = 0.f;
            }
            else
            {
                layer.finished = true;
                continue;
            }
        }

        if (layer.obj)
        {
            layer.obj->Update(timeDelta);

            if (layer.desc.base.kind == EEffectLayerKind::Mesh)
            {
                auto meshObj = static_pointer_cast<EffectMeshObject>(layer.obj);
                if (meshObj)
                {
                    meshObj->Set_ElapsedTime(layer.elapsed);
                }
            }
        }
    }

    if (_asset.totalDuration > 0.f)
    {
        if (_lifeSpan >= _asset.totalDuration)
        {
            Stop_Effect();
            return;
        }
    }
    else if (allDone)
    {
        Stop_Effect();
        return;
    }

}

void EffectComponent::Late_Update(float timeDelta)
{
    if (!_isPlaying) return;
    for (auto& layer : _layers)
    {
        if (layer.started && !layer.finished && layer.obj)
        {
            layer.obj->Late_Update(timeDelta);
        }
    }
}

HRESULT EffectComponent::Play_Effect(const FPlayDesc& desc)
{
    // 재생하고있는거 멈추고
    Stop_Effect();

    _desc = desc;
    _assetGuid = desc.effectAssetGuid;

    string path = Utils::ToString(GAME->Resolve_AssetPath(_assetGuid));
    if (path.empty())
        return E_FAIL;

    FEffectAssetDesc loadedAsset;

    if (FAILED(EffectAsset_Serializer::Load_EffectAsset(path, loadedAsset)))
        return E_FAIL;

    return Play_EffectAsset(loadedAsset);
}

HRESULT EffectComponent::Play_EffectAsset(const FEffectAssetDesc& assetDesc)
{
    Stop_Effect();

    _asset = assetDesc;

    for (const auto& layerDesc : _asset.layers)
    {
        if (!layerDesc.base.enabled)
            continue;

        FActiveLayer newLayer{};
        newLayer.desc = layerDesc;

        if (layerDesc.base.kind == EEffectLayerKind::Mesh)
        {
            EffectMeshObject::FEffectMeshDesc meshDesc{};
            meshDesc.name = Utils::ToWString(layerDesc.base.layerName);
            meshDesc.layerDesc = layerDesc.mesh;                    
            meshDesc.position = Vec3::Zero;                         
            meshDesc.scale = layerDesc.base.localScale;

            newLayer.obj = GAME->Clone_GameObject(
                0,
                Protocol::OBJECT_TYPE_EFFECT_MESH,
                &meshDesc);

            if (newLayer.obj)
            {
                auto childTransform = newLayer.obj->Get_Transform();
                auto owner = Get_Owner();

                if (childTransform && owner && owner->Get_Transform())
                {
                    childTransform->Set_Parent(owner->Get_Transform());
                    childTransform->Set_LocalPosition(layerDesc.base.localPosition);

                    childTransform->Set_LocalEulerAngles(
                        layerDesc.base.localRotation.x,
                        layerDesc.base.localRotation.y,
                        layerDesc.base.localRotation.z);

                    childTransform->Set_LocalScale(layerDesc.base.localScale);
                }
            }
        }
        else if (layerDesc.base.kind == EEffectLayerKind::Point)
        {
            /* [추가] Point 레이어는 1차에서는 아직 미구현 상태를 명시한다.
               이후 Particle_Point 연결이 들어오면 이 분기를 채운다. */
        }

        _layers.push_back(newLayer);
    }

    _lifeSpan = 0.f;
    _isPlaying = true;

    return S_OK;
}


void EffectComponent::Stop_Effect()
{
    for (auto& layer : _layers)
    {
        if (layer.obj)
            layer.obj->Set_Destroy(true);
    }

    _layers.clear();

    _isPlaying = false;
}

Shared<EffectComponent> EffectComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<EffectComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : EffectComponent");
        return nullptr;
    }

    return instance;
}

Shared<Component> EffectComponent::Clone(void* arg)
{
    auto clone = make_shared<EffectComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : EffectComponent");
        return nullptr;
    }

    return clone;
}

void EffectComponent::Free()
{
    Component::Free();
}
