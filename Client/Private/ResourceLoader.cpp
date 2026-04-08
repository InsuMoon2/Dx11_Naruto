#include "pch.h"
#include "ResourceLoader.h"
#include "GameInstance.h"
#include "Texture.h"
#include "VIBuffer_Rect.h"
#include "VIBuffer_Terrain.h"
#include <fstream>
#include <magic_enum/magic_enum.hpp>

#include "Shader.h"
#include "Model.h"

#include "SkillDataManager.h"

ResourceLoader::ResourceLoader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT ResourceLoader::Initialize()
{

    return S_OK;
}

HRESULT ResourceLoader::Load_TextureTable(const wstring& tablePath)
{
    // Json 열기
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open table: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (root.contains("Texture"))
        CHECK_FAILED(Load_Textures(root["Texture"]), E_FAIL);

    LOG_INFO("Loaded TextureTable: {}", Utils::ToString(tablePath));

    return S_OK;
}

HRESULT ResourceLoader::Load_ShaderTable(const wstring& tablePath)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (root.contains("Shader"))
        CHECK_FAILED(Load_Shaders(root["Shader"]), E_FAIL);

    LOG_INFO("Loaded: {}", Utils::ToString(tablePath));

    return S_OK;
}

HRESULT ResourceLoader::Load_TerrainTable(const wstring& tablePath)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (root.contains("Terrain"))
        CHECK_FAILED(Load_Terrains(root["Terrain"]), E_FAIL);

    LOG_INFO("Loaded: {}", Utils::ToString(tablePath));

    return S_OK;
}

HRESULT ResourceLoader::Load_ModelTable(const wstring& tablePath)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (root.contains("Model"))
        CHECK_FAILED(Load_Model(root["Model"]), E_FAIL);

    LOG_INFO("Loaded : {}", Utils::ToString(tablePath));

    return S_OK;
}

HRESULT ResourceLoader::Load_SkillTable(const wstring& tablePath)
{
    ifstream file(tablePath);

    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (root.contains("DT_SkillData"))
    {
        CHECK_FAILED(Load_Skills(root["DT_SkillData"]), E_FAIL);
    }
    else if (root.contains("Skill"))
    {
        CHECK_FAILED(Load_Skills(root["Skill"]), E_FAIL);
    }

    LOG_INFO("Loaded: {}", Utils::ToString(tablePath));

    return S_OK;
}

HRESULT ResourceLoader:: Build_TextureJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
        return E_FAIL;

    json root;
    file >> root;

    if (!root.contains("Texture"))
        return S_OK;

    umap<uint32, bool> firstTextureByType;

    for (const auto& item : root["Texture"])
    {
        string idStr = item["id"];
        string pathStr = item["path"];
        string levelStr = item.value("level", "Static");
        int32 count = item.value("count", 1);

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
            continue;

        FLoadJob job;
        job.componentID = typeId;
        job.levelIndex = levelIndex;
        job.idStr = idStr;
        job.pathStr = pathStr;
        job.count = count;

        if (!firstTextureByType[typeId])
        {
            job.type = ELoadJobType::TextureCreate;
            firstTextureByType[typeId] = true;
        }
        else
        {
            job.type = ELoadJobType::TextureAppend;
        }

        outJobs.push_back(job);
    }

    return S_OK;
}

HRESULT ResourceLoader::Build_ShaderJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (!root.contains("Shader"))
        return S_OK;

    for (const auto& item : root["Shader"])
    {
        string idStr = item["Id"];
        string pathStr = item["Path"];
        string layoutStr = item.value("Type", "");
        string levelStr = item.value("Level", "Static");
        int32 count = item.value("Count", 1);

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown Shader ID : {}", idStr);
            continue;
        }

        FLoadJob job{};
        job.type = ELoadJobType::Shader;
        job.componentID = typeId;
        job.levelIndex = levelIndex;
        job.idStr = idStr;
        job.pathStr = pathStr;
        job.extraStr = layoutStr;
        job.count = count;

        outJobs.push_back(job);
    }

    return S_OK;
}

HRESULT ResourceLoader::Build_TerrainJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (!root.contains("Terrain"))
        return S_OK;

    for (const auto& item : root["Terrain"])
    {
        string idStr = item["id"];
        string pathStr = item["path"];
        string levelStr = item.value("level", "Static");

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown terrain ID: {}", idStr);
            continue;
        }

        FLoadJob job{};
        job.type = ELoadJobType::Terrain;
        job.componentID = typeId;
        job.levelIndex = levelIndex;
        job.idStr = idStr;
        job.pathStr = pathStr;

        outJobs.push_back(job);
    }

    return S_OK;
}

