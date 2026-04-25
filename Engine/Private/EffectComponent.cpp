#include "pch.h"
#include "EffectComponent.h"
#include "EffectAsset_Serializer.h"
#include "EffectBilldboardObject.h"
#include "Particle_Point.h"

/* Vec4 수명 보간이 필요할 때 SimpleMath 의존 없이 각 채널을 선형 보간하기 위한 헬퍼다. */
static Vec4 Lerp_EffectVec4(const Vec4& start, const Vec4& end, float t)
{
    return Vec4(
        start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t,
        start.z + (end.z - start.z) * t,
        start.w + (end.w - start.w) * t);
}

/* float 계열 머티리얼 값을 레이어 진행률 기준으로 보간할 때 호출한다. */
static float Lerp_EffectFloat(float start, float end, float t)
{
    return start + (end - start) * t;
}

/* Vec3 위치/스케일 값을 레이어 진행 시간에 맞춰 선형 보간할 때 호출한다. */
static Vec3 Lerp_EffectVec3(const Vec3& start, const Vec3& end, float t)
{
    return Vec3(
        start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t,
        start.z + (end.z - start.z) * t);
}

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

        Apply_LayerPositionInternal(layer);
        Apply_LayerScaleInternal(layer);
        Apply_LayerAnimatedMaterialInternal(layer);

        if (layer.obj)
        {
            layer.obj->Update(timeDelta);

            if (layer.desc.base.kind == EEffectLayerKind::Mesh ||
                layer.desc.base.kind == EEffectLayerKind::SkeletalMesh)
            {
                auto meshObj = static_pointer_cast<EffectMeshObject>(layer.obj);
                if (meshObj)
                {
                    meshObj->Set_ElapsedTime(layer.elapsed);
                }
            }
            else if (layer.desc.base.kind == EEffectLayerKind::BillboardRect)
            {
                auto billboardObj = static_pointer_cast<EffectBillboardObject>(layer.obj);
                if (billboardObj)
                {
                    billboardObj->Set_ElapsedTime(layer.elapsed);
                }
            }
        }
    }

    if (_asset.totalDuration > 0.f)
    {
        if (_lifeSpan >= _asset.totalDuration)
        {
            if (_desc.loopOverride)
            {
                _lifeSpan = 0.f;

                for (auto& layer : _layers)
                {
                    layer.elapsed = 0.f;
                    layer.started = false;
                    layer.finished = false;
                }

                return;
            }

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

    for (int32 i = 0; i < static_cast<int32>(_layers.size()); ++i)
    {
        auto& layer = _layers[i];

        if (!layer.desc.base.enabled)
            continue;

        if (!Is_LayerVisibleInPreview(i))
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
    if (_previewSoloLayerIndex >= static_cast<int32>(_layers.size()))
        _previewSoloLayerIndex = -1;

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

void EffectComponent::Set_ForceVisiblePreview(bool enabled)
{
    _forceVisiblePreview = enabled;

    for (auto& layer : _layers)
    {
        if ((layer.desc.base.kind != EEffectLayerKind::Mesh &&
            layer.desc.base.kind != EEffectLayerKind::SkeletalMesh) || !layer.obj)
            continue;

        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
            meshObj->Set_ForceVisiblePreview(_forceVisiblePreview);
    }
}

void EffectComponent::Set_PreviewSoloLayerIndex(int32 layerIndex)
{
    if (layerIndex < 0)
    {
        _previewSoloLayerIndex = -1;
        return;
    }

    if (!_layers.empty() && layerIndex >= static_cast<int32>(_layers.size()))
    {
        _previewSoloLayerIndex = -1;
        return;
    }

    _previewSoloLayerIndex = layerIndex;
}

void EffectComponent::Set_RuntimeLocalTransform(const Vec3& localPosition, const Vec3& localRotation,
    const Vec3& localScale)
{
    _useRuntimeLocalTransform = true;
    _runtimeLocalPosition = localPosition;
    _runtimeLocalRotation = localRotation;
    _runtimeLocalScale = localScale;

    for (auto& layer : _layers)
    {
        if (!layer.obj)
            continue;

        Apply_LayerTransformInternal(layer);
    }

}

void EffectComponent::Clear_RuntimeLocalTransform()
{
    _useRuntimeLocalTransform = false;
    _runtimeLocalPosition = Vec3::Zero;
    _runtimeLocalRotation = Vec3::Zero;
    _runtimeLocalScale = Vec3(1.f, 1.f, 1.f);

    for (auto& layer : _layers)
    {
        if (!layer.obj)
            continue;

        Apply_LayerTransformInternal(layer);
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

    if (layer.desc.base.kind == EEffectLayerKind::Mesh ||
        layer.desc.base.kind == EEffectLayerKind::SkeletalMesh)
    {
        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
        {
            meshObj->Apply_LayerDesc(layer.desc);
            meshObj->Set_ForceVisiblePreview(_forceVisiblePreview);
        }
    }
    else if (layer.desc.base.kind == EEffectLayerKind::BillboardRect)
    {
        auto billboardObj = dynamic_pointer_cast<EffectBillboardObject>(layer.obj);
        if (billboardObj)
            billboardObj->Apply_LayerDesc(layer.desc);
    }
    else if (layer.desc.base.kind == EEffectLayerKind::Point)
    {
        auto pointObj = dynamic_pointer_cast<Particle_Point>(layer.obj);
        if (pointObj)
        {
            pointObj->Set_ColorTint(layer.desc.point.colorTint);
            pointObj->Set_Opacity(layer.desc.point.opacity);
        }
    }

    Apply_LayerTransformInternal(layer);
    Apply_LayerAnimatedMaterialInternal(layer);

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

bool EffectComponent::Is_LayerVisibleInPreview(int32 layerIndex) const
{
    return _previewSoloLayerIndex < 0 || _previewSoloLayerIndex == layerIndex;
}

HRESULT EffectComponent::Create_LayerObject(FActiveLayer& layer)
{
    if (!layer.desc.base.enabled)
        return S_FALSE; // 스킵

    if (layer.desc.base.kind == EEffectLayerKind::Mesh ||
        layer.desc.base.kind == EEffectLayerKind::SkeletalMesh)
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

        Apply_LayerAnimatedMaterialInternal(layer);
        return S_OK;
    }

    if (layer.desc.base.kind == EEffectLayerKind::Point)
    {
        Particle_Point::FParticlePointDesc pointDesc{};
        pointDesc.name = Utils::ToWString(layer.desc.base.layerName);
        pointDesc.textureGuid = layer.desc.point.textureGuid;
        pointDesc.maskTextureGuid = layer.desc.point.maskTextureGuid;
        pointDesc.opacityTextureGuid = layer.desc.point.opacityTextureGuid;
        pointDesc.blendMode = layer.desc.point.blendMode;
        pointDesc.colorTint = layer.desc.point.colorTint;
        pointDesc.opacity = layer.desc.point.opacity;
        pointDesc.emissiveStrength = layer.desc.point.emissiveStrength;
        pointDesc.flipbook = layer.desc.point.flipbook;
        pointDesc.customParams0 = layer.desc.point.customParams0;
        pointDesc.customParams1 = layer.desc.point.customParams1;
        pointDesc.addToBlendGroup = (layer.desc.point.blendMode != EEffectBlendMode::Opaque);

        pointDesc.bufferDesc.numInstances = layer.desc.point.numInstances;
        pointDesc.bufferDesc.center = layer.desc.point.center;
        pointDesc.bufferDesc.range = layer.desc.point.range;
        pointDesc.bufferDesc.spawnShape = layer.desc.point.spawnShape;
        pointDesc.bufferDesc.spawnRadius = layer.desc.point.spawnRadius;
        pointDesc.bufferDesc.spawnInnerRadius = layer.desc.point.spawnInnerRadius;
        pointDesc.bufferDesc.spawnHeight = layer.desc.point.spawnHeight;
        pointDesc.bufferDesc.scale = layer.desc.point.scale;
        pointDesc.bufferDesc.speed = layer.desc.point.speed;
        pointDesc.bufferDesc.lifeTime = layer.desc.point.lifeTime;
        pointDesc.bufferDesc.pivot = layer.desc.point.pivot;
        pointDesc.bufferDesc.isLoop = layer.desc.point.isLoop;
        pointDesc.bufferDesc.moveMode = static_cast<VIBuffer_Particle_Point::EMoveMode>(layer.desc.point.moveMode);

        layer.obj = GAME->Clone_GameObject(
            0,
            Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
            &pointDesc);

        if (!layer.obj)
            return E_FAIL;

        Apply_LayerTransformInternal(layer);
        auto billboardObj = dynamic_pointer_cast<EffectBillboardObject>(layer.obj);
        if (billboardObj)
            billboardObj->Set_ElapsedTime(layer.elapsed);
        Apply_LayerAnimatedMaterialInternal(layer);

        // [수정] Point 레이어 생성 블록 안으로 월드 고정 로직 이동
        if (layer.desc.point.lockWorldOnSpawn)
        {
            layer.worldLocked = true;
            layer.lockedWorldPos = layer.obj->Get_Transform()->Get_WorldPosition();

            layer.obj->Get_Transform()->Set_Parent(nullptr);
            layer.obj->Get_Transform()->Set_LocalPosition(layer.lockedWorldPos);
        }

        return S_OK;
    }

    if (layer.desc.base.kind == EEffectLayerKind::BillboardRect)
    {
        EffectBillboardObject::FEffectBillboardDesc billboardDesc{};
        billboardDesc.name = Utils::ToWString(layer.desc.base.layerName);
        billboardDesc.layerDesc = layer.desc;

        layer.obj = GAME->Clone_GameObject(
            0,
            Protocol::OBJECT_TYPE_EFFECT_BILLBOARD,
            &billboardDesc);

        if (!layer.obj)
            return E_FAIL;

        Apply_LayerTransformInternal(layer);
        Apply_LayerAnimatedMaterialInternal(layer);
        return S_OK;
    }

    layer.obj.reset();
    layer.finished = true;

    return S_FALSE;
}

void EffectComponent::Apply_LayerTransformInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    if (layer.worldLocked)
    {
        layer.obj->Get_Transform()->Set_LocalPosition(layer.lockedWorldPos);
        return;
    }

    auto owner = Get_Owner();
    auto childTransform = layer.obj->Get_Transform();

    if (!owner || !owner->Get_Transform() || !childTransform)
        return;

    childTransform->Set_Parent(owner->Get_Transform());

    Vec3 runtimePosition = Vec3::Zero;
    Vec3 runtimeRotation = Vec3::Zero;
    Vec3 runtimeScale = Vec3::One;

    if (_useRuntimeLocalTransform)
    {
        runtimePosition = _runtimeLocalPosition;
        runtimeRotation = _runtimeLocalRotation;
        runtimeScale = _runtimeLocalScale;
    }

    const Vec3 layerPosition = Resolve_LayerPosition(layer);
    const Vec3 finalLocalPosition =
        runtimePosition +
        _desc.localPosition +
        layerPosition;

    const Vec3 finalLocalRotation =
        runtimeRotation +
        _desc.localRotation +
        layer.desc.base.localRotation;

    Vec3 layerScale = Resolve_LayerScale(layer);
    Vec3 finalLocalScale(
        layerScale.x * _desc.localScale.x * runtimeScale.x,
        layerScale.y * _desc.localScale.y * runtimeScale.y,
        layerScale.z * _desc.localScale.z * runtimeScale.z);

    childTransform->Set_LocalPosition(finalLocalPosition);
    childTransform->Set_LocalEulerAngles(
        finalLocalRotation.x,
        finalLocalRotation.y,
        finalLocalRotation.z);
    childTransform->Set_LocalScale(finalLocalScale);

    // Mesh 레이어는 런타임/오너 오프셋까지 합쳐진 현재 자세를 회전 기준으로 다시 잡아야
    // Local Space / Owner Space 회전이 에디터 값과 동일한 기준으로 유지된다.
    if (layer.desc.base.kind == EEffectLayerKind::Mesh ||
        layer.desc.base.kind == EEffectLayerKind::SkeletalMesh)
    {
        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (meshObj)
            meshObj->Sync_RotationBaseFromCurrentTransform();
    }
}

void EffectComponent::Apply_LayerPositionInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    if (layer.worldLocked)
        return;

    auto childTransform = layer.obj->Get_Transform();
    if (!childTransform)
        return;

    Vec3 runtimePosition = Vec3::Zero;

    if (_useRuntimeLocalTransform)
        runtimePosition = _runtimeLocalPosition;

    const Vec3 layerPosition = Resolve_LayerPosition(layer);
    const Vec3 finalLocalPosition =
        runtimePosition +
        _desc.localPosition +
        layerPosition;

    childTransform->Set_LocalPosition(finalLocalPosition);
}

void EffectComponent::Apply_LayerScaleInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    auto childTransform = layer.obj->Get_Transform();
    if (!childTransform)
        return;

    Vec3 runtimeScale = Vec3(1.f, 1.f, 1.f);

    if (_useRuntimeLocalTransform)
        runtimeScale = _runtimeLocalScale;

    Vec3 layerScale = Resolve_LayerScale(layer);

    Vec3 finalLocalScale(
        layerScale.x * _desc.localScale.x * runtimeScale.x,
        layerScale.y * _desc.localScale.y * runtimeScale.y,
        layerScale.z * _desc.localScale.z * runtimeScale.z);

    childTransform->Set_LocalScale(finalLocalScale);
}

Vec3 EffectComponent::Resolve_LayerScale(const FActiveLayer& layer) const
{
    if (!layer.desc.base.useScaleOverTime)
        return layer.desc.base.localScale;

    const float duration = layer.desc.base.scaleDuration;

    // 끝나면, endScale로 유지
    if (duration <= 0.f)
        return layer.desc.base.endScale;

    const float t = std::clamp(layer.elapsed / duration, 0.f, 1.f);
    return Vec3::Lerp(layer.desc.base.localScale, layer.desc.base.endScale, t);
}

Vec3 EffectComponent::Resolve_LayerPosition(const FActiveLayer& layer) const
{
    if (!layer.desc.base.usePositionOverTime)
        return layer.desc.base.localPosition;

    const float duration = layer.desc.base.positionDuration;

    if (duration <= 0.f)
        return layer.desc.base.endPosition;

    const float t = std::clamp(layer.elapsed / duration, 0.f, 1.f);
    return Lerp_EffectVec3(layer.desc.base.localPosition, layer.desc.base.endPosition, t);
}

float EffectComponent::Resolve_LayerDuration(const FActiveLayer& layer) const
{
    if (layer.desc.base.duration > 0.f)
        return layer.desc.base.duration;

    if (_asset.totalDuration > 0.f)
        return (std::max)(0.f, _asset.totalDuration - layer.desc.base.startDelay);

    return 0.f;
}

float EffectComponent::Resolve_LayerProgress(const FActiveLayer& layer) const
{
    const float duration = Resolve_LayerDuration(layer);
    if (duration <= 0.f)
        return 0.f;

    return std::clamp(layer.elapsed / duration, 0.f, 1.f);
}

void EffectComponent::Apply_LayerAnimatedMaterialInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    const float layerProgress = Resolve_LayerProgress(layer);

    if (layer.desc.base.kind == EEffectLayerKind::Mesh ||
        layer.desc.base.kind == EEffectLayerKind::SkeletalMesh)
    {
        auto meshObj = dynamic_pointer_cast<EffectMeshObject>(layer.obj);
        if (!meshObj)
            return;

        const Vec4 colorTint = layer.desc.mesh.useColorTintOverTime
            ? Lerp_EffectVec4(layer.desc.mesh.colorTint, layer.desc.mesh.endColorTint, layerProgress)
            : layer.desc.mesh.colorTint;
        const float opacity = layer.desc.mesh.useOpacityOverTime
            ? Lerp_EffectFloat(layer.desc.mesh.opacity, layer.desc.mesh.endOpacity, layerProgress)
            : layer.desc.mesh.opacity;
        const float emissiveStrength = layer.desc.mesh.useEmissiveStrengthOverTime
            ? Lerp_EffectFloat(layer.desc.mesh.emissiveStrength, layer.desc.mesh.endEmissiveStrength, layerProgress)
            : layer.desc.mesh.emissiveStrength;

        meshObj->Set_RuntimeColorTintOverride(colorTint, layer.desc.mesh.useColorTintOverTime);
        meshObj->Set_RuntimeOpacityOverride(opacity, layer.desc.mesh.useOpacityOverTime);
        meshObj->Set_RuntimeEmissiveStrengthOverride(
            emissiveStrength,
            layer.desc.mesh.useEmissiveStrengthOverTime);
        return;
    }

    if (layer.desc.base.kind == EEffectLayerKind::Point)
    {
        auto pointObj = dynamic_pointer_cast<Particle_Point>(layer.obj);
        if (!pointObj)
            return;

        const Vec4 colorTint = layer.desc.point.useColorTintOverTime
            ? Lerp_EffectVec4(layer.desc.point.colorTint, layer.desc.point.endColorTint, layerProgress)
            : layer.desc.point.colorTint;
        const float opacity = layer.desc.point.useOpacityOverTime
            ? Lerp_EffectFloat(layer.desc.point.opacity, layer.desc.point.endOpacity, layerProgress)
            : layer.desc.point.opacity;

        pointObj->Set_ColorTint(colorTint);
        pointObj->Set_Opacity(opacity);
        return;
    }

    if (layer.desc.base.kind == EEffectLayerKind::BillboardRect)
    {
        auto billboardObj = dynamic_pointer_cast<EffectBillboardObject>(layer.obj);
        if (!billboardObj)
            return;

        const float baseOpacity = layer.desc.billboard.useBaseOpacityOverTime
            ? Lerp_EffectFloat(layer.desc.billboard.baseOpacity, layer.desc.billboard.endBaseOpacity, layerProgress)
            : layer.desc.billboard.baseOpacity;
        const float ringOpacity = layer.desc.billboard.useRingOpacityOverTime
            ? Lerp_EffectFloat(layer.desc.billboard.ringOpacity, layer.desc.billboard.endRingOpacity, layerProgress)
            : layer.desc.billboard.ringOpacity;

        billboardObj->Set_RuntimeBaseOpacityOverride(
            baseOpacity,
            layer.desc.billboard.useBaseOpacityOverTime);
        billboardObj->Set_RuntimeRingOpacityOverride(
            ringOpacity,
            layer.desc.billboard.useRingOpacityOverTime);
    }
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
