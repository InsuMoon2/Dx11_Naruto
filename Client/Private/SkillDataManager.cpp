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

void SkillDataManager::Register_Skill_IconIndex(int32 skill_Id, uint32 iconSrvIndex)
{
    if (skill_Id == 0)
        return;

    _skill_IconSrvIndexMap[skill_Id] = iconSrvIndex;
}

uint32 SkillDataManager::Get_SkillIconSrvIndex(int32 skill_Id) const
{
    auto iter = _skill_IconSrvIndexMap.find(skill_Id);
    if (iter == _skill_IconSrvIndexMap.end())
    {
        // 잘못된 SkillID면 기본 아이콘 0번으로 세팅 -> 0번 아마 지금은 나선환
        return 0;
    }

    return iter->second;
}

void SkillDataManager::Clear()
{
    _skillMap.clear();
    _skill_IconSrvIndexMap.clear();
}

void SkillDataManager::Free()
{
    _skillMap.clear();
    _skill_IconSrvIndexMap.clear();

    Base::Free();
}