HRESULT ResourceLoader::Build_ModelJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (!root.contains("Model"))
        return S_OK;

    for (const auto& item : root["Model"])
    {
        string idStr = item["id"];
        string guidStr = item.value("guid", "");
        string pathStr;
        string levelStr = item.value("level", "Static");
        string modelTypeStr = item.value("modelType", "Static");

        if (!guidStr.empty())
            pathStr = Utils::ToString(GAME->Resolve_AssetPath(guidStr));
        else
            pathStr = item["path"];

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown Model ID: {}", idStr);
            continue;
        }

        FLoadJob job{};
        job.type = ELoadJobType::Model;
        job.componentID = typeId;
        job.levelIndex = levelIndex;
        job.idStr = idStr;
        job.pathStr = pathStr;
        job.extraStr = modelTypeStr;
        job.isSkeletal = (modelTypeStr == "SkeletalMesh");

        outJobs.push_back(job);
    }

    return S_OK;
}

HRESULT ResourceLoader::Build_SkillJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    string skillKey = "";
    if (root.contains("DT_SkillData")) skillKey = "DT_SkillData";
    else if (root.contains("Skill"))   skillKey = "Skill";

    if (skillKey.empty())
        return S_OK;

    uint32 iconSrvIndex = 0;

    for (const auto& item : root[skillKey])
    {
        FLoadJob job{};
        job.type = ELoadJobType::Skill;

        job.skillData.skill_Id = item.value("SkillID", 0);
        job.skillData.skillName = Utils::ToWString(item.value("SkillName", string{}));
        job.skillData.coolDown = item.value("Cooldown", 0.f);

        job.skillIconSrvIndex = iconSrvIndex;

        outJobs.push_back(job);
        ++iconSrvIndex;
    }

    return S_OK;
}

