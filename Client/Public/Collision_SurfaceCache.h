#pragma once

#include "Base.h"

NS_BEGIN(Client)

class Collision_SurfaceCache final : public Base
{
public:
    enum class EEntryType : uint32
    {
        Walkable = 0,
        WallRun = 1,
        WorldBlock = 2,
        Unknown = 0xFFFFFFFF
    };

    struct FEntry
    {
        EEntryType type = EEntryType::Unknown;
        string staticClass;
        string modelGuid;
        Vec3 position = Vec3::Zero;
        Vec3 rotation = Vec3::Zero;
        Vec3 scale = Vec3(1.f, 1.f, 1.f);
    };

    struct FCellCoord
    {
        int32 x = 0;
        int32 z = 0;

        bool operator==(const FCellCoord& rhs) const
        {
            return x == rhs.x && z == rhs.z;
        }
    };

    struct FCellCoordHasher
    {
        size_t operator()(const FCellCoord& key) const
        {
            const size_t hx = std::hash<int32>{}(key.x);
            const size_t hz = std::hash<int32>{}(key.z);
            return hx ^ (hz << 1);
        }
    };

public:
    explicit Collision_SurfaceCache() = default;
    virtual ~Collision_SurfaceCache() = default;

public:
    HRESULT Load(const fs::path& filePath);
    void Clear();

    void Build_GridIndex(float cellSize);

    const vector<FEntry>& Get_Entries() const { return _entries; }
    size_t Get_TotalCount() const { return _entries.size(); }
    size_t Get_Count(EEntryType type) const;

    float Get_CellSize() const { return _cellSize; }

    static EEntryType Parse_Type(uint32 rawType);
    static const char* ToString(EEntryType type);

public:
    static Unique<Collision_SurfaceCache> Create();
    void Free() override;

private:
    static FCellCoord Build_CellCoord(const Vec3& worldPosition, float cellSize);

private:
    vector<FEntry> _entries;
    unordered_map<FCellCoord, vector<uint32>, FCellCoordHasher> _grid;
    float _cellSize = 0.f;
};

NS_END
