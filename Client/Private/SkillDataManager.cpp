#include "pch.h"
#include "SkillDataManager.h"   

IMPLEMENT_SINGLETON(SkillDataManager)

bool SkillDataManager::Register_Skill(const FSkillData& data)
{
    if (data.skill_Id == 0)
        return false;

    auto [iter, inserted] = _skillMap.try_emplace(data.skill_Id, data);

    if (!inserted)
        iter->second = data;

    return true;
}

const FSkillData* SkillDataManager::Get_SkillData(int32 id) const
{
    auto iter = _skillMap.find(id);
    if (iter == _skillMap.end())
        return nullptr;

    return &iter->second;
}

bool SkillDataManager::Has_Skill(int32 id) const
{
    return _skillMap.find(id) != _skillMap.end();
}

void SkillDataManager::Clear()
{
    _skillMap.clear();
}

void SkillDataManager::Free()
{
    _skillMap.clear();

    Base::Free();
}
