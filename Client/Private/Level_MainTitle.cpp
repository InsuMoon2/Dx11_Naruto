#include "pch.h"
#include "Level_MainTitle.h"
#include "UI_Text.h"
#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "UI_MainTitleMenuButton.h"

Level_MainTitle::Level_MainTitle(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_MainTitle::~Level_MainTitle()
{
}

HRESULT Level_MainTitle::Initialize()
{
    if (FAILED(Ready_Layer_UI()))
        return E_FAIL;

    Apply_TitlePhase();
    Apply_MenuSelection();

    return S_OK;
}

void Level_MainTitle::Update(float timeDelta)
{
    Level::Update(timeDelta);

    switch (_titleState)
    {
    case ETitleState::PressSpace:
        Update_PressPhase();
        break;

    case ETitleState::SelectMenu:
        Update_SelectPhase();
        break;
    }

    
}

void Level_MainTitle::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);


}

HRESULT Level_MainTitle::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : Logo"));
    #endif
    

    return S_OK;
}

HRESULT Level_MainTitle::Ready_Layer_UI()
{
    Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    // MainTitle
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("MainTitle");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.5f;
        desc.sizeX = viewport.x;
        desc.sizeY = viewport.y;

        desc.levelIndex = ETOI(ELevelType::MainTitle);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE;
        desc.textureIndex = ETOI(EMainTitleTexture::BG_0);

        desc.zOrder = 0.5f;
        
        auto mainTitle = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::Overlay,
                &desc));
        if (!mainTitle) return E_FAIL;
    }
    // Logo
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("Logo");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.3f;
        desc.sizeX = desc.posX;
        desc.sizeY = 200.f;

        desc.levelIndex = ETOI(ELevelType::MainTitle);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE;
        desc.textureIndex = ETOI(EMainTitleTexture::Logo);

        desc.zOrder = 0.51f;

        auto logo = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::Overlay,
                &desc));
        if (!logo) return E_FAIL;
    }
    // Text
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("Press Text");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.7f;
        desc.sizeX = desc.posX * 0.8f;
        desc.sizeY = 200.f;

        desc.levelIndex = ETOI(ELevelType::MainTitle);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE;
        desc.textureIndex = ETOI(EMainTitleTexture::PressText0);

        desc.zOrder = 0.51f;

        _pressText = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::Overlay,
                &desc));

        if (!_pressText) return E_FAIL;

        // 테스트용
        //GAME->Play_UIAnimation(_pressText, "PressText");
    }

    // Menu Button
    {
        const array<wstring, 3> menuLabels =
        {
            L"게임 시작",
            L"게임 설정",
            L"게임 종료"
        };

        const float startY = viewport.y * 0.7f;
        const float spacing = 24.f;
        const float menuWidth = 740.f;
        const float menuHeight = 90.f;

        for (int32 i = 0; i < 3; ++i)
        {
            UI_MainTitleMenuButton::FMainTitleMenuDesc desc{};
            desc.name = format(L"Menu Text {}", i);
            desc.posX = viewport.x * 0.5f;
            desc.posY = startY + i * (menuHeight + spacing);
            desc.sizeX = menuWidth;
            desc.sizeY = menuHeight;
            desc.levelIndex = ETOI(ELevelType::MainTitle);
            desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE;
            desc.textureIndex = ETOI(EMainTitleTexture::TitleMenuBtn);
            desc.zOrder = 0.52f + i * 0.001f;

            desc.labelText = menuLabels[i];
            desc.labelOffset = Vec2(0.f, 0.f);
            desc.labelSize = Vec2(400.f, 60.f);
            desc.fontSize = 24.f;

            _menuText[i] = static_pointer_cast<UI_MainTitleMenuButton>(
                GAME->Add_UI(Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT, EUILayer::Overlay, &desc));
            CHECK_NULL(_menuText[i], E_FAIL);

            _menuText[i]->Set_Visibility(false);
        }
    }

    return S_OK;
}

void Level_MainTitle::Update_PressPhase()
{
    if (INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        _titleState = ETitleState::SelectMenu;
        Apply_TitlePhase();
        Apply_MenuSelection();
    }
}

void Level_MainTitle::Update_SelectPhase()
{
    if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
    {
        _selectedIndex = (_selectedIndex + 2) % 3;
        Apply_MenuSelection();
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        _selectedIndex = (_selectedIndex + 1) % 3;
        Apply_MenuSelection();
    }

    if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        Execute_SelectedMenu();
    }
}

void Level_MainTitle::Apply_TitlePhase()
{
    const bool isPressPhase = (_titleState == ETitleState::PressSpace);
    const bool isSelectPhase = (_titleState == ETitleState::SelectMenu);

    if (_pressText)
        _pressText->Set_Visibility(isPressPhase);

    for (auto& menu : _menuText)
    {
        if (menu)
            menu->Set_Visibility(isSelectPhase);
    }
}

void Level_MainTitle::Apply_MenuSelection()
{
    for (int32 i = 0; i < 3; ++i)
    {
        if (_menuText[i])
        {
            _menuText[i]->Set_Selected(i == _selectedIndex);

            GAME->Play_UIAnimation(_menuText[i], "MenuButton");
        }
            
    }
}

void Level_MainTitle::Execute_SelectedMenu()
{
    switch (_selectedIndex)
    {
    case 0: // 게임 시작
        GAME->Change_Level(
            ETOI(ELevelType::Loading),
            Level_Loading::Create(_device, _context, ELevelType::GamePlay, true));
        break;

    case 1: // 일단은, CharacterSetup
        GAME->Change_Level(
            ETOI(ELevelType::Loading),
            Level_Loading::Create(_device, _context, ELevelType::CharacterSetup, true));
        break;

    case 2: // 게임종료는 실제 실행 ㄴ
        //PostQuitMessage(0);
        break;
    }
}

shared_ptr<Level_MainTitle> Level_MainTitle::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_MainTitle>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_MainTitle");

        return nullptr;
    }

    return instance;
}

void Level_MainTitle::Free()
{
    Level::Free();

}

