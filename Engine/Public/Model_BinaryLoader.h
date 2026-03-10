#pragma once

#include "Base.h"
#include <fstream>

NS_BEGIN(Engine)

struct FMeshFileHeader
{
    uint32 magic = 0;
    uint32 version = 0;
    uint32 meshCount = 0;
    uint32 materialCount = 0;
    uint32 reserved = 0;
};

struct FMeshVertexRaw
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 0.f, nz = 0.f;
    float tx = 0.f, ty = 0.f, tz = 0.f;
    float u = 0.f, v = 0.f;
};

struct FMeshBinaryData
{
    string                  name;
    uint32                  materialIndex = 0;
    vector<FMeshVertexRaw>  vertices;
    vector<uint32>          indices;
};

class ENGINE_DLL Model_BinaryLoader final
{
public:
    static bool Load(const string& filePath, vector<FMeshBinaryData>& outMeshes, uint32& outMaterialCount);

private:
    template<typename T>
    static bool Read_Value(ifstream& file, T& outValue)
    {
        file.read(reinterpret_cast<char*>(&outValue), sizeof(T));
        return file.good();
    }

    static bool Validate_Header(const FMeshFileHeader& header, const string& filePath);
    static bool Read_Header(ifstream& file, const string& filePath, FMeshFileHeader& outHeader);
    static bool Read_MeshData(ifstream& file, FMeshBinaryData& outMeshData);
    static bool Read_Bytes(ifstream& file, void* dst, size_t size);
    static bool Read_String(ifstream& file, string& outValue);
};

NS_END
