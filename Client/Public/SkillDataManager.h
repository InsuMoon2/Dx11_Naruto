#pragma once

#include "Base.h"

class SkillDataManager : public Base
{
    DECLARE_SINGLETON(SkillDataManager)

public:
    SkillDataManager() = default;
    ~SkillDataManager() override = default;

public:
    bool                Register_Skill(const FSkillData& data);
    const FSkillData*   Get_SkillData(int32 id) const;
    bool                Has_Skill(int32 id) const;

    void                Register_Skill_IconIndex(int32 skill_Id, uint32 iconSrvIndex);
    uint32              Get_SkillIconSrvIndex(int32 skill_Id) const;
    const FSkillData*   Find_SkillByCategoryAndSlot(ESkillCategory category, int32 uiSlotIndex) const;

    void                Clear();


private:
    umap<int32, FSkillData> _skillMap;
    umap<int32, uint32>     _skill_IconSrvIndexMap;

public:
    void Free() override;

};

