#include "pch.h"
#include "Model_BinaryLoader.h"

#include <fstream>

namespace
{
    constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
    constexpr uint32 MESHBIN_VERSION = 1;
}

bool Model_BinaryLoader::Validate_Header(const FMeshFileHeader& header, const string& filePath)
{
    if (header.magic != MESHBIN_MAGIC)
    {
        LOG_ERROR("Invalid meshbin magic: {}", filePath);
        return false;
    }

    if (header.version != MESHBIN_VERSION)
    {
        LOG_ERROR("Unsupported meshbin version: {}", filePath);
        return false;
    }

    return true;
}

bool Model_BinaryLoader::Read_Header(ifstream& file, const string& filePath, FMeshFileHeader& outHeader)
{
    if (!Read_Value(file, outHeader))
    {
        LOG_ERROR("Failed to read meshbin header: {}", filePath);
        return false;
    }

    return Validate_Header(outHeader, filePath);
}

bool Model_BinaryLoader::Read_MeshData(ifstream& file, FMeshBinaryData& outMeshData)
{
    if (!Read_String(file, outMeshData.name))
        return false;

    if (!Read_Value(file, outMeshData.materialIndex))
        return false;

    uint32 vertexCount = 0;
    if (!Read_Value(file, vertexCount))
        return false;

    outMeshData.vertices.resize(vertexCount);
    if (!Read_Bytes(file, outMeshData.vertices.data(),
        sizeof(FMeshVertexRaw) * outMeshData.vertices.size()))
    {
        return false;
    }

    uint32 indexCount = 0;
    if (!Read_Value(file, indexCount))
        return false;

    outMeshData.indices.resize(indexCount);
    if (!Read_Bytes(file, outMeshData.indices.data(),
        sizeof(uint32) * outMeshData.indices.size()))
    {
        return false;
    }

    return true;
}

bool Model_BinaryLoader::Read_Bytes(ifstream& file, void* dst, size_t size)
{
    if (size == 0)
        return true;

    file.read(reinterpret_cast<char*>(dst), static_cast<streamsize>(size));
    return file.good();
}

bool Model_BinaryLoader::Read_String(ifstream& file, string& outValue)
{
    uint32 length = 0;
    if (!Read_Value(file, length))
        return false;

    outValue.clear();
    outValue.resize(length);

    if (length == 0)
        return true;

    return Read_Bytes(file, outValue.data(), length);
}

bool Model_BinaryLoader::Load(const string& filePath, vector<FMeshBinaryData>& outMeshes, uint32& outMaterialCount)
{
    outMeshes.clear();
    outMaterialCount = 0;

    ifstream file(filePath, ios_base::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open meshbin: {}", filePath);
        return false;
    }

    FMeshFileHeader header{};
    if (!Read_Header(file, filePath, header))
        return false;

    outMaterialCount = header.materialCount;
    outMeshes.reserve(header.meshCount);

    for (uint32 meshIndex = 0; meshIndex < header.meshCount; ++meshIndex)
    {
        FMeshBinaryData meshData;
        if (!Read_MeshData(file, meshData))
        {
            LOG_ERROR("Failed to read mesh data: {} (mesh index: {})", filePath, meshIndex);
            return false;
        }

        outMeshes.push_back(move(meshData));
    }

    return true;
}
