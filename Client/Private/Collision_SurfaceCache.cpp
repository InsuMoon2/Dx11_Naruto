#include "pch.h"
#include "Collision_SurfaceCache.h"

#include <fstream>

#pragma pack(push, 1)
struct FCollision_SurfaceCacheFileHeader
{
    char magic[4];
    uint32 version;
    uint32 recordCount;
    uint32 reserved;
};

struct FCollision_SurfaceCacheFileRecord
{
    uint32 proxyType;
    char staticClass[64];
    char modelGuid[37];
    float position[3];
    float rotation[3];
    float scale[3];
};
#pragma pack(pop)

static string Trim_CString(const char* raw, size_t maxLength)
{
    size_t length = 0;
    while (length < maxLength && raw[length] != '\0')
        ++length;

    return string(raw, raw + length);
}

Collision_SurfaceCache::EEntryType Collision_SurfaceCache::Parse_Type(uint32 rawType)
{
    switch (rawType)
    {
    case 0: return EEntryType::Walkable;
    case 1: return EEntryType::WallRun;
    case 2: return EEntryType::WorldBlock;
    default: return EEntryType::Unknown;
    }
}

const char* Collision_SurfaceCache::ToString(EEntryType type)
{
    switch (type)
    {
    case EEntryType::Walkable:   return "Walkable";
    case EEntryType::WallRun:    return "WallRun";
    case EEntryType::WorldBlock: return "WorldBlock";
    default:                     return "Unknown";
    }
}

HRESULT Collision_SurfaceCache::Load(const fs::path& filePath)
{
    Clear();

    std::ifstream input(filePath, std::ios_base::binary);
    if (!input.is_open())
        return E_FAIL;

    FCollision_SurfaceCacheFileHeader header{};
    input.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!input.good())
        return E_FAIL;

    if (memcmp(header.magic, "CSC1", 4) != 0)
        return E_FAIL;

    if (header.version != 1)
        return E_FAIL;

    _entries.reserve(header.recordCount);

    for (uint32 i = 0; i < header.recordCount; ++i)
    {
        FCollision_SurfaceCacheFileRecord record{};
        input.read(reinterpret_cast<char*>(&record), sizeof(record));

        if (!input.good())
            return E_FAIL;

        FEntry entry{};
        entry.type = Parse_Type(record.proxyType);
        entry.staticClass = Trim_CString(record.staticClass, sizeof(record.staticClass));
        entry.modelGuid = Trim_CString(record.modelGuid, sizeof(record.modelGuid));
        entry.position = Vec3(record.position[0], record.position[1], record.position[2]);
        entry.rotation = Vec3(record.rotation[0], record.rotation[1], record.rotation[2]);
        entry.scale = Vec3(record.scale[0], record.scale[1], record.scale[2]);

        _entries.push_back(entry);
    }

    return S_OK;
}

void Collision_SurfaceCache::Clear()
{
    _entries.clear();
    _grid.clear();
    _cellSize = 0.f;
}

Collision_SurfaceCache::FCellCoord Collision_SurfaceCache::Build_CellCoord(const Vec3& worldPosition, float cellSize)
{
    FCellCoord coord{};
    coord.x = static_cast<int32>(floorf(worldPosition.x / cellSize));
    coord.z = static_cast<int32>(floorf(worldPosition.z / cellSize));
    return coord;
}

void Collision_SurfaceCache::Build_GridIndex(float cellSize)
{
    _grid.clear();
    _cellSize = cellSize;

    if (_cellSize <= 0.f)
        return;

    for (uint32 i = 0; i < static_cast<uint32>(_entries.size()); ++i)
    {
        const FEntry& entry = _entries[i];
        const FCellCoord cell = Build_CellCoord(entry.position, _cellSize);
        _grid[cell].push_back(i);
    }
}

size_t Collision_SurfaceCache::Get_Count(EEntryType type) const
{
    size_t count = 0;

    for (const auto& entry : _entries)
    {
        if (entry.type == type)
            ++count;
    }

    return count;
}

Unique<Collision_SurfaceCache> Collision_SurfaceCache::Create()
{
    return Unique<Collision_SurfaceCache>(new Collision_SurfaceCache());
}

void Collision_SurfaceCache::Free()
{
    Base::Free();
}
