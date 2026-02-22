#include "pch.h"
#include "Loader.h"
#include "Background.h"
#include "GameInstance.h"
#include "Player.h"
#include "Monster.h"
#include "Texture.h"
#include <magic_enum/magic_enum.hpp>
#include <fstream>

#include "AIController.h"
#include "Utils.h"
#include "Component_Factory.h"
#include "Replicator.h"
#include "BehaviorTree.h"
#include "CombatStat.h"
#include "VIBuffer_Rect.h"
#include "Shader.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "PlayerController.h"

#include "BTNode_Factory.h"

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
    auto factory = Component_Factory::GetInstance();

    uint32 staticLevel = ETOI(ELevelType::Static);

    factory->Register<CombatStat>(staticLevel, _device, _context);
    factory->Register<Replicator>(staticLevel, _device, _context);
    factory->Register<VIBuffer_Rect>(staticLevel, _device, _context);
    //factory->Register<Shader>(staticLevel, _device, _context);
    factory->Register<MovementComponent>(staticLevel, _device, _context);
    factory->Register<InputComponent>(staticLevel, _device, _context);

    factory->Register<BehaviorTree>(staticLevel, _device, _context);
    factory->Register<PlayerController>(staticLevel, _device, _context);
    factory->Register<AIController>(staticLevel, _device, _context);

    // TODO : 셰이더, 텍스처쪽 깔끔하게 바꾸기
    // Shader, Texture.. Lazy Load 또는 일단 테이블로 따로 빼는게 좋을듯

    factory->Register(
        Shader::StaticTypeID(),
        [](auto d, auto c) { return Shader::Create(d, c,
            TEXT("../../Client/Bin/Shaders/Shader_VtxTex.hlsl"),
            FVertexTex::Vertex_Desc_Layout,
            FVertexTex::Vertex_Desc_Layout_Count); },
        Shader::StaticClassName());
    factory->Register_Prototype(Shader::StaticTypeID(), staticLevel, _device, _context);

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_PLAYER,
        Player::Create(_device, _context));

    GAME->Add_GameObject_Prototype(staticLevel, Protocol::OBJECT_TYPE_MONSTER,
        Monster::Create(_device, _context));
}

void Loader::Initialize_BT_Nodes()
{
    auto& factory = BTNode_Factory::GetInstance();

    factory->Register_EngineNodes();

    // TODO : 클라이언트 노드 여기에 추가하기
    // Patrol, Attack, Skill 이런거
}

HRESULT Loader::Loading_For_LogoLevel()
{
    uint32 levelIndex = ETOI(ELevelType::Logo);

    Register_Components();
    Initialize_BT_Nodes();

    lstrcpy(_loadingText, TEXT("로고 리소스 로딩 중"));

    if (FAILED(Load_Resources_From_Json(TEXT("../../Client/Bin/Resources/Data/json/ObjectTable.json"))))
    {
        LOG_ERROR("Failed to Load Resources from JSON");
        return E_FAIL;
    }

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

    Register_Components();
    Initialize_BT_Nodes();

    lstrcpy(_loadingText, TEXT("게임플레이 리소스 로딩 중"));

    if (FAILED(Load_Resources_From_Json(TEXT("../../Client/Bin/Resources/Data/json/ObjectTable.json"))))
    {
        LOG_ERROR("Failed to Load Resources from JSON");
        return E_FAIL;
    }

    lstrcpy(_loadingText, TEXT("객체 원형 로딩 중"));
    if (FAILED(GAME->Add_GameObject_Prototype(levelIndex, Protocol::OBJECT_TYPE_PLAYER,
        Player::Create(_device, _context))))
    {
        MSG_BOX("Failed to Add Prototype : Prototype_TestPlayer");
        return E_FAIL;
    }

    lstrcpy(_loadingText, TEXT("GamePlay 로딩 완료"));

    _isFinished = true;

    return S_OK;
}

HRESULT Loader::Load_Resources_From_Json(const wstring& filePath)
{
    // Json 로드
    ifstream file(filePath);
    if (!file.is_open())
    {
        MSG_BOX("Failed to open Resource JSON file !");

        return E_FAIL;
    }

    json root;
    file >> root;

    // Texture 로드
    if (root.contains("Texture"))
    {
        for (const auto& item : root["Texture"])
        {
            string keyStr = item["id"];
            wstring protoKey = Utils::ToWString(keyStr);
            uint32 protoID = Get_ComponentID_From_String(protoKey);

            if (protoID == 0)
                continue;

            // Path
            string pathStr = item["path"];
            wstring texturePath = Utils::ToWString(pathStr);

            // Count, Level
            int32 count = item.value("count", 1);
            string levelStr = item.value("level", "Static");
            uint32 levelIndex = Get_LevelIndex_From_String(Utils::ToWString(levelStr));

            // 로딩 텍스트 업데이트
            lstrcpy(_loadingText, (TEXT("Texture: ") + protoKey).c_str());

            if (FAILED(GAME->Add_Component_Prototype(levelIndex, protoID,
                    Texture::Create(_device, _context, texturePath.c_str(), count))))
            {
                wstring errorMsg = L"Failed to load: " + protoKey;
                MSG_BOX_S(errorMsg.c_str());
            }
        }
    }

    // TODO : 모델 로드, 다른 리소스 로드 추후 추가


    return S_OK;
}

uint32 Loader::Get_LevelIndex_From_String(const wstring& levelName)
{
    string levelNameStr = Utils::ToString(levelName);

    auto levelEnum = magic_enum::enum_cast<ELevelType>(levelNameStr);

    // 주의, Enum값과 csv에 세팅된 이름이 똑같아야함 [Loading, Logo, GamePlay]
    if (levelEnum.has_value())
    {
        return ETOI(levelEnum.value());
    }

    return ETOI(ELevelType::Static);
}

uint32 Loader::Get_ComponentID_From_String(const wstring& key)
{
    const google::protobuf::EnumDescriptor* descriptor = Protocol::ComponentID_descriptor();

    const google::protobuf::EnumValueDescriptor* valueDesc =
        descriptor->FindValueByName(Utils::ToString(key));

    if (valueDesc == nullptr)
    {
        wstring errorMsg = L"CRITICAL: Unknown Component Key -> " + key;
        MSG_BOX_S(errorMsg.c_str());

        return 0; 
    }

    return static_cast<uint32>(valueDesc->number());
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
