#include "pch.h"
#include "UI_Manager.h"
#include "UIObject.h"
#include "UI_AnimPlayer.h"
#include "UI_AnimSerializer.h"
#include "Utils.h"

UI_Manager::UI_Manager()
{
}

HRESULT UI_Manager::Initialize()
{
    return S_OK;
}

void UI_Manager::Priority_Update(float timeDelta)
{
    for (uint32_t i = 0; i < ETOI(EUILayer::END); ++i)
    {
        for (auto& ui : _uiLayers[i])
        {
            if (ui->Is_Visibility())
                ui->Priority_Update(timeDelta);
        }
    }
}

void UI_Manager::Update(float timeDelta)
{
    for (uint32_t i = 0; i < ETOI(EUILayer::END); ++i)
    {
        for (auto& ui : _uiLayers[i])
        {
            if (ui->Is_Visibility())
                ui->Update(timeDelta);
        }
    }

    Update_UIAnimations(timeDelta);
}

void UI_Manager::Late_Update(float timeDelta)
{
    for (uint32_t i = 0; i < ETOI(EUILayer::END); ++i)
    {
        for (auto& ui : _uiLayers[i])
        {
            if (ui->Is_Visibility())
            {
                ui->Late_Update(timeDelta);

                GAME->Add_RenderGroup(ui->Get_RenderGroup(), ui);
            }
        }
    }
}

Shared<UIObject> UI_Manager::Add_UI(uint32 objID, EUILayer layer, void* arg)
{
    auto uiObject = Clone_UI(objID, arg);
    CHECK_NULL(uiObject, nullptr);

    if (FAILED(Register_UI(layer, uiObject)))
        return nullptr;

    return uiObject;
}

Shared<UIObject> UI_Manager::Clone_UI(uint32 objID, void* arg)
{
    const uint32 protoLevelIndex = Get_UIPrototypeLevel();
    assert(protoLevelIndex != static_cast<uint32>(-1));

    auto uiObject = dynamic_pointer_cast<UIObject>(
        GAME->Clone_GameObject(protoLevelIndex, objID, arg));

    CHECK_NULL(uiObject, nullptr);

    return uiObject;
}

HRESULT UI_Manager::Register_UI(EUILayer layer, Shared<UIObject> uiObject)
{
    CHECK_NULL(uiObject, E_FAIL);

    const wstring uiName = uiObject->Get_Name();
    if (!uiName.empty())
    {
        if (_uiMap.find(uiName) != _uiMap.end())
            return E_FAIL;

        _uiMap.emplace(uiName, uiObject);
    }

    uiObject->Set_UILayer(layer);

    CHECK_FAILED(uiObject->On_UIRegistered(), E_FAIL);

    _uiLayers[ETOI(layer)].push_back(uiObject);
    return S_OK;
}

Shared<UIObject> UI_Manager::Find_UI(const wstring& name)
{
    auto iter = _uiMap.find(name);
    if (iter == _uiMap.end())
        return nullptr;

    return iter->second;
}

void UI_Manager::Show_UI(const wstring& name)
{
    if (auto ui = Find_UI(name))
    {
        ui->Set_Visibility(true);
    }
}

void UI_Manager::Hide_UI(const wstring& name)
{
    if (auto ui = Find_UI(name))
    {
        ui->Set_Visibility(false);
    }
}

void UI_Manager::Toggle_UI(const wstring& name)
{
    if (auto ui = Find_UI(name))
    {
        ui->Set_Visibility(!ui->Is_Visibility());
    }
}

void UI_Manager::Hide_All_Layer(EUILayer layer)
{
    for (auto& ui : _uiLayers[ETOI(layer)])
    {
        if (ui)
        {
            ui->Set_Visibility(false);
        }
    }
}

void UI_Manager::Hide_All_UI()
{
    for (uint32 i = 0; i < ETOI(EUILayer::END); ++i)
    {
        for (auto& ui : _uiLayers[i])
        {
            if (ui) ui->Set_Visibility(false);
        }
    }
}

