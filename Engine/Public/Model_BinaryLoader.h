#pragma once

#include "Base.h"
#include <fstream>

NS_BEGIN(Engine)

class ENGINE_DLL Model_BinaryLoader final
{
public:
    static bool Load(const string& filePath, FModelBinaryData& outData);

private:
    template<typename T>
    static bool Read_Value(ifstream& file, T& outValue)
    {
        file.read(reinterpret_cast<char*>(&outValue), sizeof(T));
        return file.good();
    }

    static bool Read_Bytes(ifstream& file, void* dst, size_t size);
    static bool Read_String(ifstream& file, string& outValue);

    static bool Read_V1_Static(ifstream& file, const string& filePath,
        const FStaticMeshFileHeader& header, FModelBinaryData& outData);

    static bool Read_V2_Skeletal(ifstream& file, const string& filePath,
        const FSkeletalMeshFileHeader& header, FModelBinaryData& outData);
};

NS_END
