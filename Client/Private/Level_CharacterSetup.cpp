#include "pch.h"
#include "Level_CharacterSetup.h"
#include "UI_Text.h"
#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "UI_MainTitleMenuButton.h"
#include "UI_TabButton.h"

Level_CharacterSetup::Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_CharacterSetup::~Level_CharacterSetup()
{
}

HRESULT Level_CharacterSetup::Initialize()
{
    if (FAILED(Ready_Layer_UI()))
        return E_FAIL;


    return S_OK;
}

void Level_CharacterSetup::Update(float timeDelta)
{
    Level::Update(timeDelta);


    
}

void Level_CharacterSetup::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);


}

HRESULT Level_CharacterSetup::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : 파츠 선택"));
    #endif
    

    return S_OK;
}

HRESULT Level_CharacterSetup::Ready_Layer_UI()
{
    Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    // Background
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("CharacterSetup_Background");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.5f;
        desc.sizeX = viewport.x;
        desc.sizeY = viewport.y;

        desc.levelIndex = ETOI(ELevelType::CharacterSetup);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
        desc.textureIndex = ETOI(ECharacterSetupTexture::Background);

        desc.zOrder = 0.5f;
        
        auto mainTitle = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::HUD,
                &desc));

        if (!mainTitle)
            return E_FAIL;
    }

    // Window
    Background::FBackgroundDesc windowDesc{};
    windowDesc.name = TEXT("Setup_Window");
    windowDesc.posX = viewport.x * 0.3f - 60.f;
    windowDesc.posY = viewport.y * 0.5f;
    windowDesc.sizeX = 824.f * 0.8f;
    windowDesc.sizeY = 624.f * 0.8f;

    windowDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    windowDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    windowDesc.textureIndex = ETOI(ECharacterSetupTexture::Window);

    windowDesc.zOrder = 0.51f;

    auto window = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &windowDesc));

    if (!window)
        return E_FAIL;

    // WinTitle
    Background::FBackgroundDesc wintitleDesc{};
    wintitleDesc.name = TEXT("WinTitle");
    wintitleDesc.posX = windowDesc.posX;
    wintitleDesc.posY = windowDesc.posY - 200.f;
    wintitleDesc.sizeX = 792.f * 0.8f;
    wintitleDesc.sizeY = 62.f;

    wintitleDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    wintitleDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    wintitleDesc.textureIndex = ETOI(ECharacterSetupTexture::WinTitle);

    wintitleDesc.zOrder = 0.51f;

    auto winTitle = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &wintitleDesc));

    if (!winTitle)
        return E_FAIL;

    // Menu Button
    {
        const array<wstring, 6> menuLabels =
        {
            L"머리",
            L"얼굴장식",
            L"한벌옷",
            L"상의",
            L"하의",
            L"악세사리"
        };

        const float spacing = 15.f;
        const float tabWidth = 718.f * 0.75f;
        const float tabHeight = 64.f * 0.8f;

        for (int32 i = 0; i < 5; ++i)
        {
            UI_TabButton::FUITabDesc tabDesc{};
            tabDesc.name = ::format(L"TabButton {}", i);
            tabDesc.posX = windowDesc.posX;
            tabDesc.posY = windowDesc.posY + i * (tabHeight + spacing) - 140.f;
            tabDesc.sizeX = tabWidth;
            tabDesc.sizeY = tabHeight;

            tabDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
            tabDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
            tabDesc.textureIndex = ETOI(ECharacterSetupTexture::SelectButton);

            tabDesc.zOrder = 0.52f + i * 0.001f;

            tabDesc.labelText = menuLabels[i];
            tabDesc.labelOffset = Vec2(0.f, 0.f);
            tabDesc.labelSize = Vec2(400.f, 60.f);
            tabDesc.fontSize = 24.f;

            _tabButton[i] = static_pointer_cast<UI_TabButton>(
                GAME->Add_UI(
                    Protocol::OBJECT_TYPE_UI_TAB,
                    EUILayer::HUD,
                    &tabDesc));

            CHECK_NULL(_tabButton[i], E_FAIL);
        }
    }

    // Select Button


    // Select Desc


    // Title Bg


    // Title Symbol





    return S_OK;
}

Shared<Level_CharacterSetup> Level_CharacterSetup::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_CharacterSetup>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_CharacterSetup");

        return nullptr;
    }

    return instance;
}

void Level_CharacterSetup::Free()
{
    Level::Free();

}