void UI_Manager::Clear_All_UI()
{
    for (uint32 i = 0; i < ETOI(EUILayer::END); ++i)
    {
        _uiLayers[i].clear();
    }

    _uiMap.clear();

    Clear_UIAnimations();
}

void UI_Manager::Clear_UI_ByLevel(uint32 levelIndex)
{
    auto iter = _uiMap.begin();
    while (iter != _uiMap.end())
    {
        if (iter->second->Get_LevelIndex() == levelIndex)
        {
            EUILayer layer = iter->second->Get_UILayer();
            _uiLayers[ETOI(layer)].remove(iter->second);

            iter = _uiMap.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    for (uint32 i = 0; i < ETOI(EUILayer::END); ++i)
    {
        _uiLayers[i].remove_if([levelIndex](const Shared<UIObject>& ui)
            {
                return ui && ui->Get_LevelIndex() == levelIndex;
            });
    }

    Clear_UIAnimations_ByLevel(levelIndex);
}

void UI_Manager::Notify_Viewport_Resize(float widht, float height)
{
    // TODO : 뷰포트 사이즈 변경 알림
}

bool UI_Manager::Is_InputBlocked() const
{
    // Popup 창(인벤, 상점 등)이 하나라도 열려있으면 마우스/키보드 게임 내 이동 막기
    for (auto& ui : _uiLayers[ETOI(EUILayer::Popup)])
    {
        if (ui->Is_Visibility()) return true;
    }
    return false;
}

HRESULT UI_Manager::Remove_UI(const wstring& name)
{
    auto iter = _uiMap.find(name);
    if (iter == _uiMap.end())
        return E_FAIL;

    Shared<UIObject> uiObject = iter->second;
    CHECK_NULL(uiObject, E_FAIL);

    const EUILayer layer = uiObject->Get_UILayer();
    _uiLayers[ETOI(layer)].remove(uiObject);

    _uiMap.erase(iter);

    return S_OK;
}

HRESULT UI_Manager::Remove_UI(const Shared<UIObject>& uiObject)
{
    CHECK_NULL(uiObject, E_FAIL);

    const wstring name = uiObject->Get_Name();
    if (!name.empty())
        return Remove_UI(name);

    const EUILayer layer = uiObject->Get_UILayer();
    _uiLayers[ETOI(layer)].remove(uiObject);

    return S_OK;
}

bool UI_Manager::Play_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    if (!target)
        return false;

    auto asset = Load_UIAnimationAsset(animationName);
    if (!asset)
        return false;

    auto* entry = Find_UIAnimationEntry(target, animationName);

    if (!entry)
    {
        FUIAnimPlaybackEntry newEntry;
        newEntry.target = target;
        newEntry.targetName = target->Get_Name();
        newEntry.animationName = animationName;
        newEntry.asset = asset;
        newEntry.player = UI_AnimPlayer::Create();
        newEntry.player->Set_Asset(asset);
        newEntry.player->Bind_Target(target);

        _uiAnimEntries.push_back(newEntry);
        entry = &_uiAnimEntries.back();
    }
    else
    {
        entry->asset = asset;
        entry->player->Set_Asset(asset);
        entry->player->Bind_Target(target);
    }

    entry->player->Set_CurrentFrame(asset->startFrame);
    entry->player->Play();

    return true;
}

bool UI_Manager::Play_UIAnimation(const wstring& targetName, const string& animationName)
{
    auto target = Find_UI(targetName);
    if (!target)
        return false;

    return Play_UIAnimation(target, animationName);
}

bool UI_Manager::Pause_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    auto* entry = Find_UIAnimationEntry(target, animationName);
    if (!entry)
        return false;

    entry->player->Pause();

    return true;
}

bool UI_Manager::Pause_UIAnimation(const wstring& targetName, const string& animationName)
{
    auto target = Find_UI(targetName);
    if (!target)
        return false;

    return Pause_UIAnimation(target, animationName);
}

