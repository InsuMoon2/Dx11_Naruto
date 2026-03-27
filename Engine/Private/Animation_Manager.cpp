#include "pch.h"
#include "Animation_Manager.h"
#include "Animation.h"
#include "Model_BinaryLoader.h"
#include "Utils.h"

Animation_Manager::Animation_Manager()
{
}

HRESULT Animation_Manager::Initialize(const wstring& directoryPath)
{
    Load_Animations_From_Directory(directoryPath);

    return S_OK;
}

void Animation_Manager::Load_Animations_From_Directory(const wstring& directoryPath)
{
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath))
    {
        LOG_WARN("Animation directory not found: {}", Utils::ToString(directoryPath));
        return;
    }

    Model_BinaryLoader loader;
    uint32 loadedCount = 0;

    // 디렉토리 내 모든 파일 찾기
    for (const auto& entry : fs::recursive_directory_iterator(directoryPath))
    {
        if (entry.path().extension() == ".animbin")
        {
            vector<Shared<Animation>> tempAnims;

            if (loader.Load_AnimationOnly(entry.path().string(), tempAnims))
            {
                for (auto& anim : tempAnims)
                {
                    if (anim)
                    {
                        const string& animName = anim->Get_Name();

                        anim->Set_SourcePath(entry.path().string());

                        // 캐시에 저장
                        _animations[animName] = anim;
                        loadedCount++;
                    }
                }
            }
        }
    }

    LOG_INFO("Animation_Manager: Successfully loaded {} animations from {}", loadedCount, Utils::ToString(directoryPath));
}

Shared<Animation> Animation_Manager::Get_Animation(const string& name) const
{
    auto it = _animations.find(name);
    if (it != _animations.end())
        return it->second;

    return nullptr;
}

vector<Shared<Animation>> Animation_Manager::Get_Animations_By_Prefix(const string& prefix) const
{
    vector<Shared<Animation>> result;
    result.reserve(32);

    for (const auto& pair : _animations)
    {
        if (pair.first.find(prefix) == 0)
        {
            result.push_back(pair.second);
        }
    }

    return result;
}

vector<Shared<Animation>> Animation_Manager::Get_All_Animations() const
{
    vector<Shared<Animation>> result;
    result.reserve(_animations.size());

    for (const auto& pair : _animations)
    {
        result.push_back(pair.second);
    }

    return result;
}

vector<Shared<Animation>> Animation_Manager::Get_Animations_InFolder(const string& folderPath)
{
    vector<Shared<Animation>> result;
    result.reserve(64);

    if (folderPath.empty()) return result;

    fs::path normalizedFolder = fs::absolute(folderPath).lexically_normal();
    string folderStr = Utils::ToLowerCopy(normalizedFolder.string());

    for (const auto& [name, anim] : _animations)
    {
        const string& sourcePath = anim->Get_SourcePath();
        if (sourcePath.empty())
            continue;
        
        fs::path animFolder = fs::absolute(sourcePath).parent_path().lexically_normal();
        string animFolderStr = Utils::ToLowerCopy(animFolder.string());

        if (animFolderStr.find(folderStr) == 0)
        {
            result.push_back(anim);
        }
    }
    return result;
}

Unique<Animation_Manager> Animation_Manager::Create(const wstring& directoryPath)
{
    auto instance = make_unique<Animation_Manager>();

    if (FAILED(instance->Initialize(directoryPath)))
    {
        LOG_ERROR("Failed to create Animation_Manager");
        return nullptr;
    }
    return instance;
}

void Animation_Manager::Free()
{
    Base::Free();

    _animations.clear();
}
