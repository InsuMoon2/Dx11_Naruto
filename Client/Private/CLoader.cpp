#include "pch.h"
#include "CLoader.h"

CLoader::CLoader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

CLoader::~CLoader()
{
}

unsigned int APIENTRY ThreadMain(void* arg)
{
    CLoader* loader = static_cast<CLoader*>(arg);
    NULL_CHECK_RETURN(loader, 1);

    if (FAILED(loader->Loading()))
        return 1;


    return 0;
}

HRESULT CLoader::Initialize(LEVEL nextLevelID)
{
    _nextLevelID = nextLevelID;

    // 크리티컬 섹션 초기화
    InitializeCriticalSection(&_criticalSection);

    // 쓰레드 생성 및 시작
    _thread = (HANDLE)_beginthreadex(
        nullptr, 0, ThreadMain, this, 0, nullptr);

    if (_thread == 0)
    {
        MSG_BOX("Faield to Created : Thread");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CLoader::Loading()
{
    // 크리티컬 섹션 진입
    EnterCriticalSection(&_criticalSection);

    HRESULT hr = { };

    switch (_nextLevelID)
    {
    case LEVEL::LOGO:
        hr = Loading_For_LogoLevel();
        break;

    case LEVEL::GAMEPLAY:
        hr = Loading_For_GamePlay();
        break;
    }

    // 크리티컬 섹션 탈출
    LeaveCriticalSection(&_criticalSection);


    return hr;
}

HRESULT CLoader::Print_LoadingText()
{
    SetWindowText(g_hWnd, _loadingText);

    return S_OK;
}

HRESULT CLoader::Loading_For_LogoLevel()
{
    // TODO : 로고 로딩
    lstrcpy(_loadingText, TEXT("텍스쳐 로딩 중"));
    for (int i = 0; i < 999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("셰이더 로딩 중"));
    for (int i = 0; i < 99999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("사운드 로딩 중"));
    for (int i = 0; i < 99999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("모델 로딩 중"));
    for (int i = 0; i < 99999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));
    for (int i = 0; i < 99999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("로딩 완료"));
    for (int i = 0; i < 99999; i++)
        int Data = 10;


    lstrcpy(_loadingText, TEXT("로딩이 완료되었습니다."));

    _isFinished = true;

    return S_OK;
}

HRESULT CLoader::Loading_For_GamePlay()
{
    // TODO : 게임플레이 로딩
    lstrcpy(_loadingText, TEXT("텍스쳐 로딩 중"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("셰이더 로딩 중"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("사운드 로딩 중"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("모델 로딩 중"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("로딩 완료"));
    for (int i = 0; i < 9999999; i++)
        int Data = 10;

    lstrcpy(_loadingText, TEXT("로딩이 완료되었습니다."));

    _isFinished = true;

    return S_OK;
}

shared_ptr<CLoader> CLoader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, LEVEL nextLevelID)
{
    auto instance = make_shared<CLoader>(device, context);

    if (FAILED(instance->Initialize(nextLevelID)))
    {
        MSG_BOX("Failed to Create : Loader");
        return nullptr;
    }

    return instance;
}

void CLoader::Free()
{
    // 쓰레드 완료 대기
    WaitForSingleObject(_thread, INFINITE);

    // 핸들 정리
    CloseHandle(_thread);
    DeleteCriticalSection(&_criticalSection);

    CBase::Free();
}
