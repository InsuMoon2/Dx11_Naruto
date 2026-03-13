#include "pch.h"
#include "Level_MainTitle.h"

#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"

Level_MainTitle::Level_MainTitle(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_MainTitle::~Level_MainTitle()
{
}

HRESULT Level_MainTitle::Initialize()
{
    if (FAILED(Ready_Layer_Background()))
        return E_FAIL;

    

    return S_OK;
}

void Level_MainTitle::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        GAME->Change_Level(ETOI(ELevelType::Loading),
            Level_Loading::Create(_device, _context, ELevelType::GamePlay, false));
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

HRESULT Level_MainTitle::Ready_Layer_Background()
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
        desc.textureIndex = ETOI(EMainTitle::BG_0);

        desc.zOrder = 0.5f;
        
        auto mainTitle = Background::Create(_device, _context, &desc);
        if (!mainTitle) return E_FAIL;
        GAME->Add_UI_ToLayer(EUILayer::Overlay, mainTitle);
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
        desc.textureIndex = ETOI(EMainTitle::Logo);

        desc.zOrder = 0.51f;

        auto logo = Background::Create(_device, _context, &desc);
        if (!logo) return E_FAIL;
        GAME->Add_UI_ToLayer(EUILayer::Overlay, logo);
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
        desc.textureIndex = ETOI(EMainTitle::Text0);

        desc.zOrder = 0.51f;

        auto text = Background::Create(_device, _context, &desc);
        if (!text) return E_FAIL;
        GAME->Add_UI_ToLayer(EUILayer::Overlay, text);
    }

    return S_OK;
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

