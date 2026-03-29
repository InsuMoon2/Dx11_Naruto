#pragma once

#include "Base.h"
#include "Client_Struct.h"

class ComboProfile_Manager : public Base
{
    DECLARE_SINGLETON(ComboProfile_Manager)

public:
    ComboProfile_Manager() = default;
    ~ComboProfile_Manager() override = default;

public:
    bool Load_FromJson(const string& filePath);

    const FComboProfile* Find(EAttackProfileType profileType) const;
    const FComboProfile* Find(EWeaponType weaponType, bool isAerial) const;

    void Clear();

private:
    umap<int32, FComboProfile> _profileMap;

public:
    void Free() override;
};

