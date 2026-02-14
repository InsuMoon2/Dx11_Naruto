#include "pch.h"
#include "Spawn_Helper.h"

Spawn_Helper::Builder::Builder(const string& prefabName)
{
    _spawnDesc.prefabName = prefabName;

}

Spawn_Helper::Builder& Spawn_Helper::Builder::AtLevel(uint32 levelIndex)
{
    _spawnDesc.levelIndex = levelIndex;

    return *this;
}

Spawn_Helper::Builder& Spawn_Helper::Builder::InLayer(const wstring& layerTag)
{
    _spawnDesc.layerTag = layerTag;

    return *this;
}

Spawn_Helper::Builder& Spawn_Helper::Builder::Position(const Vec3& position)
{
    _spawnDesc.overrides["position"] = { position.x, position.y, position.z };

    return *this;
}

Spawn_Helper::Builder& Spawn_Helper::Builder::Rotation(const Vec3& degree)
{
    _spawnDesc.overrides["rotation"] = { degree.x, degree.y, degree.z };

    return *this;
}

Spawn_Helper::Builder& Spawn_Helper::Builder::Scale(const Vec3& scale)
{
    _spawnDesc.overrides["scale"] = { scale.x, scale.y, scale.z };

    return *this;
}

Shared<GameObject> Spawn_Helper::Builder::Spawn() const
{
    return Spawn_Helper::Spawn_Implementation(_spawnDesc);
}

Shared<GameObject> Spawn_Helper::Spawn(const string& prefabName, uint32 levelIndex, const wstring& layerTag)
{
    FSpawnDesc desc;
    desc.prefabName = prefabName;
    desc.levelIndex = levelIndex;
    desc.layerTag = layerTag;

    return Spawn_Implementation(desc);
}

Shared<GameObject> Spawn_Helper::SpawnAt(const string& prefabName, uint32 levelIndex, const wstring& layerTag,
    const Vec3& position)
{
    FSpawnDesc desc;
    desc.prefabName = prefabName;
    desc.levelIndex = levelIndex;
    desc.layerTag = layerTag;
    desc.overrides["position"] = { position.x, position.y, position.z };

    return Spawn_Implementation(desc);
}

Shared<GameObject> Spawn_Helper::Spawn_Implementation(const FSpawnDesc& desc)
{
    auto gameObject = GAME->Instantiate_Prefab(desc.prefabName, desc.overrides);
    CHECK_NULL(gameObject, nullptr);

    CHECK_FAILED(GAME->Add_GameObject(desc.levelIndex, desc.layerTag, gameObject), nullptr);

    return gameObject;
}
