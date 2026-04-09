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
        if (!layer.desc.base.enabled)
            continue;

        if (layer.finished)
            continue;

        if (!layer.obj)
        {
            layer.finished = true;
            continue;
        }

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
    if (!_isPlaying)
        return;

    for (auto& layer : _layers)
    {
        if (!layer.desc.base.enabled)
            continue;

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
    _assetName = desc.effectAssetName;

    string path = Resolve_EffectAssetPathByName(_assetName);
    if (path.empty())
    {
        LOG_ERROR("EffectComponent::Play_Effect failed to resolve effect asset. name='{}'", _assetName);
        return E_FAIL;
    }

    FEffectAssetDesc loadedAsset{};
    if (FAILED(EffectAsset_Serializer::Load_EffectAsset(path, loadedAsset)))
    {
        LOG_ERROR("EffectComponent::Play_Effect failed to load effect asset. path='{}'", path);
        return E_FAIL;
    }

    return Play_EffectAsset(loadedAsset);
}

HRESULT EffectComponent::Play_EffectAsset(const FEffectAssetDesc& assetDesc)
{
    Stop_Effect();

    _asset = assetDesc;
    _layers.clear();
    _layers.resize(_asset.layers.size());

    for (size_t i = 0; i < _asset.layers.size(); ++i)
    {
        _layers[i].desc = _asset.layers[i];
        _layers[i].elapsed = 0.f;
        _layers[i].started = false;
        _layers[i].finished = false;

        if (_layers[i].desc.base.enabled)
        {
            Create_LayerObject(_layers[i]);
        }
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

// 에디터 프리뷰 토글이 바뀌었을 때 이미 생성된 메시 레이어에도 즉시 같은 가시화 상태를 반영한다.
void EffectComponent::Set_ForceVisiblePreview(bool enabled)
{
    _forceVisiblePreview = enabled;

    for (auto& layer : _layers)
    {
        if (layer.desc.base.kind != EEffectLayerKind::Mesh || !layer.obj)
            continue;

        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
            meshObj->Set_ForceVisiblePreview(_forceVisiblePreview);
    }
}

bool EffectComponent::Apply_LayerDesc(int32 layerIndex, const FEffectLayerDesc& layerDesc, bool rebuildObject)
{
    if (layerIndex < 0 || layerIndex >= static_cast<int32>(_layers.size()))
        return false;

    if (layerIndex < static_cast<int32>(_asset.layers.size()))
        _asset.layers[layerIndex] = layerDesc;

    auto& layer = _layers[layerIndex];
    const bool kindChanged = (layer.desc.base.kind != layerDesc.base.kind);

    layer.desc = layerDesc;

    if (!layer.desc.base.enabled)
    {
        if (layer.obj)
        {
            layer.obj->Set_Destroy(true);
            layer.obj.reset();
        }

        layer.started = false;
        layer.finished = false;
        layer.elapsed = 0.f;
        return true;
    }

    if (rebuildObject || kindChanged || !layer.obj)
    {
        if (layer.obj)
        {
            layer.obj->Set_Destroy(true);
            layer.obj.reset();
        }

        layer.started = false;
        layer.finished = false;
        layer.elapsed = 0.f;

        return SUCCEEDED(Create_LayerObject(layer));
    }

    if (layer.desc.base.kind == EEffectLayerKind::Mesh)
    {
        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
        {
            meshObj->Apply_LayerDesc(layer.desc);
            meshObj->Set_ForceVisiblePreview(_forceVisiblePreview);
        }
    }

    Apply_LayerTransformInternal(layer);

    return true;
}

bool EffectComponent::Apply_LayerTransform(int32 layerIndex, const FEffectLayerBase& baseDesc)
{
    if (layerIndex < 0 || layerIndex >= static_cast<int32>(_layers.size()))
        return false;

    if (layerIndex < static_cast<int32>(_asset.layers.size()))
        _asset.layers[layerIndex].base = baseDesc;

    auto& layer = _layers[layerIndex];
    layer.desc.base = baseDesc;

    Apply_LayerTransformInternal(layer);

    return true;
}

HRESULT EffectComponent::Create_LayerObject(FActiveLayer& layer)
{
    if (!layer.desc.base.enabled)
        return S_FALSE; // 스킵

    if (layer.desc.base.kind == EEffectLayerKind::Mesh)
    {
        EffectMeshObject::FEffectMeshDesc meshDesc{};
        meshDesc.name = Utils::ToWString(layer.desc.base.layerName);
        meshDesc.layerDesc = layer.desc;

        layer.obj = GAME->Clone_GameObject(
            0,
            Protocol::OBJECT_TYPE_EFFECT_MESH,
            &meshDesc);

        if (!layer.obj)
            return E_FAIL;

        Apply_LayerTransformInternal(layer);

        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
            meshObj->Set_ForceVisiblePreview(_forceVisiblePreview);

        return S_OK;
    }

    // Point는 이번 단계에서 아직 미구현
    layer.obj.reset();
    layer.finished = true;

    return S_FALSE;
}

void EffectComponent::Apply_LayerTransformInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    auto owner = Get_Owner();
    auto childTransform = layer.obj->Get_Transform();

    if (!owner || !owner->Get_Transform() || !childTransform)
        return;

    childTransform->Set_Parent(owner->Get_Transform());
    childTransform->Set_LocalPosition(layer.desc.base.localPosition);
    childTransform->Set_LocalEulerAngles(
        layer.desc.base.localRotation.x,
        layer.desc.base.localRotation.y,
        layer.desc.base.localRotation.z);

    childTransform->Set_LocalScale(layer.desc.base.localScale);
}

string EffectComponent::Resolve_EffectAssetPathByName(const string& effectAssetName)
{
    if (effectAssetName.empty())
        return "";

    fs::path effectFolder = EffectAsset_Serializer::Get_EffectFolderPath();

    if(effectFolder.empty() || !fs::exists(effectFolder))
        return "";

    fs::path directPath = effectFolder / effectAssetName;
    if (fs::exists(directPath) && fs::is_regular_file(directPath))
        return directPath.string();

    fs::path effectJsonPath = effectFolder / (effectAssetName + ".effect.json");
    if (fs::exists(effectJsonPath) && fs::is_regular_file(effectJsonPath))
        return effectJsonPath.string();

    for (const auto& entry : fs::directory_iterator(effectFolder))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path path = entry.path();
        const string fileName = path.filename().string();
        const string stemName = path.stem().stem().string();

        if (Utils::ToLowerCopy(fileName) == Utils::ToLowerCopy(effectAssetName))
            return path.string();

        if (Utils::ToLowerCopy(stemName) == Utils::ToLowerCopy(effectAssetName))
            return path.string();
    }

    return "";

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
