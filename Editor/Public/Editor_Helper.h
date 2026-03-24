#pragma once

class Editor_Helper
{
public:
    Editor_Helper() = default;
    ~Editor_Helper() = default;

public:
    static EAssetOpenType   ClassifyAsset(const fs::path& path);
    static void             Open_Asset(EAssetOpenType assetType, const wstring& filePath, const string& pureName);
    static bool             IsEditorManagedAsset(EAssetOpenType assetType);

    // 애니메이션 표시용 이름
    static string           Build_AnimatoinDisplayName(const string& rawClipName);

    // 검색은 원본 이름 + 표시 이름 둘 다 
    static bool             Passes_AnimationDisplayFilter(const string& rawClipName, const string& filterText);
};

