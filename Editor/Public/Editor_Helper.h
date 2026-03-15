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

};

