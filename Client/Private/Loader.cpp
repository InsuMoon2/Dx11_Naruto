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
#include "Player_CustomPart.h"

#include "ComboProfile_Manager.h"
#include "EquipmentComponent.h"
#include "ProjectileComponent.h"
#include "TargetComponent.h"
#include "GhostEffect_Component.h"
#include "SmearEffect_Component.h"  
#include "EffectComponent.h"

#include "Collider.h"
#include "CharkraMove_Component.h"
#include "SwordTrail_Component.h"

#include "VIBuffer_Particle_Point.h"
#include "Particle_Point.h"
#include "Trail_Component.h"
#include "VIBuffer_Trail.h"

// Behavior
#include "BTTask_Attack.h"
#include "BTTask_ChibakuTensei.h"
#include "BTTask_Dash.h"
#include "BTTask_FindClosestTarget.h"
#include "BTTask_Hit.h"
#include "BTTask_PlayStateAndWait.h"
#include "BTTask_DodgeAndWait.h"
#include "BTTask_JumpToTarget.h"
#include "BTTask_RetreatAndWait.h"
#include "BTTask_SelectAttackType.h"
#include "BTTask_JumpToTarget.h"
#include "BTTask_UpdateBossContext.h"
#include "BTTask_PainForceSkill.h"
#include "BTTask_StrafeAroundTarget.h"
#include "BTTask_KonohaSenpung.h"

static bool Try_GetLevelObjectType(const json& objJson, Protocol::OBJECT_TYPE& outObjectType)
{
    if (!objJson.contains("object_type"))
        return false;

    if (objJson["object_type"].is_string())
    {
        const auto objectType = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
            objJson["object_type"].get<string>());

        if (!objectType.has_value())
            return false;

        outObjectType = objectType.value();
        return true;
    }

    outObjectType = static_cast<Protocol::OBJECT_TYPE>(objJson["object_type"].get<uint32>());
    return true;
}

// 레벨 오브젝트 JSON에서 static mesh 계열의 model_guid를 뽑아낼 때 호출한다.
// custom_properties 안에 들어간 경우도 같이 처리한다.
static bool Try_GetStaticModelGuidFromLevelObject(const json& objJson, string& outModelGuid)
{
    Protocol::OBJECT_TYPE objectType = Protocol::OBJECT_TYPE_NONE;
    if (!Try_GetLevelObjectType(objJson, objectType))
        return false;

    if (objectType != Protocol::OBJECT_TYPE_STATIC_MESH &&
        objectType != Protocol::OBJECT_TYPE_COLLISION_PROXY)
    {
        return false;
    }

    if (objJson.contains("model_guid") && objJson["model_guid"].is_string())
    {
        outModelGuid = objJson["model_guid"].get<string>();
        return !outModelGuid.empty();
    }

    if (objJson.contains("custom_properties") &&
        objJson["custom_properties"].is_object() &&
        objJson["custom_properties"].contains("model_guid") &&
        objJson["custom_properties"]["model_guid"].is_string())
    {
        outModelGuid = objJson["custom_properties"]["model_guid"].get<string>();
        return !outModelGuid.empty();
    }

    return false;
}

// 코노하 맵 로딩 직전에 FModel 변환 산출물 폴더를 런타임 에셋 매니저에 다시 스캔할 때 호출한다.
// 배치로 새로 생성한 COL meshbin/meta가 .asset_cache.json에 아직 반영되지 않았더라도 이번 로딩에서 바로 resolve 되게 만든다.
static void Scan_KonohaLevelAssets()
{
    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources/StaticMesh/KonohaVillage"));
    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources/Data/json/Levels"));
}

// 같은 static model guid는 로딩 화면에서 한 번만 프로토타입 preload 잡으로 등록한다.
static void Append_StaticModelPrototypePreloadJob(
    const json& objJson,
    unordered_set<string>& queuedModelGuids,
    vector<FLoadJob>& outJobs)
{
    string modelGuid;
    if (!Try_GetStaticModelGuidFromLevelObject(objJson, modelGuid))
        return;

    if (!queuedModelGuids.insert(modelGuid).second)
        return;

    FLoadJob preloadJob{};
    preloadJob.type = ELoadJobType::StaticModelPrototype;
    preloadJob.levelIndex = ETOI(ELevelType::Static);
    preloadJob.componentID = static_cast<uint32>(hash<string>{}(modelGuid));
    preloadJob.idStr = modelGuid;
    outJobs.push_back(std::move(preloadJob));
}

