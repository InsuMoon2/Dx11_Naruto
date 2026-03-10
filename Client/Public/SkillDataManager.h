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
    void                Clear();

private:
    umap<int32, FSkillData> _skillMap;

public:
    void Free() override;

};

