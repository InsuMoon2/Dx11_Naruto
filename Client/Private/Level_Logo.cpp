#include "pch.h"
#include "Level_Logo.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"

Level_Logo::Level_Logo(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

Level_Logo::~Level_Logo()
{
}

HRESULT Level_Logo::Initialize()
{
    if (FAILED(Ready_Layer_Background(TEXT("Layer_Background"))))
        return E_FAIL;

    

    return S_OK;
}

void Level_Logo::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (GetKeyState(VK_SPACE) & 0x8000)
    {
        GAME->Change_Level(ETOI(LevelType::Loading), Level_Loading::Create(_device, _context, LevelType::GamePlay));
    }
}

void Level_Logo::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);

    if (INPUT->KeyDown(KEY_TYPE::KEY_1))
    {
        LOG_INFO("Info Test");
    }

    if (INPUT->KeyDown(KEY_TYPE::KEY_2))
    {
        LOG_WARN("Warning Test");
    }

    if (INPUT->KeyDown(KEY_TYPE::KEY_3))
    {
        LOG_ERROR("Error Test");
    }

}

HRESULT Level_Logo::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : Logo"));
    #endif
    

    return S_OK;
}

HRESULT Level_Logo::Ready_Layer_Background(const wstring& layerTag)
{
    if (FAILED(GAME->Add_GameObject(ETOI(LevelType::Logo), TEXT("Prototype_Background"),
        ETOI(LevelType::Logo), layerTag)))
        return E_FAIL;

    return S_OK;
}

shared_ptr<Level_Logo> Level_Logo::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_Logo>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_Logo");

        return nullptr;
    }

    return instance;
}

void Level_Logo::Free()
{
    Level::Free();

}

