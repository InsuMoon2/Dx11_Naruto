#include "pch.h"
#include "Editor_Helper.h"

#include "BehaviorTree_View.h"
#include "Prefab_View.h"
#include "UI_Animation_View.h"
#include "Utils.h"
#include <shellapi.h>

EAssetOpenType Editor_Helper::ClassifyAsset(const fs::path& path)
{
    string fullPath = Utils::ToLowerCopy(path.string());
    string extension = Utils::ToLowerCopy(path.extension().string());

    if (fullPath.ends_with(".prefab.json"))
        return EAssetOpenType::Prefab;

    if (fullPath.ends_with(".bt.json"))
        return EAssetOpenType::BehaviorTree;

    if (fullPath.ends_with(".uianim.json"))
        return EAssetOpenType::UIAnimation;

    if (extension == ".fbx" || extension == ".gltf")
        return EAssetOpenType::Mesh;

    if (extension == ".png" || extension == ".jpg" ||
        extension == ".jpeg" || extension == ".dds")
        return EAssetOpenType::Texture;

    if (extension == ".csv")
        return EAssetOpenType::Csv;

    return EAssetOpenType::None;
}

void Editor_Helper::Open_Asset(EAssetOpenType assetType, const wstring& filePath, const string& pureName)
{
    string fullPath = Utils::ToString(filePath);

    switch (assetType)
    {
    case EAssetOpenType::Prefab:
    {
        auto prefabView = dynamic_pointer_cast<Prefab_View>(EDITOR->Get_Window(TEXT("Prefab")));
        if (prefabView)
        {
            prefabView->Set_Active(true);
            prefabView->Open_Prefab(pureName, fullPath);
        }
        break;
    }
    case EAssetOpenType::BehaviorTree:
    {
        auto behaviorView = dynamic_pointer_cast<BehaviorTree_View>(EDITOR->Get_Window(TEXT("BehaviorTree")));
        if (behaviorView)
        {
            behaviorView->Set_Active(true);
            behaviorView->Load_BehaviorTree(fullPath);
        }
        break;
    }
    case EAssetOpenType::UIAnimation:
    {
        auto animView = dynamic_pointer_cast<UI_Animation_View>(EDITOR->Get_Window(TEXT("UI Animation")));
        if (animView)
        {
            animView->Set_Active(true);
            animView->Load_Animation(fullPath);
        }
        break;
    }
    default:
        ShellExecute(nullptr, L"open", filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    }
}

bool Editor_Helper::IsEditorManagedAsset(EAssetOpenType assetType)
{
    // 색상처리
    return assetType == EAssetOpenType::Prefab
        || assetType == EAssetOpenType::BehaviorTree 
        || assetType == EAssetOpenType::UIAnimation;
}