static HRESULT Append_LevelObjectJobsFromFile(const wstring& fileName,
                                              uint32 targetLevelIndex,
                                              uint32 prototypeLevelIndex,
                                              unordered_set<string>& queuedModelGuids,
                                              vector<FLoadJob>& outJobs)
{
    const wstring levelDirectory = L"../../Client/Bin/Resources/Data/json/Levels/";
    const array<wstring, 2> candidatePaths =
    {
        levelDirectory + fileName + L".level.json",
        levelDirectory + fileName + L".proxy.level.json"
    };

    for (const wstring& candidatePath : candidatePaths)
    {
        if (!filesystem::exists(candidatePath))
            continue;

        ifstream file(candidatePath);
        if (!file.is_open())
        {
            LOG_ERROR("파일 열기 실패 : {}", Utils::ToString(candidatePath));
            return E_FAIL;
        }

        json root;
        file >> root;
        file.close();

        if (!root.contains("gameObjects") || !root["gameObjects"].is_array())
            continue;

        for (const auto& objJson : root["gameObjects"])
        {
            Append_StaticModelPrototypePreloadJob(objJson, queuedModelGuids, outJobs);

            FLoadJob job{};
            job.type = ELoadJobType::LevelObject;
            job.levelIndex = targetLevelIndex;
            job.prototypeLevelIndex = prototypeLevelIndex;
            job.payloadJson = objJson;
            outJobs.push_back(std::move(job));
        }
    }

    return S_OK;
}


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

    case ELevelType::CharacterSetup:
        hr = Loading_For_CharacterSetup();
        break;

    case ELevelType::Konoha:
        hr = Loading_For_Konoha();
        break;

    case ELevelType::Lobby:
        hr = Loading_For_Lobby();
        break;
    }

    if (FAILED(hr))
        _prepareFailed = true;


    CoUninitialize();
    LeaveCriticalSection(&_criticalSection);

    return hr;
}

#ifdef _DEBUG
HRESULT Loader::Print_LoadingText()
{
    SetWindowText(g_hWnd, _loadingText);

    return S_OK;
}
#endif

void Loader::Register_Components()
{
    uint32 staticLevel = ETOI(ELevelType::Static);

    /* Component */
    GAME->Register_ComponentFactory<CombatStat>(staticLevel);
    GAME->Register_ComponentFactory<Replicator>(staticLevel);

    GAME->Register_ComponentFactory<VIBuffer_Rect>(staticLevel);
    GAME->Register_ComponentFactory<VIBuffer_Particle_Point>(staticLevel);
    GAME->Register_ComponentFactory<VIBuffer_Trail>(staticLevel);

    GAME->Register_ComponentFactory<MovementComponent>(staticLevel);
    GAME->Register_ComponentFactory<InputComponent>(staticLevel);
    GAME->Register_ComponentFactory<BehaviorTree>(staticLevel);
    GAME->Register_ComponentFactory<PlayerController>(staticLevel);
    GAME->Register_ComponentFactory<AIController>(staticLevel);
    GAME->Register_ComponentFactory<PlayerStateMachine>(staticLevel);
    GAME->Register_ComponentFactory<SkillComponent>(staticLevel);
    GAME->Register_ComponentFactory<AnimationStateComponent>(staticLevel);
    GAME->Register_ComponentFactory<EquipmentComponent>(staticLevel);
    GAME->Register_ComponentFactory<ProjectileComponent>(staticLevel);
    GAME->Register_ComponentFactory<TargetComponent>(staticLevel);
    GAME->Register_ComponentFactory<GhostEffect_Component>(staticLevel);
    GAME->Register_ComponentFactory<SmearEffect_Component>(staticLevel);

    GAME->Register_ComponentFactory<Trail_Component>(staticLevel);
    GAME->Register_ComponentFactory<CharkraMove_Component>(staticLevel);

    GAME->Register_ComponentFactory<EffectComponent>(staticLevel);
    GAME->Register_ComponentFactory<SwordTrail_Component>(staticLevel);

    //GAME->Register_ComponentFactory<Model>(staticLevel);

    auto Register_Collider = [&](Protocol::ComponentID id, EShape shape, const wstring& name)
        {
            GAME->Register_ComponentFactory(id, [shape](ComPtr<Device> device, ComPtr<DeviceContext> context)
                {
                    return Collider::Create(device, context, shape);
                },
                name);

            GAME->Register_ComponentFactory_Prototype(id, staticLevel);
        };

    Register_Collider(Protocol::COMPONENT_TYPE_COLLIDER_AABB, EShape::AABB, TEXT("Collider_AABB"));
    Register_Collider(Protocol::COMPONENT_TYPE_COLLIDER_OBB, EShape::OBB, TEXT("Collider_OBB"));
    Register_Collider(Protocol::COMPONENT_TYPE_COLLIDER_SPHERE, EShape::Sphere, TEXT("Collider_Sphere"));
    Register_Collider(Protocol::COMPONENT_TYPE_COLLIDER_CAPSULE, EShape::Capsule, TEXT("Collider_Capsule"));

}