HRESULT ResourceLoader::Build_AllResourceJobs(const wstring& tablePath, vector<FLoadJob>& outJobs)
{
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open table: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    unordered_map<uint32, bool> firstTextureByType;

    if (root.contains("Texture"))
    {
        for (const auto& item : root["Texture"])
        {
            string idStr = item.value("Id", string{});
            string pathStr = item.value("Path", string{});
            string levelStr = item.value("Level", "Static");
            int32  count = item.value("Count", 1);

            if (idStr.empty()) idStr = item.value("id", string{});
            if (pathStr.empty()) pathStr = item.value("path", string{});
            if (idStr.empty()) continue;

            uint32 typeId = Get_ComponentID_From_String(idStr);
            uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

            if (typeId == 0) continue;
            FLoadJob job;

            job.componentID = typeId;
            job.levelIndex = levelIndex;
            job.idStr = idStr;
            job.pathStr = pathStr;
            job.count = count;

            if (!firstTextureByType[typeId])
            {
                job.type = ELoadJobType::TextureCreate;
                firstTextureByType[typeId] = true;
            }
            else
            {
                job.type = ELoadJobType::TextureAppend;
            }

            outJobs.push_back(job);
        }
    }

    if (root.contains("Shader"))
    {
        for (const auto& item : root["Shader"])
        {
            string idStr = item.value("Id", string{});
            string pathStr = item.value("Path", string{});
            string layoutStr = item.value("Extra", string{}); 
            string levelStr = item.value("Level", "Static");
            int32  count = item.value("Count", 1);

            if (idStr.empty()) idStr = item.value("id", string{});
            if (pathStr.empty()) pathStr = item.value("path", string{});
            if (layoutStr.empty()) layoutStr = item.value("type", string{});
            if (idStr.empty()) continue;

            uint32 typeId = Get_ComponentID_From_String(idStr);
            uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

            if (typeId == 0) continue;
            FLoadJob job{};

            job.type = ELoadJobType::Shader;
            job.componentID = typeId;
            job.levelIndex = levelIndex;
            job.idStr = idStr;
            job.pathStr = pathStr;
            job.extraStr = layoutStr;
            job.count = count;
            outJobs.push_back(job);
        }
    }

    if (root.contains("Model"))
    {
        for (const auto& item : root["Model"])
        {
            string idStr = item.value("Id", string{});
            string pathStr = item.value("Path", string{});
            string guidStr = item.value("Guid", string{});
            string levelStr = item.value("Level", "Static");
            string modelTypeStr = item.value("ModelType", "Static");

            if (idStr.empty()) idStr = item.value("id", string{});
            if (pathStr.empty()) pathStr = item.value("path", string{});
            if (guidStr.empty()) guidStr = item.value("guid", string{});
            if (modelTypeStr.empty()) modelTypeStr = item.value("modelType", "Static");
            if (idStr.empty()) continue;

            if (!guidStr.empty())
                pathStr = Utils::ToString(GAME->Resolve_AssetPath(guidStr));

            uint32 typeId = Get_ComponentID_From_String(idStr);
            uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

            if (typeId == 0) continue;
            FLoadJob job{};

            job.type = ELoadJobType::Model;
            job.componentID = typeId;
            job.levelIndex = levelIndex;
            job.idStr = idStr;
            job.pathStr = pathStr;
            job.extraStr = modelTypeStr;
            job.isSkeletal = (modelTypeStr == "SkeletalMesh");

            outJobs.push_back(job);
        }
    }

    string skillKey = "";
    if (root.contains("DT_SkillData")) skillKey = "DT_SkillData";
    else if (root.contains("Skill"))   skillKey = "Skill";

    if (!skillKey.empty())
    {
        uint32 iconSrvIndex = 0;

        for (const auto& item : root[skillKey])
        {
            if (!item.contains("SkillID"))
                continue;

            FLoadJob job{};
            job.type = ELoadJobType::Skill;
            job.skillData.skill_Id = item.value("SkillID", 0);
            job.skillData.skillName = Utils::ToWString(item.value("SkillName", string{}));
            job.skillData.coolDown = item.value("Cooldown", 0.f);
            job.skillData.animStateName = item.value("AnimStateName", string{});
            job.skillData.loopDurationSec = item.value("LoopDurationSec", 0.f);

            const auto& holdValue = item["IsHoldSkill"];

            if (item.contains("IsHoldSkill"))
            {
                if (holdValue.is_boolean())
                    job.skillData.isHoldSkill = holdValue.get<bool>();
                else if (holdValue.is_number())
                    job.skillData.isHoldSkill = (holdValue.get<float>() != 0.f);
            }

			if (item.contains("hasDashPhase"))
			{
				const auto& dashValue = item["hasDashPhase"];
				if (dashValue.is_boolean())
					job.skillData.hasDashPhase = dashValue.get<bool>();
				else if (dashValue.is_number())
					job.skillData.hasDashPhase = (dashValue.get<float>() != 0.f);
			}

			job.skillData.dashSpeed = item.value("dashSpeed", 15.f);
			job.skillData.maxDashDistance = item.value("maxDashDistance", 20.f);
			job.skillData.targetStopDistance = item.value("targetStopDistance", 1.5f);
			job.skillData.attackEndAnimStateName = item.value("attackEndAnimStateName", string{});

            job.skillData.airAnimStateName = item.value("airAnimStateName", string{});

            if (item.contains("airGravityOff"))
            {
                const auto& gravityValue = item["airGravityOff"];
                if (gravityValue.is_boolean())
                    job.skillData.airGravityOff = gravityValue.get<bool>();
                else if (gravityValue.is_number())
                    job.skillData.airGravityOff = (gravityValue.get<float>() != 0.f);
            }

            job.skillIconSrvIndex = iconSrvIndex;

            outJobs.push_back(job);
            ++iconSrvIndex;
        }
    }

	string jsonKey = "";
	if (root.contains("DT_GameObject")) jsonKey = "DT_GameObject";
	else if (root.contains("GameObject")) jsonKey = "GameObject";

	if (!jsonKey.empty())
	{
		for (const auto& item : root[jsonKey])
		{
			string typeStr = item.value("ObjectType", string{});
			string levelStr = item.value("Level", "Static");

			const google::protobuf::EnumDescriptor* descriptor = Protocol::OBJECT_TYPE_descriptor();
			const google::protobuf::EnumValueDescriptor* valueDesc = descriptor->FindValueByName(typeStr);
			
			if (valueDesc == nullptr)
			{
				LOG_ERROR("엑셀 파싱 에러: {} 라는 ObjectType은 Protobuf Enum에 존재하지 않습니다! 오타를 확인하세요.", typeStr);
				continue; 
			}

			uint32 objType = static_cast<uint32>(valueDesc->number());
			uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

			FLoadJob job{};
			job.type = ELoadJobType::GameObjectPrototype;
			job.levelIndex = levelIndex;
			job.objectType = objType;
			outJobs.push_back(job); 
		}
	}

    LOG_INFO("Loaded integrated resources successfully: {}", Utils::ToString(tablePath));
    return S_OK;
}

