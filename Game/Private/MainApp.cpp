#include "pch.h"
#include "MainApp.h"

#include "Background.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "NetworkManager.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "Texture.h"

MainApp::MainApp()
{
}

MainApp::~MainApp()
{

}

HRESULT MainApp::Initialize()
{
    // Engine Setting
    {
        ENGINE_DESC engineDesc = {};
        engineDesc.hWnd = g_hWnd;
        engineDesc.winMode = EWinMode::Win;
        engineDesc.viewportWidth = g_winSizeX;
        engineDesc.viewportHeight = g_winSizeY;
        engineDesc.numLevels = ETOI(ELevelType::END);

        if (FAILED(GAME->Initialize_Engine(engineDesc, _device, _context)))
            return E_FAIL;
    }

    CHECK_FAILED(Ready_StaticLevel(), E_FAIL);
    CHECK_FAILED(Ready_StartLevel(ELevelType::MainTitle), E_FAIL);

    GAME->Set_GameState(EGameState::Play);

    return S_OK;
}

void MainApp::Priority_Update(float timeDelta)
{
    GAME->Priority_Update_Engine(timeDelta);
}

void MainApp::Update(float timeDelta)
{
    INPUT->Update(timeDelta);

    GAME->Update_Engine(timeDelta);

    if (!_networkConnected && GAME->Current_Level() == ETOI(ELevelType::GamePlay))
    {
        NetworkManager::GetInstance()->Initialize();
        _networkConnected = true;
    }

    NetworkManager::GetInstance()->Update();
}

void MainApp::Late_Update(float timeDelta)
{
    GAME->Late_Update_Engine(timeDelta);
}

HRESULT MainApp::Render()
{
    Color clearColor = { 0.1f, 0.3f, 1.0f, 1.f };

    CHECK_FAILED(GAME->Clear_Buffers(clearColor), E_FAIL);
    CHECK_FAILED(GAME->Draw(), E_FAIL);
    CHECK_FAILED(GAME->Present(), E_FAIL);

    return S_OK;
}

HRESULT MainApp::Ready_StaticLevel()
{
    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_SHADER_VTXTEX,
        Shader::Create(_device, _context, TEXT("../../Client/Bin/Shaders/Shader_Vtxtex.hlsl"),
            VTXTEX::Elements, VTXTEX::numElements))))
    {
        return E_FAIL;
    }

    shared_ptr<Texture> loadingTex = Texture::Create(_device, _context, TEXT("../../Client/Bin/Resources/Textures/UI/Loading_Screen/Textures/T_UI_LoadingScreen_%03d_BC.png"), 2);
    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_TEXTURE_LOADING, loadingTex)))
    {
        return E_FAIL;
    }

    if (FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static),
        Protocol::COMPONENT_TYPE_RECT, VIBuffer_Rect::Create(_device, _context))))
    {
        return E_FAIL;
    }

    if (FAILED(GAME->Add_GameObject_Prototype(ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_BACKGROUND, Background::Create(_device, _context))))
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT MainApp::Ready_StartLevel(ELevelType startLevelID)
{
    if (ELevelType::Loading == startLevelID)
        return E_FAIL;
    
    if (FAILED(GAME->Change_Level(ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, startLevelID))))
        return E_FAIL;

    return S_OK;
}

unique_ptr<MainApp> MainApp::Create()
{
    auto instance = make_unique<MainApp>();

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Created : MainApp");

        return nullptr;
    }

    return instance;
}

void MainApp::Free()
{
    Base::Free();

	// Network
	{
		NetworkManager::GetInstance()->Free();
		NetworkManager::DestroyInstance();
	}

    GameInstance::DestroyInstance();

}
