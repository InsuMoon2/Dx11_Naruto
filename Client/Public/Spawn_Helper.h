#pragma once

NS_BEGIN(Engine)
class GameObject;
NS_END

NS_BEGIN(Client)

// 에디터에서 만든 프리펩 스폰 헬퍼
class Spawn_Helper final
{
public:
    struct FSpawnDesc
    {
        string  prefabName;
        uint32  levelIndex = ETOI(ELevelType::GamePlay);
        wstring layerTag = TEXT("Layer_GameObject");

        json    overrides;
    };

    class Builder
    {
    public:
        explicit Builder(const string& prefabName);

    public:
        Builder& AtLevel(uint32 levelIndex);
        Builder& InLayer(const wstring& layerTag);
        Builder& SpawnAs(uint32 objectType);

        Builder& Position(const Vec3& position);
        Builder& Rotation(const Vec3& degree);
        Builder& Scale(const Vec3& scale);

        Shared<GameObject> Spawn() const;

    private:
        FSpawnDesc _spawnDesc;

    };

    static Builder Prefab(const string& prefabName) { return Builder(prefabName); }

    // 일반 스폰
    static Shared<GameObject> Spawn(
        const string& prefabName, uint32 levelIndex, const wstring& layerTag);

    // Position 세팅
    static Shared<GameObject> SpawnAt(
        const string& prefabName, uint32 levelIndex, const wstring& layerTag, const Vec3& position);

private:
    static Shared<GameObject> Spawn_Implementation(const FSpawnDesc& desc);

};

NS_END