HRESULT ResourceLoader::Load_Model(const json& data)
{
    for (const auto& item : data)
    {
        string idStr = item.value("Id", string{});
        if(idStr.empty()) idStr = item.value("id", string{});

        string guidStr = item.value("Guid", string{});
        if (guidStr.empty()) guidStr = item.value("guid", string{});

        string pathStr = item.value("Path", string{});
        if (pathStr.empty()) pathStr = item.value("path", string{});

        if(!guidStr.empty())
            pathStr = Utils::ToString(GAME->Resolve_AssetPath(guidStr));
        
        string levelStr = item.value("Level", string{});
        if (levelStr.empty()) levelStr = item.value("level", "Static");

        string modelTypeStr = item.value("ModelType", string{});
        if (modelTypeStr.empty()) modelTypeStr = item.value("modelType", "Static");
        EMeshVertexType modelType = (modelTypeStr == "SkeletalMesh") ? EMeshVertexType::SkeletalMesh : EMeshVertexType::StaticMesh;

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown Model ID: {}", idStr);
            continue;
        }

        GAME->Register_ComponentFactory(
            typeId, [pathStr, modelType](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                Matrix preTransform = Matrix::Identity;

                if (modelType == EMeshVertexType::SkeletalMesh)
                {
                    preTransform =
                        Matrix::CreateScale(0.01f) *
                        Matrix::CreateRotationX(XMConvertToRadians(90.f)) *
                        Matrix::CreateRotationY(XMConvertToRadians(180.f));
                }
                return Model::Create(device, context, modelType, pathStr, preTransform);
            },
            Utils::ToWString(idStr));

        GAME->Register_ComponentFactory_Prototype(typeId, levelIndex);

        LOG_INFO("Model registered: {}", idStr);
    }

    return S_OK;
}

HRESULT ResourceLoader::Load_Shaders(const json& data)
{
    for (const auto& item : data)
    {
        string idStr = item.value("Id", string{});
        if (idStr.empty()) idStr = item.value("id", string{});

        string pathStr = item.value("Path", string{});
        if (pathStr.empty()) pathStr = item.value("path", string{});

        string layoutStr = item.value("Extra", string{});
        if (layoutStr.empty()) layoutStr = item.value("type", string{});

        string levelStr = item.value("Level", string{});
        if (levelStr.empty()) levelStr = item.value("level", "Static");

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown Shader ID : {}", idStr);
            continue;
        }

        auto layout = Get_InputLayout(layoutStr);
        if (!layout.desc) continue;

        wstring wPath = Utils::ToWString(pathStr);
        auto desc = layout.desc;
        auto count = layout.count;

        GAME->Register_ComponentFactory(
            typeId,
            [wPath, desc, count](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                return Shader::Create(device, context, wPath, desc, count);
            },
            Utils::ToWString(idStr));

        GAME->Register_ComponentFactory_Prototype(typeId, levelIndex);

        LOG_INFO("Shader registered: {} ({})", idStr, layoutStr);
    }

    return S_OK;
}

HRESULT ResourceLoader::Load_Terrains(const json& data)
{
    for (const auto& item : data)
    {
        string idStr = item.value("Id", string{});
        if (idStr.empty()) idStr = item.value("id", string{});

        string pathStr = item.value("Path", string{});
        if (pathStr.empty()) pathStr = item.value("path", string{});

        string levelStr = item.value("Level", string{});
        if (levelStr.empty()) levelStr = item.value("level", "Static");

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown terrain ID: {}", idStr); continue;
        }

        wstring wPath = Utils::ToWString(pathStr);

        GAME->Register_ComponentFactory(
            typeId,
            [wPath](ComPtr<Device> device, ComPtr<DeviceContext> context)
            {
                return VIBuffer_Terrain::Create(device, context, wPath);
            },
            Utils::ToWString(idStr));

        GAME->Register_ComponentFactory_Prototype(typeId, levelIndex);

        LOG_INFO("Terrain registered: {}", idStr);
    }
    return S_OK;
}

