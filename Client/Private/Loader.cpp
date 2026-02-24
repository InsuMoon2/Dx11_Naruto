#include "pch.h"
#include "Loader.h"
#include "Background.h"
#include "GameInstance.h"
#include "Player.h"
#include "Monster.h"
#include <fstream>

#include "AIController.h"
#include "Utils.h"
#include "Replicator.h"
#include "BehaviorTree.h"
#include "CombatStat.h"
#include "VIBuffer_Rect.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "PlayerController.h"

#include "Camera_Free.h"
#include "MyPlayer.h"
#include "RemotePlayer.h"
#include "ResourceLoader.h"
#include "Terrain.h"

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
    CHECK_NULL(loader, 1);

    if (FAILED(loader->Loading()))
        return 1;


    return 0;
}

HRESULT Loader::Initialize(ELevelType nextLevelID)
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

    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    _resourceLoader = ResourceLoader::Create(_device, _context);
    if (!_resourceLoader)
    {
        LOG_ERROR("Failed to Create ResourceLoader");
        LeaveCriticalSection(&_criticalSection);
        return E_FAIL;
    }

    HRESULT hr = { };

    switch (_nextLevelID)
    {
    case ELevelType::Logo:
        hr = Loading_For_LogoLevel();
        break;

    case ELevelType::GamePlay:
        hr = Loading_For_GamePlay();
        break;
    }

    CoUninitialize();

    // 크리티컬 섹션 탈출
    LeaveCriticalSection(&_criticalSection);

    return hr;
}

HRESULT Loader::Print_LoadingText()
{
    SetWindowText(g_hWnd, _loadingText);

    return S_OK;
}

void Loader::Register_Components()
{
    uint32 staticLevel = ETOI(ELevelType::Static);

    /* Component */
    GAME->Register_ComponentFactory<CombatStat>(staticLevel);
    GAME->Register_ComponentFactory<Replicator>(staticLevel);
    GAME->Register_ComponentFactory<VIBuffer_Rect>(staticLevel);
    GAME->Register_ComponentFactory<MovementComponent>(staticLevel);
    GAME->Register_ComponentFactory<InputComponent>(staticLevel);
    GAME->Register_ComponentFactory<BehaviorTree>(staticLevel);
    GAME->Register_ComponentFactory<PlayerController>(staticLevel);
    GAME->Register_ComponentFactory<AIController>(staticLevel);

    /* GameObject */
    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_PLAYER,
        MyPlayer::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_REMOTE_PLAYER,
        RemotePlayer::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_MONSTER,
        Monster::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_TERRAIN,
        Terrain::Create(_device, _context));
}

void Loader::Initialize_BT_Nodes()
{
    // 엔진 노드는 Initialize_Engine에서 호출

    // TODO : 클라이언트 노드 여기에 추가하기
    // Patrol, Attack, Skill 이런거
}

HRESULT Loader::Loading_For_LogoLevel()
{
    uint32 levelIndex = ETOI(ELevelType::Logo);

    Register_Components();
    Initialize_BT_Nodes();

    lstrcpy(_loadingText, TEXT("로고 리소스 로딩 중"));

    if (FAILED(_resourceLoader->Load_ShaderTable(
        TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json"))))
        return E_FAIL;

    if (FAILED(_resourceLoader->Load_TerrainTable(
        TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json"))))
        return E_FAIL;

    if (FAILED(_resourceLoader->Load_TextureTable(
        TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json"))))
        return E_FAIL;

    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));

    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, Protocol::OBJECT_TYPE_BACKGROUND,
        Background::Create(_device, _context))))
    {
        LOG_ERROR("Failed to Add Prototype : Background");
        return E_FAIL;
    }


    lstrcpy(_loadingText, TEXT("Logo 로딩 완료"));


    _isFinished = true;

    return S_OK;
}

HRESULT Loader::Loading_For_GamePlay()
{
    uint32 levelIndex = ETOI(ELevelType::GamePlay);

    //Register_Components();
    //Initialize_BT_Nodes();

    lstrcpy(_loadingText, TEXT("게임플레이 리소스 로딩 중"));

    if (FAILED(_resourceLoader->Load_ShaderTable(
        TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json"))))
        return E_FAIL;

    if (FAILED(_resourceLoader->Load_TerrainTable(
        TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json"))))
        return E_FAIL;

    if (FAILED(_resourceLoader->Load_TextureTable(
        TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json"))))
        return E_FAIL;

    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));
    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, Protocol::OBJECT_TYPE_PLAYER,
        Player::Create(_device, _context))))
    {
        MSG_BOX("Failed to Add Prototype : Prototype_Player");
        return E_FAIL;
    }

    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, Protocol::OBJECT_TYPE_TERRAIN,
        Terrain::Create(_device, _context))))
    {
        MSG_BOX("Failed to Add Prototype : Prototype_Terrain");
        return E_FAIL;
    }

    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, Protocol::OBJECT_TYPE_CAMERA_FREE,
        Camera_Free::Create(_device, _context))))
    {
        MSG_BOX("Failed to Add Prototype : Camera_Free");
        return E_FAIL;
    }


    lstrcpy(_loadingText, TEXT("GamePlay 로딩 완료"));

    _isFinished = true;

    return S_OK;
}

shared_ptr<Loader> Loader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID)
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
