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
    j["usePositionOverTime"] = layerDesc.base.usePositionOverTime;
    j["endPosition"] = {
        layerDesc.base.endPosition.x,
        layerDesc.base.endPosition.y,
        layerDesc.base.endPosition.z
    };
    j["positionDuration"] = layerDesc.base.positionDuration;
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
        j["opacitySubUvTextureGuid"] = mesh.opacitySubUvTextureGuid;
        j["opacityGradationTextureGuid"] = mesh.opacityGradationTextureGuid;
        j["emissiveGradationTextureGuid"] = mesh.emissiveGradationTextureGuid;
        j["uvDistortionTextureGuid"] = mesh.uvDistortionTextureGuid;
        j["normalTextureGuid"] = mesh.normalTextureGuid;
        j["roughnessTextureGuid"] = mesh.roughnessTextureGuid;
        j["specularTextureGuid"] = mesh.specularTextureGuid;
        j["blendMode"] = (int)mesh.blendMode;
        j["shadingMode"] = static_cast<int>(mesh.shadingMode);

        j["uvScrollSpeed"] = { mesh.uvScrollSpeed.x, mesh.uvScrollSpeed.y };
        j["uvTiling"] = { mesh.uvTiling.x, mesh.uvTiling.y };
        j["uvDistortionStrength"] = { mesh.uvDistortionStrength.x, mesh.uvDistortionStrength.y };
        j["uvDistortionSpeed"] = { mesh.uvDistortionSpeed.x, mesh.uvDistortionSpeed.y };
        j["flipbook"] = {
            { "enabled", mesh.flipbook.enabled },
            { "columns", mesh.flipbook.columns },
            { "rows", mesh.flipbook.rows },
            { "fps", mesh.flipbook.fps },
            { "startFrame", mesh.flipbook.startFrame },
            { "endFrame", mesh.flipbook.endFrame },
            { "loop", mesh.flipbook.loop }
        };
        j["colorTint"] = { mesh.colorTint.x, mesh.colorTint.y, mesh.colorTint.z, mesh.colorTint.w };

        j["opacity"] = mesh.opacity;
        j["normalStrength"] = mesh.normalStrength;
        j["roughness"] = mesh.roughness;
        j["specularStrength"] = mesh.specularStrength;
        j["specularPower"] = mesh.specularPower;
        j["emissiveStrength"] = mesh.emissiveStrength;
        j["useColorTintOverTime"] = mesh.useColorTintOverTime;
        j["endColorTint"] = { mesh.endColorTint.x, mesh.endColorTint.y, mesh.endColorTint.z, mesh.endColorTint.w };
        j["useOpacityOverTime"] = mesh.useOpacityOverTime;
        j["endOpacity"] = mesh.endOpacity;
        j["useEmissiveStrengthOverTime"] = mesh.useEmissiveStrengthOverTime;
        j["endEmissiveStrength"] = mesh.endEmissiveStrength;
        j["rotationAxis"] = { mesh.rotationAxis.x, mesh.rotationAxis.y, mesh.rotationAxis.z };
        j["rotationSpeed"] = mesh.rotationSpeed;
        j["fresnelPower"] = mesh.fresnelPower;
        j["fresnelMultiplier"] = mesh.fresnelMultiplier;
        j["twoSided"] = mesh.twoSided;
        j["useOpacityAsTransparency"] = mesh.useOpacityAsTransparency;
        j["customParams0"] = { mesh.customParams0.x, mesh.customParams0.y, mesh.customParams0.z, mesh.customParams0.w };
        j["customParams1"] = { mesh.customParams1.x, mesh.customParams1.y, mesh.customParams1.z, mesh.customParams1.w };
    }
    else if (layerDesc.base.kind == EEffectLayerKind::Point)
    {
        const auto& point = layerDesc.point;
        j["textureGuid"] = point.textureGuid;
        j["maskTextureGuid"] = point.maskTextureGuid;
        j["opacityTextureGuid"] = point.opacityTextureGuid;
        j["numInstances"] = point.numInstances;
        j["center"] = { point.center.x, point.center.y, point.center.z };
        j["range"] = { point.range.x, point.range.y, point.range.z };
        j["spawnShape"] = static_cast<int>(point.spawnShape);
        j["spawnRadius"] = point.spawnRadius;
        j["spawnInnerRadius"] = point.spawnInnerRadius;
        j["spawnHeight"] = point.spawnHeight;
        j["scale"] = { point.scale.x, point.scale.y };
        j["speed"] = { point.speed.x, point.speed.y };
        j["lifeTime"] = { point.lifeTime.x, point.lifeTime.y };
        j["pivot"] = { point.pivot.x, point.pivot.y, point.pivot.z };
        j["isLoop"] = point.isLoop;
        j["blendMode"] = static_cast<int>(point.blendMode);
        j["colorTint"] = { point.colorTint.x, point.colorTint.y, point.colorTint.z, point.colorTint.w };
        j["emissiveStrength"] = point.emissiveStrength;
        j["useColorTintOverTime"] = point.useColorTintOverTime;
        j["endColorTint"] = { point.endColorTint.x, point.endColorTint.y, point.endColorTint.z, point.endColorTint.w };
        j["opacity"] = point.opacity;
        j["useOpacityOverTime"] = point.useOpacityOverTime;
        j["endOpacity"] = point.endOpacity;
        j["flipbook"] = {
            { "enabled", point.flipbook.enabled },
            { "columns", point.flipbook.columns },
            { "rows", point.flipbook.rows },
            { "fps", point.flipbook.fps },
            { "startFrame", point.flipbook.startFrame },
            { "endFrame", point.flipbook.endFrame },
            { "loop", point.flipbook.loop }
        };
        j["customParams0"] = { point.customParams0.x, point.customParams0.y, point.customParams0.z, point.customParams0.w };
        j["customParams1"] = { point.customParams1.x, point.customParams1.y, point.customParams1.z, point.customParams1.w };
        j["moveMode"] = point.moveMode;
        j["moveMode"] = point.moveMode;
        j["lockWorldOnSpawn"] = point.lockWorldOnSpawn;

    }
    else if (layerDesc.base.kind == EEffectLayerKind::BillboardRect)
    {
        const auto& billboard = layerDesc.billboard;
        j["baseTextureGuid"] = billboard.baseTextureGuid;
        j["baseMaskTextureGuid"] = billboard.baseMaskTextureGuid;
        j["baseOpacityTextureGuid"] = billboard.baseOpacityTextureGuid;
        j["baseOpacityGradationTextureGuid"] = billboard.baseOpacityGradationTextureGuid;
        j["ringTextureGuid"] = billboard.ringTextureGuid;
        j["ringOpacityTextureGuid"] = billboard.ringOpacityTextureGuid;
        j["ringOpacityGradationTextureGuid"] = billboard.ringOpacityGradationTextureGuid;
        j["blendMode"] = static_cast<int>(billboard.blendMode);
        j["renderMode"] = static_cast<int>(billboard.renderMode);
        j["baseTint"] = { billboard.baseTint.x, billboard.baseTint.y, billboard.baseTint.z, billboard.baseTint.w };
        j["ringTint"] = { billboard.ringTint.x, billboard.ringTint.y, billboard.ringTint.z, billboard.ringTint.w };
        j["baseOpacity"] = billboard.baseOpacity;
        j["ringOpacity"] = billboard.ringOpacity;
        j["baseEmissiveStrength"] = billboard.baseEmissiveStrength;
        j["ringEmissiveStrength"] = billboard.ringEmissiveStrength;
        j["useBaseOpacityOverTime"] = billboard.useBaseOpacityOverTime;
        j["endBaseOpacity"] = billboard.endBaseOpacity;
        j["useRingOpacityOverTime"] = billboard.useRingOpacityOverTime;
        j["endRingOpacity"] = billboard.endRingOpacity;
        j["baseFlipbook"] = {
            { "enabled", billboard.baseFlipbook.enabled },
            { "columns", billboard.baseFlipbook.columns },
            { "rows", billboard.baseFlipbook.rows },
            { "fps", billboard.baseFlipbook.fps },
            { "startFrame", billboard.baseFlipbook.startFrame },
            { "endFrame", billboard.baseFlipbook.endFrame },
            { "loop", billboard.baseFlipbook.loop }
        };
        j["ringFlipbook"] = {
            { "enabled", billboard.ringFlipbook.enabled },
            { "columns", billboard.ringFlipbook.columns },
            { "rows", billboard.ringFlipbook.rows },
            { "fps", billboard.ringFlipbook.fps },
            { "startFrame", billboard.ringFlipbook.startFrame },
            { "endFrame", billboard.ringFlipbook.endFrame },
            { "loop", billboard.ringFlipbook.loop }
        };
        j["useRing"] = billboard.useRing;
        j["billboardToCamera"] = billboard.billboardToCamera;
        j["customParams0"] = { billboard.customParams0.x, billboard.customParams0.y, billboard.customParams0.z, billboard.customParams0.w };
        j["customParams1"] = { billboard.customParams1.x, billboard.customParams1.y, billboard.customParams1.z, billboard.customParams1.w };
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
    if (j.contains("usePositionOverTime")) layer.base.usePositionOverTime = j["usePositionOverTime"];
    if (j.contains("endPosition")) layer.base.endPosition = Vec3(j["endPosition"][0], j["endPosition"][1], j["endPosition"][2]);
    if (j.contains("positionDuration")) layer.base.positionDuration = j["positionDuration"];
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
        if (j.contains("opacitySubUvTextureGuid")) mesh.opacitySubUvTextureGuid = j["opacitySubUvTextureGuid"];
        if (j.contains("opacityGradationTextureGuid")) mesh.opacityGradationTextureGuid = j["opacityGradationTextureGuid"];
        if (j.contains("emissiveGradationTextureGuid")) mesh.emissiveGradationTextureGuid = j["emissiveGradationTextureGuid"];
        if (j.contains("uvDistortionTextureGuid")) mesh.uvDistortionTextureGuid = j["uvDistortionTextureGuid"];
        if (j.contains("normalTextureGuid")) mesh.normalTextureGuid = j["normalTextureGuid"];
        if (j.contains("roughnessTextureGuid")) mesh.roughnessTextureGuid = j["roughnessTextureGuid"];
        if (j.contains("specularTextureGuid")) mesh.specularTextureGuid = j["specularTextureGuid"];
        if (j.contains("blendMode")) mesh.blendMode = (EEffectBlendMode)j["blendMode"].get<int>();
        if (j.contains("shadingMode")) mesh.shadingMode = static_cast<EEffectMeshShadingMode>(j["shadingMode"].get<int>());

        if (j.contains("uvScrollSpeed")) mesh.uvScrollSpeed = Vec2(j["uvScrollSpeed"][0], j["uvScrollSpeed"][1]);
        if (j.contains("uvTiling")) mesh.uvTiling = Vec2(j["uvTiling"][0], j["uvTiling"][1]);
        if (j.contains("uvDistortionStrength")) mesh.uvDistortionStrength = Vec2(j["uvDistortionStrength"][0], j["uvDistortionStrength"][1]);
        if (j.contains("uvDistortionSpeed")) mesh.uvDistortionSpeed = Vec2(j["uvDistortionSpeed"][0], j["uvDistortionSpeed"][1]);
        if (j.contains("flipbook"))
        {
            const auto& flipbook = j["flipbook"];
            if (flipbook.contains("enabled")) mesh.flipbook.enabled = flipbook["enabled"];
            if (flipbook.contains("columns")) mesh.flipbook.columns = flipbook["columns"];
            if (flipbook.contains("rows")) mesh.flipbook.rows = flipbook["rows"];
            if (flipbook.contains("fps")) mesh.flipbook.fps = flipbook["fps"];
            if (flipbook.contains("startFrame")) mesh.flipbook.startFrame = flipbook["startFrame"];
            if (flipbook.contains("endFrame")) mesh.flipbook.endFrame = flipbook["endFrame"];
            if (flipbook.contains("loop")) mesh.flipbook.loop = flipbook["loop"];
        }
        if (j.contains("colorTint")) mesh.colorTint = Vec4(j["colorTint"][0], j["colorTint"][1], j["colorTint"][2], j["colorTint"][3]);

        if (j.contains("opacity")) mesh.opacity = j["opacity"];
        if (j.contains("normalStrength")) mesh.normalStrength = j["normalStrength"];
        if (j.contains("roughness")) mesh.roughness = j["roughness"];
        if (j.contains("specularStrength")) mesh.specularStrength = j["specularStrength"];
        if (j.contains("specularPower")) mesh.specularPower = j["specularPower"];
        if (j.contains("emissiveStrength")) mesh.emissiveStrength = j["emissiveStrength"];
        if (j.contains("useColorTintOverTime")) mesh.useColorTintOverTime = j["useColorTintOverTime"];
        if (j.contains("endColorTint")) mesh.endColorTint = Vec4(j["endColorTint"][0], j["endColorTint"][1], j["endColorTint"][2], j["endColorTint"][3]);
        if (j.contains("useOpacityOverTime")) mesh.useOpacityOverTime = j["useOpacityOverTime"];
        if (j.contains("endOpacity")) mesh.endOpacity = j["endOpacity"];
        if (j.contains("useEmissiveStrengthOverTime")) mesh.useEmissiveStrengthOverTime = j["useEmissiveStrengthOverTime"];
        if (j.contains("endEmissiveStrength")) mesh.endEmissiveStrength = j["endEmissiveStrength"];
        if (j.contains("rotationAxis")) mesh.rotationAxis = Vec3(j["rotationAxis"][0], j["rotationAxis"][1], j["rotationAxis"][2]);
        if (j.contains("rotationSpeed")) mesh.rotationSpeed = j["rotationSpeed"];
        if (j.contains("fresnelPower")) mesh.fresnelPower = j["fresnelPower"];
        if (j.contains("fresnelMultiplier")) mesh.fresnelMultiplier = j["fresnelMultiplier"];
        if (j.contains("twoSided")) mesh.twoSided = j["twoSided"];
        if (j.contains("useOpacityAsTransparency")) mesh.useOpacityAsTransparency = j["useOpacityAsTransparency"];
        if (j.contains("customParams0")) mesh.customParams0 = Vec4(j["customParams0"][0], j["customParams0"][1], j["customParams0"][2], j["customParams0"][3]);
        if (j.contains("customParams1")) mesh.customParams1 = Vec4(j["customParams1"][0], j["customParams1"][1], j["customParams1"][2], j["customParams1"][3]);
    }
    else if (layer.base.kind == EEffectLayerKind::Point)
    {
        auto& point = layer.point;
        if (j.contains("textureGuid")) point.textureGuid = j["textureGuid"];
        if (j.contains("maskTextureGuid")) point.maskTextureGuid = j["maskTextureGuid"];
        if (j.contains("opacityTextureGuid")) point.opacityTextureGuid = j["opacityTextureGuid"];
        if (j.contains("numInstances")) point.numInstances = j["numInstances"];
        if (j.contains("center")) point.center = Vec3(j["center"][0], j["center"][1], j["center"][2]);
        if (j.contains("range")) point.range = Vec3(j["range"][0], j["range"][1], j["range"][2]);
        if (j.contains("spawnShape")) point.spawnShape = static_cast<EEffectPointSpawnShape>(j["spawnShape"].get<int>());
        if (j.contains("spawnRadius")) point.spawnRadius = j["spawnRadius"];
        if (j.contains("spawnInnerRadius")) point.spawnInnerRadius = j["spawnInnerRadius"];
        if (j.contains("spawnHeight")) point.spawnHeight = j["spawnHeight"];
        if (j.contains("scale")) point.scale = Vec2(j["scale"][0], j["scale"][1]);
        if (j.contains("speed")) point.speed = Vec2(j["speed"][0], j["speed"][1]);
        if (j.contains("lifeTime")) point.lifeTime = Vec2(j["lifeTime"][0], j["lifeTime"][1]);
        if (j.contains("pivot")) point.pivot = Vec3(j["pivot"][0], j["pivot"][1], j["pivot"][2]);
        if (j.contains("isLoop")) point.isLoop = j["isLoop"];
        if (j.contains("blendMode")) point.blendMode = static_cast<EEffectBlendMode>(j["blendMode"].get<int>());
        if (j.contains("colorTint")) point.colorTint = Vec4(j["colorTint"][0], j["colorTint"][1], j["colorTint"][2], j["colorTint"][3]);
        if (j.contains("emissiveStrength")) point.emissiveStrength = j["emissiveStrength"];
        if (j.contains("useColorTintOverTime")) point.useColorTintOverTime = j["useColorTintOverTime"];
        if (j.contains("endColorTint")) point.endColorTint = Vec4(j["endColorTint"][0], j["endColorTint"][1], j["endColorTint"][2], j["endColorTint"][3]);
        if (j.contains("opacity")) point.opacity = j["opacity"];
        if (j.contains("useOpacityOverTime")) point.useOpacityOverTime = j["useOpacityOverTime"];
        if (j.contains("endOpacity")) point.endOpacity = j["endOpacity"];
        if (j.contains("flipbook"))
        {
            const auto& flipbook = j["flipbook"];
            if (flipbook.contains("enabled")) point.flipbook.enabled = flipbook["enabled"];
            if (flipbook.contains("columns")) point.flipbook.columns = flipbook["columns"];
            if (flipbook.contains("rows")) point.flipbook.rows = flipbook["rows"];
            if (flipbook.contains("fps")) point.flipbook.fps = flipbook["fps"];
            if (flipbook.contains("startFrame")) point.flipbook.startFrame = flipbook["startFrame"];
            if (flipbook.contains("endFrame")) point.flipbook.endFrame = flipbook["endFrame"];
            if (flipbook.contains("loop")) point.flipbook.loop = flipbook["loop"];
        }
        if (j.contains("customParams0")) point.customParams0 = Vec4(j["customParams0"][0], j["customParams0"][1], j["customParams0"][2], j["customParams0"][3]);
        if (j.contains("customParams1")) point.customParams1 = Vec4(j["customParams1"][0], j["customParams1"][1], j["customParams1"][2], j["customParams1"][3]);
        if (j.contains("moveMode")) point.moveMode = j["moveMode"];
        if (j.contains("moveMode"))        point.moveMode        = j["moveMode"];
        if (j.contains("lockWorldOnSpawn")) point.lockWorldOnSpawn = j["lockWorldOnSpawn"];
    }
    else if (layer.base.kind == EEffectLayerKind::BillboardRect)
    {
        auto& billboard = layer.billboard;
        if (j.contains("baseTextureGuid")) billboard.baseTextureGuid = j["baseTextureGuid"];
        if (j.contains("baseMaskTextureGuid")) billboard.baseMaskTextureGuid = j["baseMaskTextureGuid"];
        if (j.contains("baseOpacityTextureGuid")) billboard.baseOpacityTextureGuid = j["baseOpacityTextureGuid"];
        if (j.contains("baseOpacityGradationTextureGuid")) billboard.baseOpacityGradationTextureGuid = j["baseOpacityGradationTextureGuid"];
        if (j.contains("ringTextureGuid")) billboard.ringTextureGuid = j["ringTextureGuid"];
        if (j.contains("ringOpacityTextureGuid")) billboard.ringOpacityTextureGuid = j["ringOpacityTextureGuid"];
        if (j.contains("ringOpacityGradationTextureGuid")) billboard.ringOpacityGradationTextureGuid = j["ringOpacityGradationTextureGuid"];
        if (j.contains("blendMode")) billboard.blendMode = static_cast<EEffectBlendMode>(j["blendMode"].get<int>());
        if (j.contains("renderMode")) billboard.renderMode = static_cast<EEffectBillboardRenderMode>(j["renderMode"].get<int>());
        if (j.contains("baseTint")) billboard.baseTint = Vec4(j["baseTint"][0], j["baseTint"][1], j["baseTint"][2], j["baseTint"][3]);
        if (j.contains("ringTint")) billboard.ringTint = Vec4(j["ringTint"][0], j["ringTint"][1], j["ringTint"][2], j["ringTint"][3]);
        if (j.contains("baseOpacity")) billboard.baseOpacity = j["baseOpacity"];
        if (j.contains("ringOpacity")) billboard.ringOpacity = j["ringOpacity"];
        if (j.contains("baseEmissiveStrength")) billboard.baseEmissiveStrength = j["baseEmissiveStrength"];
        if (j.contains("ringEmissiveStrength")) billboard.ringEmissiveStrength = j["ringEmissiveStrength"];
        if (j.contains("useBaseOpacityOverTime")) billboard.useBaseOpacityOverTime = j["useBaseOpacityOverTime"];
        if (j.contains("endBaseOpacity")) billboard.endBaseOpacity = j["endBaseOpacity"];
        if (j.contains("useRingOpacityOverTime")) billboard.useRingOpacityOverTime = j["useRingOpacityOverTime"];
        if (j.contains("endRingOpacity")) billboard.endRingOpacity = j["endRingOpacity"];
        if (j.contains("baseFlipbook"))
        {
            const auto& flipbook = j["baseFlipbook"];
            if (flipbook.contains("enabled")) billboard.baseFlipbook.enabled = flipbook["enabled"];
            if (flipbook.contains("columns")) billboard.baseFlipbook.columns = flipbook["columns"];
            if (flipbook.contains("rows")) billboard.baseFlipbook.rows = flipbook["rows"];
            if (flipbook.contains("fps")) billboard.baseFlipbook.fps = flipbook["fps"];
            if (flipbook.contains("startFrame")) billboard.baseFlipbook.startFrame = flipbook["startFrame"];
            if (flipbook.contains("endFrame")) billboard.baseFlipbook.endFrame = flipbook["endFrame"];
            if (flipbook.contains("loop")) billboard.baseFlipbook.loop = flipbook["loop"];
        }
        if (j.contains("ringFlipbook"))
        {
            const auto& flipbook = j["ringFlipbook"];
            if (flipbook.contains("enabled")) billboard.ringFlipbook.enabled = flipbook["enabled"];
            if (flipbook.contains("columns")) billboard.ringFlipbook.columns = flipbook["columns"];
            if (flipbook.contains("rows")) billboard.ringFlipbook.rows = flipbook["rows"];
            if (flipbook.contains("fps")) billboard.ringFlipbook.fps = flipbook["fps"];
            if (flipbook.contains("startFrame")) billboard.ringFlipbook.startFrame = flipbook["startFrame"];
            if (flipbook.contains("endFrame")) billboard.ringFlipbook.endFrame = flipbook["endFrame"];
            if (flipbook.contains("loop")) billboard.ringFlipbook.loop = flipbook["loop"];
        }
        if (j.contains("useRing")) billboard.useRing = j["useRing"];
        if (j.contains("billboardToCamera")) billboard.billboardToCamera = j["billboardToCamera"];
        if (j.contains("customParams0")) billboard.customParams0 = Vec4(j["customParams0"][0], j["customParams0"][1], j["customParams0"][2], j["customParams0"][3]);
        if (j.contains("customParams1")) billboard.customParams1 = Vec4(j["customParams1"][0], j["customParams1"][1], j["customParams1"][2], j["customParams1"][3]);
    }
    return layer;
}