HRESULT ResourceLoader::Load_Textures(const json& data)
{
    umap<uint32, Shared<Texture>> createdTextures;

    for (const auto& item : data)
    {
        string idStr = item.value("Id", string{});
        if (idStr.empty()) idStr = item.value("id", string{});

        string pathStr = item.value("Path", string{});
        if (pathStr.empty()) pathStr = item.value("path", string{});

        string levelStr = item.value("Level", string{});
        if (levelStr.empty()) levelStr = item.value("level", "Static");

        int32 count = item.value("Count", 0);
        if (count == 0) count = item.value("count", 1);

        uint32 typeId = Get_ComponentID_From_String(idStr);
        uint32 levelIndex = Get_LevelIndex_From_String(levelStr);

        if (typeId == 0)
        {
            LOG_WARN("Unknown component ID: {}", idStr);
            continue;
        }

        wstring wPath = Utils::ToWString(pathStr);

        // 이미 같은 ID로 만든 텍스처가 있는지 확인
        auto iter = createdTextures.find(typeId);

        if (iter == createdTextures.end())
        {
            // 첫 번째, 새로 만들어야함
            auto texture = Texture::Create(_device, _context, wPath.c_str(), count);
            if (!texture)
            {
                LOG_ERROR("Failed to load Texture: {}", idStr);
                continue;
            }

            if (FAILED(GAME->Add_Component_Prototype(levelIndex, typeId, texture)))
            {
                LOG_ERROR("Failed to load Texture: {}", idStr);
            }

            createdTextures[typeId] = texture;
        }
        else
        {
            auto& existingTexture = iter->second;
            if (pathStr.find("%d") != string::npos)
            {
                // count만큼 반복 추가
                for (int32 i = 0; i < count; i++)
                {
                    wchar_t fullPath[MAX_PATH] = {};
                    wsprintf(fullPath, wPath.c_str(), i);
                    if (FAILED(existingTexture->Add_SRV(fullPath)))
                        LOG_ERROR("Failed to add SRV: {}", Utils::ToString(fullPath));
                }
            }
            else
            {
                // 개별 파일은 하나만 추가
                if (FAILED(existingTexture->Add_SRV(wPath)))
                    LOG_ERROR("Failed to add SRV: {}", pathStr);
            }
        }
    }

    return S_OK;
}

HRESULT ResourceLoader::Load_Skills(const json& data)
{
    auto mgr = GET_SINGLE(SkillDataManager);

    mgr->Clear();

    uint32 iconSrvIndex = 0;

    for (const auto& item : data)
    {
        FSkillData skill;
        skill.skill_Id = item["SkillID"];
        skill.skillName = Utils::ToWString(item.value("SkillName", string{}));
        skill.coolDown = item.value("Cooldown", 0.f);

        mgr->Register_Skill(skill);
        mgr->Register_Skill_IconIndex(static_cast<int32>(skill.skill_Id), iconSrvIndex);

        ++iconSrvIndex;
    }
    return S_OK;
}

uint32 ResourceLoader::Get_ComponentID_From_String(const string& idStr)
{
    const google::protobuf::EnumDescriptor* descriptor = Protocol::ComponentID_descriptor();
    const google::protobuf::EnumValueDescriptor* valueDesc =
        descriptor->FindValueByName(idStr);

    if (valueDesc == nullptr)
    {
        // Proto에 없는 이름 ex) Model_Armor1과 가틍면 문자열 자체를 해싱해서 고유 ID로 세팅
        return static_cast<uint32>(std::hash<string>{}(idStr));
    }
        

    return static_cast<uint32>(valueDesc->number());
}

uint32 ResourceLoader::Get_LevelIndex_From_String(const string& levelName)
{
    auto levelEnum = magic_enum::enum_cast<ELevelType>(levelName);

    if (levelEnum.has_value())
        return ETOI(levelEnum.value());

    return ETOI(ELevelType::Static);
}

ResourceLoader::FInputLayoutInfo ResourceLoader::Get_InputLayout(const string& name)
{
    if (name == "VtxTex")
    {
        return { FVertexTex::Elements, FVertexTex::numElements };
    }

    if (name == "VtxNorTex")
    {
        return { FVertexNormalTex::Elements, FVertexNormalTex::numElements };
    }

    if (name == "VtxMesh")
    {
        return { FVertexMesh::Elements, FVertexMesh::numElements };
    }

    if (name == "VtxAnim")
    {
        return { FVertexAnimationMesh::Elements, FVertexAnimationMesh::numElements };
    }

    if (name == "VtxParticlePoint")
    {
        return { VTXPARTICLE_POINT_INSTANCE_DESC::Elements, VTXPARTICLE_POINT_INSTANCE_DESC::numElements };
    }

    return { nullptr, 0 };
}

shared_ptr<ResourceLoader> ResourceLoader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<ResourceLoader>(device, context);

    if (FAILED(instance->Initialize()))
    {
        LOG_ERROR("Failed to Create : Resource Loader");

        return nullptr;
    }

    return instance;
}

void ResourceLoader::Free()
{
    Base::Free();
}
