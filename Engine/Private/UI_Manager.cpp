#include "pch.h"
#include "UI_Manager.h"
#include "UIObject.h"

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

                GAME->Add_RenderGroup(ERenderGroup::UI, ui);
            }
        }
    }
}

HRESULT UI_Manager::Add_UI(EUILayer layer, Shared<UIObject> uiObject)
{
    CHECK_NULL(uiObject, E_FAIL);

    wstring uiName = uiObject->Get_Name();

    if (_uiMap.find(uiName) != _uiMap.end())
        return E_FAIL; // 이름 중복

    uiObject->Set_UILayer(layer);

    _uiLayers[ETOI(layer)].push_back(uiObject);
    _uiMap.emplace(uiName, uiObject);

    return S_OK;
}

HRESULT UI_Manager::Add_UI_ToLayer(EUILayer layer, Shared<UIObject> uiObject)
{
    CHECK_NULL(uiObject, E_FAIL);

    uiObject->Set_UILayer(layer);
    
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
