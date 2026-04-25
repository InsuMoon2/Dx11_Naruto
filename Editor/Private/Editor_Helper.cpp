#include "pch.h"
#include "Editor_Helper.h"

#include "BehaviorTree_View.h"
#include "Effect_View.h"
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

    if (fullPath.ends_with(".effect.json"))
        return EAssetOpenType::Effect;

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
    case EAssetOpenType::Effect:
    {
        auto effectView = dynamic_pointer_cast<Effect_View>(EDITOR->Get_Window(TEXT("Effect View")));
        if (effectView)
        {
            effectView->Set_Active(true);
            effectView->Load_EffectFile(fullPath);
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
        || assetType == EAssetOpenType::UIAnimation
        || assetType == EAssetOpenType::Effect;
}

string Editor_Helper::Build_AnimatoinDisplayName(const string& rawClipName)
{
    string displayName = rawClipName;

    // ex) "SK_CHR_NormalModel|CustomMan_Jump_Vertical" -> "CustomMan_Jump_Vertical"
    const size_t barPos = displayName.rfind('|');
    if (barPos != string::npos)
        displayName = displayName.substr(barPos + 1);

    const size_t firstCharPos = displayName.find_first_not_of(" \t");
    if (firstCharPos != string::npos)
        displayName = displayName.substr(firstCharPos);

    static const string prefix = "CustomMan_";
    if (displayName.rfind(prefix, 0) == 0)
        displayName = displayName.substr(prefix.size());

    return displayName.empty() ? rawClipName : displayName;
}

bool Editor_Helper::Passes_AnimationDisplayFilter(const string& rawClipName, const string& filterText)
{
    if (filterText.empty())
        return true;

    const string filterLower = Utils::ToLowerCopy(filterText);
    const string rawLower = Utils::ToLowerCopy(rawClipName);
    const string displayLower = Utils::ToLowerCopy(Build_AnimatoinDisplayName(rawClipName));

    // 원본 이름으로도 검색되고, 접두사 제거된 표시 이름으로도 검색되게
    return rawLower.find(filterLower) != string::npos
        || displayLower.find(filterLower) != string::npos;
}
