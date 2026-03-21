#include "pch.h"
#include "Loader.h"
#include "Background.h"
#include "GameInstance.h"
#include "Player.h"
#include "Monster.h"
#include <fstream>
#include "UI_Text.h"

#include "PlayerStart.h"
#include "SkillDataManager.h"
#include "AIController.h"
#include "Utils.h"
#include "Replicator.h"
#include "BehaviorTree.h"
#include "CombatStat.h"
#include "VIBuffer_Rect.h"
#include "VIBuffer_Terrain.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "PlayerController.h"
#include "PlayerStateMachine.h"
#include "SkillComponent.h"
#include "Level.h"

#include "Model.h"
#include "Camera_Free.h"
#include "Camera_Target.h"
#include "MyPlayer.h"
#include "RemotePlayer.h"
#include "ResourceLoader.h"
#include "Terrain.h"
#include "Model.h"
#include "StaticMeshActor.h"

#include "Shader.h"
#include "UI_LoadingProgressBar.h"
#include "UI_LoadingSpinner.h"
#include "UI_MainTitleMenuButton.h"
#include "UI_PlayerHP.h"
#include "UI_PlayerHUD.h"
#include "UI_PlayerSkill.h"
#include "UI_PlayerStatus.h"
#include "UI_SkillSlot.h"
#include "AnimationStateComponent.h"
#include "Player_BodyUpper.h"

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

HRESULT Loader::Initialize(ELevelType nextLevelID, bool loadSharedResources)
{
    _nextLevelID = nextLevelID;
    _loadSharedResources = loadSharedResources;

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
        _prepareFailed = true;

        CoUninitialize();
        LeaveCriticalSection(&_criticalSection);
        return E_FAIL;
    }

    vector<FLoadJob> jobs;

    HRESULT hr = S_OK;

    switch (_nextLevelID)
    {
    case ELevelType::MainTitle:
        hr = Loading_For_Maintitle();
        break;

    case ELevelType::GamePlay:
        hr = Loading_For_GamePlay();
        break;

    case ELevelType::Equipment:
        hr = Loading_For_Equipment();
        break;
    }

    if (FAILED(hr))
        _prepareFailed = true;


    CoUninitialize();
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
    GAME->Register_ComponentFactory<PlayerStateMachine>(staticLevel);
    GAME->Register_ComponentFactory<SkillComponent>(staticLevel);
    GAME->Register_ComponentFactory<AnimationStateComponent>(staticLevel);
    //GAME->Register_ComponentFactory<Model>(staticLevel);

    /* GameObject */
    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_PLAYER,
        MyPlayer::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_REMOTE_PLAYER,
        RemotePlayer::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_MONSTER,
        Monster::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_TERRAIN,
        Terrain::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_PART_OBJECT,
        Player_BodyUpper::Create(_device, _context));
}

void Loader::Initialize_BT_Nodes()
{
    // 엔진 노드는 Initialize_Engine에서 호출

    // TODO : 클라이언트 노드 여기에 추가하기
    // Patrol, Attack, Skill 이런거
}

float Loader::Get_ProgressRatio() const
{
    const int32 totalJobs = _totalJobs.load();

    if (totalJobs <= 0)
    {
        return _prepareFinished.load() ? 1.f : 0.f;
    }

    const int32 completedJobs = _completedJobs.load();

    return ::clamp(
        static_cast<float>(completedJobs) / static_cast<float>(totalJobs),
        0.f, 1.f);
}

bool Loader::Pop_NextJob(FLoadJob& outJob)
{
    scoped_lock lock(_jobMutex);

    if (_pendingJobs.empty())
        return false;

    outJob = std::move(_pendingJobs.front());
    _pendingJobs.pop();
    return true;
}