void Loader::Initialize_BT_Nodes()
{
    // 엔진 노드는 Initialize_Engine에서 호출
    GAME->Register_BTNode("Task", "Task_FindClosestTarget",
        []() { return BTTask_FindClosestTarget::Create(); });

    GAME->Register_BTNode("Task", "Task_Attack",
        []() { return BTTask_Attack::Create(); });

    GAME->Register_BTNode("Task", "Task_Hit",
        []() { return BTTask_Hit::Create(); });

    GAME->Register_BTNode("Task", "Task_PlayStateAndWait",
    []() { return BTTask_PlayStateAndWait::Create(); });

    GAME->Register_BTNode("Task", "Task_DodgeAndWait",
    []() { return BTTask_DodgeAndWait::Create(); });

    GAME->Register_BTNode("Task", "Task_RetreatAndWait",
    []() { return BTTask_RetreatAndWait::Create(); });

    GAME->Register_BTNode("Task", "Task_SelectAttackType",
    []() { return BTTask_SelectAttackType::Create(); });

    GAME->Register_BTNode("Task", "Task_JumpToTarget",
    []() { return BTTask_JumpToTarget::Create(); });

    GAME->Register_BTNode("Task", "Task_UpdateBossContext",
    []() { return BTTask_UpdateBossContext::Create(); });

    GAME->Register_BTNode("Task", "Task_PainForceSkill",
        []() { return BTTask_PainForceSkill::Create(); });

    GAME->Register_BTNode("Task", "Task_StrafeAroundTarget",
        []() { return BTTask_StrafeAroundTarget::Create(); });

    GAME->Register_BTNode("Task", "Task_ChibakuTensei",
    []() { return BTTask_ChibakuTensei::Create(); });

    GAME->Register_BTNode("Task", "Task_KonohaSenpung",
    []() { return BTTask_KonohaSenpung::Create(); });

    GAME->Register_BTNode("Task", "Task_Dash",
    []() { return BTTask_Dash::Create(); });
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
        wstring resolvedPath = wPath;
        if (job.count == 1 && job.pathStr.find("%d") != string::npos)
        {
            wchar_t fullPath[MAX_PATH] = {};
            wsprintf(fullPath, wPath.c_str(), job.startIndex);
            resolvedPath = fullPath;
        }

        auto texture = Texture::Create(_device, _context, resolvedPath.c_str(), job.count);
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
                wsprintf(fullPath, wPath.c_str(), job.startIndex + i);
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
    case ELoadJobType::StaticModelPrototype:
        {
        if (GAME->Find_Component_Prototype(ETOI(ELevelType::Static), job.componentID) != nullptr)
            break;

        const wstring resolvedPath = GAME->Resolve_AssetPath(job.idStr);
        if (resolvedPath.empty())
        {
            LOG_ERROR("Static model preload failed. guid='{}'", job.idStr);
            return E_FAIL;
        }

        const Matrix preTransform =
            Matrix::CreateScale(1.f) *
            Matrix::CreateRotationY(XMConvertToRadians(180.f));

        auto proto = Model::Create(
            _device,
            _context,
            EMeshVertexType::StaticMesh,
            Utils::ToString(resolvedPath),
            preTransform,
            true);

        CHECK_NULL(proto, E_FAIL);
        CHECK_FAILED(GAME->Add_Component_Prototype(ETOI(ELevelType::Static), job.componentID, proto), E_FAIL);
        break;
        }
    case ELoadJobType::Skill:
        {
        auto mgr = GET_SINGLE(SkillDataManager);
        mgr->Register_Skill(job.skillData);
        mgr->Register_Skill_IconIndex(
            static_cast<int32>(job.skillData.skill_Id),
            job.skillIconSrvIndex);

        break;
        }

    case ELoadJobType::GameObjectPrototype:
        {
        auto instance = GAME->Create_GameObjectFromFactory(
            static_cast<Protocol::OBJECT_TYPE>(job.objectType));

        if (instance)
        {
            GAME->Add_GameObject_Prototype(job.levelIndex, job.objectType, instance);
        }
        else
        {
            //LOG_ERROR("로더: 엑셀에 적힌 객체를 팩토리에서 못 찾음!");
            //return E_FAIL;
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
    case ELoadJobType::LevelObject:
        {
        CHECK_FAILED(Level::Add_GameObjectJsonToLevel(
            job.levelIndex,
            job.prototypeLevelIndex,
            job.payloadJson), E_FAIL);
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

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Model.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_SkillData.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);
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
    unordered_set<string> queuedStaticModelGuids;

    // 클라 단독 실행
    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        //lstrcpy(_loadingText, TEXT("공용 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Model.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_SkillData.json"), jobs), E_FAIL);

        // 콤보 프로파일 읽기
        GET_SINGLE(ComboProfile_Manager)->Load_FromJson(
            "../../Client/Bin/Resources/Data/json/DT_ComboProfile.json");
    }

    CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
        TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);

    auto pushChunk = [&](const char* fileName) -> HRESULT
        {
            return Append_LevelObjectJobsFromFile(
                Utils::ToWString(fileName),
                ETOI(ELevelType::GamePlay),
                ETOI(ELevelType::GamePlay),
                queuedStaticModelGuids,
                jobs);
        };

    // 맵 리소스 로드
    //pushChunk("BM_ExamStadium_Env_Terrain");
    //pushChunk("BM_ExamStadium_p");

    CHECK_FAILED(pushChunk("[20260427]Tutorial"), E_FAIL);

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

HRESULT Loader::Loading_For_CharacterSetup()
{
    vector<FLoadJob> jobs;

    // 클라 단독 실행
    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        //lstrcpy(_loadingText, TEXT("공용 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Model.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_SkillData.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);
    }

    //auto pushProto = [&](Protocol::OBJECT_TYPE type)
    //    {
    //        FLoadJob job{};
    //        job.type = ELoadJobType::GameObjectPrototype;
    //        job.levelIndex = ETOI(ELevelType::CharacterSetup);
    //        job.objectType = static_cast<uint32>(type);
    //        jobs.push_back(std::move(job));
    //    };

    //pushProto(Protocol::OBJECT_TYPE_CAMERA_FREE);
    //pushProto(Protocol::OBJECT_TYPE_CAMERA_TARGET);

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

HRESULT Loader::Loading_For_Konoha()
{
    vector<FLoadJob> jobs;
    unordered_set<string> queuedStaticModelGuids;

    // 코노하 전용 변환 산출물은 플레이 직전에 다시 스캔해서 COL proxy가 참조하는 model_guid를 즉시 찾을 수 있게 한다.
    Scan_KonohaLevelAssets();

    // 클라 단독 실행
    if (_loadSharedResources)
    {
        Register_Components();
        Initialize_BT_Nodes();

        //lstrcpy(_loadingText, TEXT("공용 리소스 작업 준비 중"));

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Shader.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Texture.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_Model.json"), jobs), E_FAIL);

        CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
            TEXT("../../Client/Bin/Resources/Data/json/DT_SkillData.json"), jobs), E_FAIL);

        // 콤보 프로파일 읽기
        GET_SINGLE(ComboProfile_Manager)->Load_FromJson(
            "../../Client/Bin/Resources/Data/json/DT_ComboProfile.json");
    }

    CHECK_FAILED(_resourceLoader->Build_AllResourceJobs(
        TEXT("../../Client/Bin/Resources/Data/json/DT_GameObject.json"), jobs), E_FAIL);

    auto pushChunk = [&](const char* fileName) -> HRESULT
        {
            return Append_LevelObjectJobsFromFile(
                Utils::ToWString(fileName),
                ETOI(ELevelType::Konoha),
                ETOI(ELevelType::Konoha),
                queuedStaticModelGuids,
                jobs);
        };

    // 맵 리소스 로드
    //CHECK_FAILED(pushChunk("BM_KonohaVillage"), E_FAIL);
    //CHECK_FAILED(pushChunk("BM_KonohaVillage_Env_Terrain"), E_FAIL);
    //CHECK_FAILED(pushChunk("BM_KonohaVillage_Floor"), E_FAIL);
    //CHECK_FAILED(pushChunk("BM_KonohaVillage_Props"), E_FAIL);
    //CHECK_FAILED(pushChunk("BM_Konoha_Village_Env_WaterTank"), E_FAIL);

    CHECK_FAILED(pushChunk("[20260427]Konoha"), E_FAIL);

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

HRESULT Loader::Loading_For_Lobby()
{
    vector<FLoadJob> jobs;

    lstrcpy(_loadingText, TEXT("로비 리소스 작업 준비 중"));

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
