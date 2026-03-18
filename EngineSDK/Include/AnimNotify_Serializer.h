#pragma once

#include "Base.h"
#include "AnimNotify_Types.h"

NS_BEGIN(Engine)

// 파일 저장 / 로드만 담당
class ENGINE_DLL AnimNotify_Serializer final
{
public:
    static json To_Json(const FAnimNotifyAsset& asset);
    static bool From_Json(const json& root, FAnimNotifyAsset& outAsset);

    static bool Save_ToFile(const wstring& filePath, const FAnimNotifyAsset& asset);
    static bool Load_FromFile(const wstring& filePath, FAnimNotifyAsset& outAsset);

    static fs::path Get_BaseFolderPath();
    static fs::path Get_ModelNotifyFilePath(const string& modelGuid);

    static FAnimNotifyClipData*         Find_Clip(FAnimNotifyAsset& asset, const string& clipName);
    static const FAnimNotifyClipData*   Find_Clip(const FAnimNotifyAsset& asset, const string& clipName);
    static FAnimNotifyClipData&         Get_OrAddClip(FAnimNotifyAsset& asset, const string& clipName);
};

NS_END

