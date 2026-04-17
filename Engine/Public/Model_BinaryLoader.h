#pragma once

#include "Base.h"
#include <fstream>

NS_BEGIN(Engine)

class Animation;

class ENGINE_DLL Model_BinaryLoader final
{
public:
    static bool Load(const string& filePath, FModelBinaryData& outData);

    // 애니메이션만 추출, 커스텀 애니메이션
    bool Load_AnimationOnly(const string& filePath, vector<Shared<Animation>>& outAnimations);

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

    // [추가] UV1이 포함된 최신 정적 meshbin(v3)을 읽는다.
    static bool Read_V3_Static(ifstream& file, const string& filePath,
        const FStaticMeshFileHeader& header, FModelBinaryData& outData);

    static bool Read_V2_Skeletal(ifstream& file, const string& filePath,
        const FSkeletalMeshFileHeader& header, FModelBinaryData& outData);
};

NS_END
