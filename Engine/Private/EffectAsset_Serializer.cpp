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

fs::path EffectAsset_Serializer::Get_EffectFolderPath()
{
    return fs::path("../../Client/Bin/Resources/Data/json/Effects");
}

vector<fs::path> EffectAsset_Serializer::Get_EffectFiles()
{
    vector <fs::path> files;
    fs::path folder = Get_EffectFolderPath();

    // 폴더 없으면 생성
    if (!fs::exists(folder))
    {
        fs::create_directories(folder);
        return files;
    }

    for (const auto& entry : fs::directory_iterator(folder))
    {
        if (!entry.is_regular_file()) // 폴더가 아닌 파일인지?
            continue;

        string fileName = Utils::ToLowerCopy(entry.path().filename().string()); // 이름 소문자 처리
        if (fileName.ends_with(".effect.json"))
        {
            files.push_back(entry.path());
        }
    }

    sort(files.begin(), files.end());

    return files;
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
    j["useScaleOverTime"] = layerDesc.base.useScaleOverTime;
    j["endScale"] = {
        layerDesc.base.endScale.x,
        layerDesc.base.endScale.y,
        layerDesc.base.endScale.z
    };

    if (layerDesc.base.kind == EEffectLayerKind::Mesh)
    {
        const auto& mesh = layerDesc.mesh;
        j["modelGuid"] = mesh.modelGuid;
        j["diffuseTextureGuid"] = mesh.diffuseTextureGuid;
        j["maskTextureGuid"] = mesh.maskTextureGuid;
        j["emissiveTextureGuid"] = mesh.emissiveTextureGuid;
        j["opacityTextureGuid"] = mesh.opacityTextureGuid;
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
    else if (layerDesc.base.kind == EEffectLayerKind::Point)
    {
        const auto& point = layerDesc.point;
        j["textureGuid"] = point.textureGuid;
        j["numInstances"] = point.numInstances;
        j["center"] = { point.center.x, point.center.y, point.center.z };
        j["range"] = { point.range.x, point.range.y, point.range.z };
        j["scale"] = { point.scale.x, point.scale.y };
        j["speed"] = { point.speed.x, point.speed.y };
        j["lifeTime"] = { point.lifeTime.x, point.lifeTime.y };
        j["pivot"] = { point.pivot.x, point.pivot.y, point.pivot.z };
        j["isLoop"] = point.isLoop;
        j["blendMode"] = static_cast<int>(point.blendMode);
        j["colorTint"] = { point.colorTint.x, point.colorTint.y, point.colorTint.z, point.colorTint.w };
        j["opacity"] = point.opacity;
        j["moveMode"] = point.moveMode;
    }
    else if (layerDesc.base.kind == EEffectLayerKind::BillboardRect)
    {
        const auto& billboard = layerDesc.billboard;
        j["baseTextureGuid"] = billboard.baseTextureGuid;
        j["ringTextureGuid"] = billboard.ringTextureGuid;
        j["blendMode"] = static_cast<int>(billboard.blendMode);
        j["baseTint"] = { billboard.baseTint.x, billboard.baseTint.y, billboard.baseTint.z, billboard.baseTint.w };
        j["ringTint"] = { billboard.ringTint.x, billboard.ringTint.y, billboard.ringTint.z, billboard.ringTint.w };
        j["baseOpacity"] = billboard.baseOpacity;
        j["ringOpacity"] = billboard.ringOpacity;
        j["useRing"] = billboard.useRing;
        j["billboardToCamera"] = billboard.billboardToCamera;
    }

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
    if (j.contains("useScaleOverTime")) layer.base.useScaleOverTime = j["useScaleOverTime"];
    if (j.contains("endScale")) layer.base.endScale = Vec3(j["endScale"][0], j["endScale"][1], j["endScale"][2]);

    if (layer.base.kind == EEffectLayerKind::Mesh)
    {
        auto& mesh = layer.mesh;
        if (j.contains("modelGuid")) mesh.modelGuid = j["modelGuid"];
        if (j.contains("diffuseTextureGuid")) mesh.diffuseTextureGuid = j["diffuseTextureGuid"];
        if (j.contains("maskTextureGuid")) mesh.maskTextureGuid = j["maskTextureGuid"];
        if (j.contains("emissiveTextureGuid")) mesh.emissiveTextureGuid = j["emissiveTextureGuid"];
        if (j.contains("opacityTextureGuid")) mesh.opacityTextureGuid = j["opacityTextureGuid"];
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
    else if (layer.base.kind == EEffectLayerKind::Point)
    {
        auto& point = layer.point;
        if (j.contains("textureGuid")) point.textureGuid = j["textureGuid"];
        if (j.contains("numInstances")) point.numInstances = j["numInstances"];
        if (j.contains("center")) point.center = Vec3(j["center"][0], j["center"][1], j["center"][2]);
        if (j.contains("range")) point.range = Vec3(j["range"][0], j["range"][1], j["range"][2]);
        if (j.contains("scale")) point.scale = Vec2(j["scale"][0], j["scale"][1]);
        if (j.contains("speed")) point.speed = Vec2(j["speed"][0], j["speed"][1]);
        if (j.contains("lifeTime")) point.lifeTime = Vec2(j["lifeTime"][0], j["lifeTime"][1]);
        if (j.contains("pivot")) point.pivot = Vec3(j["pivot"][0], j["pivot"][1], j["pivot"][2]);
        if (j.contains("isLoop")) point.isLoop = j["isLoop"];
        if (j.contains("blendMode")) point.blendMode = static_cast<EEffectBlendMode>(j["blendMode"].get<int>());
        if (j.contains("colorTint")) point.colorTint = Vec4(j["colorTint"][0], j["colorTint"][1], j["colorTint"][2], j["colorTint"][3]);
        if (j.contains("opacity")) point.opacity = j["opacity"];
        if (j.contains("moveMode")) point.moveMode = j["moveMode"];
    }
    else if (layer.base.kind == EEffectLayerKind::BillboardRect)
    {
        auto& billboard = layer.billboard;
        if (j.contains("baseTextureGuid")) billboard.baseTextureGuid = j["baseTextureGuid"];
        if (j.contains("ringTextureGuid")) billboard.ringTextureGuid = j["ringTextureGuid"];
        if (j.contains("blendMode")) billboard.blendMode = static_cast<EEffectBlendMode>(j["blendMode"].get<int>());
        if (j.contains("baseTint")) billboard.baseTint = Vec4(j["baseTint"][0], j["baseTint"][1], j["baseTint"][2], j["baseTint"][3]);
        if (j.contains("ringTint")) billboard.ringTint = Vec4(j["ringTint"][0], j["ringTint"][1], j["ringTint"][2], j["ringTint"][3]);
        if (j.contains("baseOpacity")) billboard.baseOpacity = j["baseOpacity"];
        if (j.contains("ringOpacity")) billboard.ringOpacity = j["ringOpacity"];
        if (j.contains("useRing")) billboard.useRing = j["useRing"];
        if (j.contains("billboardToCamera")) billboard.billboardToCamera = j["billboardToCamera"];
    }
    return layer;
}
