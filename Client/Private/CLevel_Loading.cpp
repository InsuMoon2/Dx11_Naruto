#include "pch.h"
#include "CLevel_Loading.h"
#include "CLoader.h"
#include "CGameInstance.h"
#include "CLevel_Gameplay.h"
#include "CLevel_Logo.h"

CLevel_Loading::CLevel_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : CLevel { device, context }
{
}

CLevel_Loading::~CLevel_Loading()
{
}

HRESULT CLevel_Loading::Initialize(LEVEL nextLevelID)
{
    _nextLevelID = nextLevelID;

    if (FAILED(Ready_Layer_Background(TEXT("Layer_Background"))))
        return E_FAIL;

    if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
        return E_FAIL;

    // 로더 생성
    _loader = CLoader::Create(_device, _context, nextLevelID);
    NULL_CHECK_RETURN(_loader, E_FAIL);



    return S_OK;
}

void CLevel_Loading::Update(float timeDelta)
{
    if (_loader->IsFinished() && GetKeyState(VK_RETURN))
    {
        shared_ptr<CLevel> nextLevel = { nullptr };

        switch (_nextLevelID)
        {
        case LEVEL::LOGO:
            nextLevel = CLevel_Logo::Create(_device, _context);
            break;

        case LEVEL::GAMEPLAY:
            nextLevel = CLevel_Gameplay::Create(_device, _context);
            break;
        }

        if (nextLevel == nullptr)
        {
            MSG_BOX("Failed to Created : NextLevel");
            return;
        }

        if (FAILED(GAME->Change_Level(ETOI(_nextLevelID), nextLevel)))
        {
            MSG_BOX("Failed to Change : NextLevel");
            return;
        }

        return;
    }
}

void CLevel_Loading::LateUpdate(float timeDelta)
{
    
}

HRESULT CLevel_Loading::Render()
{
    #ifdef _DEBUG
    _loader->Print_LoadingText();
    #endif

    return S_OK;
}

HRESULT CLevel_Loading::Ready_Layer_Background(const wstring& layerTag)
{

    return S_OK;
}

HRESULT CLevel_Loading::Ready_Layer_UI(const wstring& uiTag)
{

    return S_OK;
}

shared_ptr<CLevel_Loading> CLevel_Loading::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, LEVEL nextLevelID)
{
    auto instance = make_shared<CLevel_Loading>(device, context);

    if (FAILED(instance->Initialize(nextLevelID)))
    {
        MSG_BOX("Faield to Created : Level Loading");

        return nullptr;
    }

    return instance;
}

void CLevel_Loading::Free()
{
    _loader.reset();

    CLevel::Free();
}
