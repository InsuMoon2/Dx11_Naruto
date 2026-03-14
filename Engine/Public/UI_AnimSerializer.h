#pragma once

#include "UI_AnimTypes.h"

NS_BEGIN(Engine)

class ENGINE_DLL UIAnimSerializer
{

private:
    static string Property_ToString(EUIAnimProperty property);
    static bool String_ToProperty(const string& text, EUIAnimProperty& outProperty);

    static json Key_ToJson(const FUIAnimKey& key);
    static bool Json_ToKey(const json& j, FUIAnimKey& outKey);

    static json Track_ToJson(const FUIAnimTrack& track);
    static bool Json_ToTrack(const json& j, FUIAnimTrack& outTrack);

    static void Normalize_Asset(FUIAnimAsset& asset);
    static void Normalize_Track(FUIAnimTrack& track);

public:
    static json To_Json(const FUIAnimAsset& asset);
    static bool From_Json(const json& root, FUIAnimAsset& outAsset);

    static Shared<FUIAnimAsset> Load_FromFile(const wstring& fullPath);
    static bool Save_ToFile(const wstring& fullPath, const FUIAnimAsset& asset);
};

NS_END
