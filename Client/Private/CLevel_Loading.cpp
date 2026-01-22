#include "pch.h"
#include "CLevel_Loading.h"
#include "CLoader.h"
#include "CGameInstance.h"

CLevel_Loading::CLevel_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : CLevel { device, context }
{
}

CLevel_Loading::~CLevel_Loading()
{
}

HRESULT CLevel_Loading::Initialize(LEVEL nextLevelID)
{
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
    if (_loader && _loader->IsFinished())
    {

    }
}

void CLevel_Loading::LateUpdate(float timeDelta)
{
    
}

HRESULT CLevel_Loading::Render()
{

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
