#include "pch.h"
#include "Loader.h"
#include "Background.h"
#include "GameInstance.h"

Loader::Loader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

Loader::~Loader()
{
}

unsigned int APIENTRY ThreadMain(void* arg)
{
    Loader* loader = static_cast<Loader*>(arg);
    NULL_CHECK_RETURN(loader, 1);

    if (FAILED(loader->Loading()))
        return 1;


    return 0;
}

HRESULT Loader::Initialize(LevelType nextLevelID)
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

HRESULT Loader::Loading()
{
    // 크리티컬 섹션 진입
    EnterCriticalSection(&_criticalSection);

    HRESULT hr = { };

    switch (_nextLevelID)
    {
    case LevelType::Logo:
        hr = Loading_For_LogoLevel();
        break;

    case LevelType::GamePlay:
        hr = Loading_For_GamePlay();
        break;
    }

    // 크리티컬 섹션 탈출
    LeaveCriticalSection(&_criticalSection);


    return hr;
}

HRESULT Loader::Print_LoadingText()
{
    SetWindowText(g_hWnd, _loadingText);

    return S_OK;
}

HRESULT Loader::Loading_For_LogoLevel()
{
    uint32 levelIndex = ETOI(LevelType::Logo);

    lstrcpy(_loadingText, TEXT("텍스쳐 로딩 중"));


    lstrcpy(_loadingText, TEXT("셰이더 로딩 중"));


    lstrcpy(_loadingText, TEXT("사운드 로딩 중"));


    lstrcpy(_loadingText, TEXT("모델 로딩 중"));


    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));
    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, TEXT("Prototype_Background"),
        Background::Create(_device, _context))))
    {
        MSG_BOX("Failed to Add Prototype : Background");
        return E_FAIL;
    }


    lstrcpy(_loadingText, TEXT("로딩 완료"));


    _isFinished = true;

    return S_OK;
}

HRESULT Loader::Loading_For_GamePlay()
{
    uint32 levelIndex = ETOI(LevelType::GamePlay);

    lstrcpy(_loadingText, TEXT("텍스쳐 로딩 중"));


    lstrcpy(_loadingText, TEXT("셰이더 로딩 중"));


    lstrcpy(_loadingText, TEXT("사운드 로딩 중"));


    lstrcpy(_loadingText, TEXT("모델 로딩 중"));


    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));


    lstrcpy(_loadingText, TEXT("로딩 완료"));



    _isFinished = true;

    return S_OK;
}

shared_ptr<Loader> Loader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, LevelType nextLevelID)
{
    auto instance = make_shared<Loader>(device, context);

    if (FAILED(instance->Initialize(nextLevelID)))
    {
        MSG_BOX("Failed to Create : Loader");
        return nullptr;
    }

    return instance;
}

void Loader::Free()
{
    // 쓰레드 완료 대기
    WaitForSingleObject(_thread, INFINITE);

    // 핸들 정리
    CloseHandle(_thread);
    DeleteCriticalSection(&_criticalSection);

    Base::Free();
}
