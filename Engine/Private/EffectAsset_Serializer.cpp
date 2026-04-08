#include "pch.h"
#include "EffectAsset_Serializer.h"
#include <fstream>

HRESULT EffectAsset_Serializer::Save_EffectAsset(const string& filePath, const FEffectAssetDesc& assetDesc)
{
    json j;
    j["effectName"] = assetDesc.effectName;
    j["autoPlay"] = assetDesc.autoPlay;
    j["totalDuration"] = assetDesc.totalDuration;

    json layers = json::array();
    for (const auto& layer : assetDesc.layers)
    {
        layers.push_back(Serialize_Layer(layer));
    }
    j["layers"] = layers;

    std::ofstream file(filePath);
    if (!file.is_open())
        return E_FAIL;

    file << std::setw(4) << j << std::endl;
    return S_OK;
}

HRESULT EffectAsset_Serializer::Load_EffectAsset(const string& filePath, FEffectAssetDesc& outAssetDesc)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return E_FAIL;

    json j;
    file >> j;

    if (j.contains("effectName")) outAssetDesc.effectName = j["effectName"];
    if (j.contains("autoPlay")) outAssetDesc.autoPlay = j["autoPlay"];
    if (j.contains("totalDuration")) outAssetDesc.totalDuration = j["totalDuration"];

    outAssetDesc.layers.clear();
    if (j.contains("layers"))
    {
        for (const auto& layerJson : j["layers"])
        {
            outAssetDesc.layers.push_back(Deserialize_Layer(layerJson));
        }
    }
    return S_OK;
}

json EffectAsset_Serializer::Serialize_Layer(const FEffectLayerDesc& layerDesc)
{
    json j;
    j["layerName"] = layerDesc.base.layerName;
    j["enabled"] = layerDesc.base.enabled;
    j["kind"] = (int)layerDesc.base.kind;
    j["startDelay"] = layerDesc.base.startDelay;
    j["duration"] = layerDesc.base.duration;
    j["loop"] = layerDesc.base.loop;

    j["localPosition"] = {
        layerDesc.base.localPosition.x,
        layerDesc.base.localPosition.y,
        layerDesc.base.localPosition.z
    };
    j["localRotation"] = {
        layerDesc.base.localRotation.x,
        layerDesc.base.localRotation.y,
        layerDesc.base.localRotation.z
    };
    j["localScale"] = {
        layerDesc.base.localScale.x,
        layerDesc.base.localScale.y,
        layerDesc.base.localScale.z
    };

    if (layerDesc.base.kind == EEffectLayerKind::Mesh)
    {
        const auto& mesh = layerDesc.mesh;
        j["modelGuid"] = mesh.modelGuid;
        j["diffuseTextureGuid"] = mesh.diffuseTextureGuid;
        j["maskTextureGuid"] = mesh.maskTextureGuid;
        j["blendMode"] = (int)mesh.blendMode;

        j["uvScrollSpeed"] = { mesh.uvScrollSpeed.x, mesh.uvScrollSpeed.y };
        j["uvTiling"] = { mesh.uvTiling.x, mesh.uvTiling.y };
        j["colorTint"] = { mesh.colorTint.x, mesh.colorTint.y, mesh.colorTint.z, mesh.colorTint.w };

        j["opacity"] = mesh.opacity;
        j["rotationAxis"] = { mesh.rotationAxis.x, mesh.rotationAxis.y, mesh.rotationAxis.z };
        j["rotationSpeed"] = mesh.rotationSpeed;
        j["fresnelPower"] = mesh.fresnelPower;
        j["fresnelMultiplier"] = mesh.fresnelMultiplier;
        j["twoSided"] = mesh.twoSided;
    }

    /* [추가] Point 레이어 직렬화는 2차에서 추가한다.
       현재 Effect View는 Mesh Effect 1차 제작/프리뷰를 우선 지원한다. */

    return j;
}


FEffectLayerDesc EffectAsset_Serializer::Deserialize_Layer(const json& j)
{
    FEffectLayerDesc layer{};
    if (j.contains("layerName")) layer.base.layerName = j["layerName"];
    if (j.contains("enabled")) layer.base.enabled = j["enabled"];
    if (j.contains("kind")) layer.base.kind = (EEffectLayerKind)j["kind"].get<int>();
    if (j.contains("startDelay")) layer.base.startDelay = j["startDelay"];
    if (j.contains("duration")) layer.base.duration = j["duration"];
    if (j.contains("loop")) layer.base.loop = j["loop"];

    if (j.contains("localPosition")) layer.base.localPosition = Vec3(j["localPosition"][0], j["localPosition"][1], j["localPosition"][2]);
    if (j.contains("localRotation")) layer.base.localRotation = Vec3(j["localRotation"][0], j["localRotation"][1], j["localRotation"][2]);
    if (j.contains("localScale")) layer.base.localScale = Vec3(j["localScale"][0], j["localScale"][1], j["localScale"][2]);

    if (layer.base.kind == EEffectLayerKind::Mesh)
    {
        auto& mesh = layer.mesh;
        if (j.contains("modelGuid")) mesh.modelGuid = j["modelGuid"];
        if (j.contains("diffuseTextureGuid")) mesh.diffuseTextureGuid = j["diffuseTextureGuid"];
        if (j.contains("maskTextureGuid")) mesh.maskTextureGuid = j["maskTextureGuid"];
        if (j.contains("blendMode")) mesh.blendMode = (EEffectBlendMode)j["blendMode"].get<int>();

        if (j.contains("uvScrollSpeed")) mesh.uvScrollSpeed = Vec2(j["uvScrollSpeed"][0], j["uvScrollSpeed"][1]);
        if (j.contains("uvTiling")) mesh.uvTiling = Vec2(j["uvTiling"][0], j["uvTiling"][1]);
        if (j.contains("colorTint")) mesh.colorTint = Vec4(j["colorTint"][0], j["colorTint"][1], j["colorTint"][2], j["colorTint"][3]);

        if (j.contains("opacity")) mesh.opacity = j["opacity"];
        if (j.contains("rotationAxis")) mesh.rotationAxis = Vec3(j["rotationAxis"][0], j["rotationAxis"][1], j["rotationAxis"][2]);
        if (j.contains("rotationSpeed")) mesh.rotationSpeed = j["rotationSpeed"];
        if (j.contains("fresnelPower")) mesh.fresnelPower = j["fresnelPower"];
        if (j.contains("fresnelMultiplier")) mesh.fresnelMultiplier = j["fresnelMultiplier"];
        if (j.contains("twoSided")) mesh.twoSided = j["twoSided"];
    }
    return layer;
}