HRESULT Loader::Execute_Job_OnMainThread(const FLoadJob& job)
{
    switch (job.type)
    {
    case ELoadJobType::Shader:
        {
        auto layout = ResourceLoader::Get_InputLayout(job.extraStr);
        if (!layout.desc)
        {
            LOG_ERROR("Invalid shader layout: {}", job.extraStr);
            return E_FAIL;
        }

        wstring wPath = Utils::ToWString(job.pathStr);
        auto desc = layout.desc;
        auto count = layout.count;
        auto className = Utils::ToWString(job.idStr);

        GAME->Register_ComponentFactory(
            job.componentID,
            [wPath, desc, count](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                return Shader::Create(device, context, wPath, desc, count);
            },
            className);

        GAME->Register_ComponentFactory_Prototype(job.componentID, job.levelIndex);
        break;
        }
    case ELoadJobType::TextureCreate:
        {
        wstring wPath = Utils::ToWString(job.pathStr);

        auto texture = Texture::Create(_device, _context, wPath.c_str(), job.count);
        CHECK_NULL(texture, E_FAIL);

        CHECK_FAILED(
            GAME->Add_Component_Prototype(job.levelIndex, job.componentID, texture),
            E_FAIL);
            break;
        }
    case ELoadJobType::TextureAppend:
    {
        auto component = GAME->Find_Component_Prototype(job.levelIndex, job.componentID);
        auto texture = dynamic_pointer_cast<Texture>(component);
        CHECK_NULL(texture, E_FAIL);

        wstring wPath = Utils::ToWString(job.pathStr);

        if (job.pathStr.find("%d") != string::npos)
        {
            for (uint32 i = 0; i < job.count; ++i)
            {
                wchar_t fullPath[MAX_PATH] = {};
                wsprintf(fullPath, wPath.c_str(), i);
                CHECK_FAILED(texture->Add_SRV(fullPath), E_FAIL);
            }
        }
        else
        {
            CHECK_FAILED(texture->Add_SRV(wPath), E_FAIL);
        }
        break;
    }
    case ELoadJobType::Terrain:
        {
        wstring wPath = Utils::ToWString(job.pathStr);
        auto className = Utils::ToWString(job.idStr);

        GAME->Register_ComponentFactory(
            job.componentID,
            [wPath](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                return VIBuffer_Terrain::Create(device, context, wPath);
            },
            className);

        GAME->Register_ComponentFactory_Prototype(job.componentID, job.levelIndex);
        break;
        }
    case ELoadJobType::Model:
        {
        wstring wPath = Utils::ToWString(job.pathStr);
        auto className = Utils::ToWString(job.idStr);
        EMeshVertexType modelType = job.isSkeletal ? EMeshVertexType::SkeletalMesh : EMeshVertexType::StaticMesh;

        string pathStr = Utils::ToString(wPath);

        GAME->Register_ComponentFactory(
            job.componentID,
            [pathStr, modelType](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                Matrix preTransform = Matrix::Identity;

                if (modelType == EMeshVertexType::SkeletalMesh)
                {
                    preTransform =
                        Matrix::CreateScale(0.0001f) *
                        Matrix::CreateRotationY(XMConvertToRadians(180.f));
                }

                return Model::Create(device, context, modelType, pathStr, preTransform);
            },
            className);

        GAME->Register_ComponentFactory_Prototype(job.componentID, job.levelIndex);
        break;
        }
    case ELoadJobType::Skill:
        {
        auto mgr = GET_SINGLE(SkillDataManager);
        mgr->Register_Skill(job.skillData);
        break;
        }

    case ELoadJobType::GameObjectPrototype:
        {
        const auto objType = static_cast<Protocol::OBJECT_TYPE>(job.objectType);

        switch (objType)
        {
        case Protocol::OBJECT_TYPE_TERRAIN:
            CHECK_FAILED(GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType,
                Terrain::Create(_device, _context)), E_FAIL);
            break;

        case Protocol::OBJECT_TYPE_CAMERA_FREE:
            CHECK_FAILED(GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType,
                Camera_Free::Create(_device, _context)), E_FAIL);
            break;

        case Protocol::OBJECT_TYPE_CAMERA_TARGET:
            CHECK_FAILED(GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType,
                Camera_Target::Create(_device, _context)), E_FAIL);
            break;

        case Protocol::OBJECT_TYPE_PLAYER_START:
            CHECK_FAILED(GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType,
                PlayerStart::Create(_device, _context)), E_FAIL);
            break;

        default:
            return E_FAIL;
        }
        break;
        }

    case ELoadJobType::LevelChunk:
        {
        CHECK_FAILED(Level::Load_LevelChunkToLevel(
            job.levelIndex,
            job.prototypeLevelIndex,
            Utils::ToWString(job.pathStr)), E_FAIL);
        break;
        }

    default:
        return E_FAIL;
    }

    ++_completedJobs;

    if (_prepareFinished.load() && _completedJobs.load() >= _totalJobs.load())
    {
        _isFinished = true;
    }

    return S_OK;
}

