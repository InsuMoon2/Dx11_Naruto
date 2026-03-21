#include "pch.h"
#include "Level_Equipment.h"
#include "UI_Text.h"
#include "Background.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "UI_MainTitleMenuButton.h"

Level_Equipment::Level_Equipment(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Equipment::~Level_Equipment()
{
}

HRESULT Level_Equipment::Initialize()
{
    if (FAILED(Ready_Layer_UI()))
        return E_FAIL;


    return S_OK;
}

void Level_Equipment::Update(float timeDelta)
{
    Level::Update(timeDelta);


    
}

void Level_Equipment::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);


}

HRESULT Level_Equipment::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : 파츠 선택"));
    #endif
    

    return S_OK;
}

HRESULT Level_Equipment::Ready_Layer_UI()
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
        desc.textureIndex = 0; // 이거 바꿔야함. 일단 텍스트 가져와야한다.

        desc.zOrder = 0.5f;
        
        auto mainTitle = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::Overlay,
                &desc));
        if (!mainTitle) return E_FAIL;
    }

    return S_OK;
}

Shared<Level_Equipment> Level_Equipment::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Equipment>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Equipment");

        return nullptr;
    }

    return instance;
}

void Level_Equipment::Free()
{
    Level::Free();

}

