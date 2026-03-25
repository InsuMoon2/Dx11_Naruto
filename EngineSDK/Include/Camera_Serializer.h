#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Camera_Serializer : public Base
{
public:
    static json To_Json(const FCameraSequenceAsset& asset);
    static bool From_Json(const json& root, FCameraSequenceAsset& outAsset);

    static bool Save_ToFile(const wstring& filePath, const FCameraSequenceAsset& asset);
    static bool Load_FromFile(const wstring& filePath, FCameraSequenceAsset& outAsset);

    static fs::path Get_BaseFolderPath();
};

NS_END