bool UI_Manager::Stop_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    auto* entry = Find_UIAnimationEntry(target, animationName);
    if (!entry)
        return false;

    entry->player->Stop();
    return true;
}

bool UI_Manager::Stop_UIAnimation(const wstring& targetName, const string& animationName)
{
    auto target = Find_UI(targetName);
    if (!target)
        return false;

    return Stop_UIAnimation(target, animationName);
}

Shared<FUIAnimAsset> UI_Manager::Load_UIAnimationAsset(const string& animationName)
{
    auto it = _uiAnimAssetCache.find(animationName);
    if (it != _uiAnimAssetCache.end())
        return it->second;

    wstring path = Resolve_UIAnimationPath(animationName);
    if (path.empty())
        return nullptr;

    auto asset = UI_AnimSerializer::Load_FromFile(path);
    if (!asset)
        return nullptr;

    _uiAnimAssetCache.emplace(animationName, asset);
    return asset;
}

wstring UI_Manager::Resolve_UIAnimationPath(const string& animationName) const
{
    auto cacheIt = _uiAnimPathCache.find(animationName);
    if (cacheIt != _uiAnimPathCache.end())
        return cacheIt->second;

    auto assets = GAME->Get_AssetByType("ui_animation");

    for (const auto* meta : assets)
    {
        if (!meta)
            continue;

        if (Extract_UIAnimationName(meta->fullPath) == animationName)
        {
            _uiAnimPathCache.emplace(animationName, meta->fullPath);
            return meta->fullPath;
        }
    }

    return L"";
}

FUIAnimPlaybackEntry* UI_Manager::Find_UIAnimationEntry(const Shared<UIObject>& target, const string& animationName)
{
    if (!target)
        return nullptr;

    for (auto& entry : _uiAnimEntries)
    {
        auto locked = entry.target.lock();
        if (!locked)
            continue;

        if (locked == target && entry.animationName == animationName)
            return &entry;
    }

    return nullptr;
}

void UI_Manager::Update_UIAnimations(float timeDelta)
{
    Remove_ExpiredUIAnimations();

    for (auto& entry : _uiAnimEntries)
    {
        auto target = entry.target.lock();
        if (!target || !entry.player)
            continue;

        entry.player->Update(timeDelta);
    }

}

void UI_Manager::Remove_ExpiredUIAnimations()
{
    _uiAnimEntries.erase(
        remove_if(_uiAnimEntries.begin(), _uiAnimEntries.end(),
            [](const FUIAnimPlaybackEntry& entry)
            {
                return entry.target.expired() || !entry.player;
            }),
        _uiAnimEntries.end());
}

void UI_Manager::Clear_UIAnimations()
{
    _uiAnimEntries.clear();
    _uiAnimAssetCache.clear();
    _uiAnimPathCache.clear();
}

void UI_Manager::Clear_UIAnimations_ByLevel(uint32 levelIndex)
{
    _uiAnimEntries.erase(
        remove_if(_uiAnimEntries.begin(), _uiAnimEntries.end(),
            [levelIndex](const FUIAnimPlaybackEntry& entry)
            {
                auto target = entry.target.lock();
                return !target || target->Get_LevelIndex() == levelIndex;
            }),
        _uiAnimEntries.end());
}

string UI_Manager::Extract_UIAnimationName(const wstring& fullPath)
{
    string filename = fs::path(fullPath).filename().string();

    if (Utils::EndsWidth(filename, ".uianim.json"))
        return filename.substr(0, filename.size() - strlen(".uianim.json"));

    return fs::path(filename).stem().string();
}

Unique<UI_Manager> UI_Manager::Create()
{
    auto instance = make_unique<UI_Manager>();

    if (FAILED(instance->Initialize()))
    {
        assert(false);
        return nullptr;
    }

    return instance;
}

void UI_Manager::Free()
{
    Base::Free();

    Clear_All_UI();
}