HRESULT Loader::Loading_For_Maintitle()
{
    vector<FLoadJob> jobs;

    //uint32 levelIndex = ETOI(ELevelType::MainTitle);

    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        lstrcpy(_loadingText, TEXT("셰이더 작업 준비 중"));
        CHECK_FAILED(_resourceLoader->Build_ShaderJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json"), jobs), E_FAIL);

        lstrcpy(_loadingText, TEXT("지형 작업 준비 중"));
        CHECK_FAILED(_resourceLoader->Build_TerrainJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json"), jobs), E_FAIL);

        lstrcpy(_loadingText, TEXT("텍스처 작업 준비 중"));
        CHECK_FAILED(_resourceLoader->Build_TextureJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json"), jobs), E_FAIL);

        lstrcpy(_loadingText, TEXT("모델 작업 준비 중"));
        CHECK_FAILED(_resourceLoader->Build_ModelJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ModelTable.json"), jobs), E_FAIL);

        lstrcpy(_loadingText, TEXT("스킬 작업 준비 중"));
        CHECK_FAILED(_resourceLoader->Build_SkillJobs(
            TEXT("../../Client/Bin/Resources/Data/json/SkillDataTable.json"), jobs), E_FAIL);
    }

    {
        scoped_lock lock(_jobMutex);

        for (auto& job : jobs)
            _pendingJobs.push(std::move(job));
    }

    _totalJobs = static_cast<uint32>(jobs.size());
    _completedJobs = 0;
    _prepareFinished = true;

    _isFinished = (_totalJobs.load() == 0);

#ifdef _DEBUG
    lstrcpy(_loadingText, TEXT("MainTitle loading jobs prepared"));
#endif

    return S_OK;
}

HRESULT Loader::Loading_For_GamePlay()
{
    //::Sleep(20000); UI 값 변경 테스트용 Delay

    vector<FLoadJob> jobs;

    // 클라 단독 실행
    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        lstrcpy(_loadingText, TEXT("공용 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_ShaderJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_TerrainJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_TextureJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_ModelJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ModelTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_SkillJobs(
            TEXT("../../Client/Bin/Resources/Data/json/SkillDataTable.json"), jobs), E_FAIL);
    }

    // 게임오브젝트
    auto pushProto = [&](Protocol::OBJECT_TYPE type)
        {
            FLoadJob job{};
            job.type = ELoadJobType::GameObjectPrototype;
            job.levelIndex = ETOI(ELevelType::GamePlay);
            job.objectType = static_cast<uint32>(type);
            jobs.push_back(std::move(job));
        };

    // 맵
    auto pushChunk = [&](const char* fileName)
        {
            FLoadJob job{};
            job.type = ELoadJobType::LevelChunk;
            job.levelIndex = ETOI(ELevelType::GamePlay);
            job.prototypeLevelIndex = ETOI(ELevelType::GamePlay);
            job.pathStr = fileName;
            jobs.push_back(std::move(job));
        };

    // 게임오브젝트
    pushProto(Protocol::OBJECT_TYPE_TERRAIN);
    pushProto(Protocol::OBJECT_TYPE_CAMERA_FREE);
    pushProto(Protocol::OBJECT_TYPE_CAMERA_TARGET);
    pushProto(Protocol::OBJECT_TYPE_PLAYER_START);

    // 맵 리소스 로드
    pushChunk("BM_KonohaVillage03_Environments_BackdropBuildings");
    pushChunk("BM_KonohaVillage03_Environments_Props");
    pushChunk("BM_KonohaVillage03_Environments_Terrain");

    {
        scoped_lock lock(_jobMutex);
        for (auto& job : jobs)
            _pendingJobs.push(std::move(job));
    }

    _totalJobs = static_cast<int32>(jobs.size());
    _completedJobs = 0;
    _prepareFinished = true;

    _isFinished = (_totalJobs.load() == 0);

    return S_OK;
}

HRESULT Loader::Loading_For_Equipment()
{
    vector<FLoadJob> jobs;

    // 클라 단독 실행
    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        lstrcpy(_loadingText, TEXT("공용 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_ShaderJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ShaderTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_TerrainJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TerrainTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_TextureJobs(
            TEXT("../../Client/Bin/Resources/Data/json/TextureTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_ModelJobs(
            TEXT("../../Client/Bin/Resources/Data/json/ModelTable.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_SkillJobs(
            TEXT("../../Client/Bin/Resources/Data/json/SkillDataTable.json"), jobs), E_FAIL);
    }

    auto pushProto = [&](Protocol::OBJECT_TYPE type)
        {
            FLoadJob job{};
            job.type = ELoadJobType::GameObjectPrototype;
            job.levelIndex = ETOI(ELevelType::Equipment);
            job.objectType = static_cast<uint32>(type);
            jobs.push_back(std::move(job));
        };

    pushProto(Protocol::OBJECT_TYPE_CAMERA_FREE);
    pushProto(Protocol::OBJECT_TYPE_CAMERA_TARGET);

    {
        scoped_lock lock(_jobMutex);
        for (auto& job : jobs)
            _pendingJobs.push(std::move(job));
    }

    _totalJobs = static_cast<int32>(jobs.size());
    _completedJobs = 0;
    _prepareFinished = true;

    _isFinished = (_totalJobs.load() == 0);

    return S_OK;
}

shared_ptr<Loader> Loader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID, bool loadSharedResources)
{
    auto instance = make_shared<Loader>(device, context);

    if (FAILED(instance->Initialize(nextLevelID, loadSharedResources)))
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
