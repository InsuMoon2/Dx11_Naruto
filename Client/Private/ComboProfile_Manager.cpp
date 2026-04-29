#include "pch.h"
#include "ComboProfile_Manager.h"

#include <fstream>

IMPLEMENT_SINGLETON(ComboProfile_Manager)

bool ComboProfile_Manager::Load_FromJson(const string& filePath)
{
    std::ifstream ifs(filePath);
    if (!ifs.is_open())
    {
        LOG_ERROR("ComboProfileManager: JSON 파일 열기 실패 — {}", filePath);
        return false;
    }

    json root;
    ifs >> root;

    if (!root.contains("DT_ComboProfile") || !root["DT_ComboProfile"].is_array())
    {
        LOG_ERROR("ComboProfileManager: 'DT_ComboProfile' 배열 없음");
        return false;
    }

    for (const auto& item : root["DT_ComboProfile"])
    {
        string profileTypeStr = item.value("profileType", "Hand_Ground");
        auto opt = magic_enum::enum_cast<EAttackProfileType>(profileTypeStr);
        EAttackProfileType profileType = opt.value_or(EAttackProfileType::Hand_Ground);

        FComboProfile& profile = _profileMap[ETOI(profileType)];

        if (profile.profileName.empty())
        {
            profile.profileName = item.value("profileName", "");
            profile.profileType = profileType;

            string weaponStr = item.value("weaponType", "Hand");
            auto wopt = magic_enum::enum_cast<EWeaponType>(weaponStr);
            profile.weaponType = wopt.value_or(EWeaponType::Hand);

            profile.isAerial = item.value("isAerial", false);
        }

        // 해당 인덱스의 콤보(Entry) 데이터 파싱
        int32 comboIndex = item.value("comboIndex", -1);
        if (comboIndex < 0)
            continue;

        if (profile.combos.size() <= comboIndex)
            profile.combos.resize(comboIndex + 1);

        FComboEntry entry;
        entry.animStateKey = item.value("animStateKey", "");
        entry.damageMultiplier = item.value("damageMultiplier", 61.f);
        entry.canCancel = item.value("canCancel", true);

        entry.launchPower = item.value("launchPower", 0.f);
        entry.launchUp = item.value("launchUp", 0.f);
        entry.hitSound = 0;
        entry.hitSoundFile.clear();
        if (item.contains("hitSound"))
        {
            const json& hitSoundValue = item["hitSound"];
            if (hitSoundValue.is_number_integer())
            {
                entry.hitSound = hitSoundValue.get<int32>();
            }
            else if (hitSoundValue.is_string())
            {
                entry.hitSoundFile = hitSoundValue.get<string>();
            }
        }

        const string hitReactionTypeStr = item.value("hitReactionType", "Default");
        const auto hitReactionOpt = magic_enum::enum_cast<EHitReactionType>(hitReactionTypeStr);
        entry.hitReactionType = hitReactionOpt.value_or(EHitReactionType::Default);

        // 지정된 콤보 인덱스 자리에 세팅
        profile.combos[comboIndex] = entry;
    }

    for (auto& [key, profile] : _profileMap)
    {
        profile.maxCombo = static_cast<int32>(profile.combos.size());
        LOG_INFO("ComboProfile 로드: {} (combos={})", profile.profileName, profile.maxCombo);
    }

    return true;
}


const FComboProfile* ComboProfile_Manager::Find(EAttackProfileType profileType) const
{
    auto iter = _profileMap.find(ETOI(profileType));
    if (iter == _profileMap.end())
        return nullptr;

    return &iter->second;
}

const FComboProfile* ComboProfile_Manager::Find(EWeaponType weaponType, bool isAerial) const
{
    for (const auto& [key, profile] : _profileMap)
    {
        if (profile.weaponType == weaponType && profile.isAerial == isAerial)
        {
            return &profile;
        }
    }

    return nullptr;
}

void ComboProfile_Manager::Clear()
{
    _profileMap.clear();
}

void ComboProfile_Manager::Free()
{
    _profileMap.clear();
    Base::Free();
}
