#pragma once

#include "Base.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class Player;

struct FCustomizerDesc
{
    array<wstring, ETOI(ContainerObject::EPartSlot::END)> partTags{};

    // 기본 커스터마이징 프리셋
    void Reset_ToDefault()
    {
        for (auto& tag : partTags)
            tag.clear();

        partTags[ETOI(ContainerObject::EPartSlot::Headegear)] = TEXT("Model_Headgear_Man_Cap1");
        partTags[ETOI(ContainerObject::EPartSlot::Face)] = TEXT("Model_Face_Face1");
        partTags[ETOI(ContainerObject::EPartSlot::Onepiece)] = TEXT("Model_Body_Upper_Coat15");
    }

    void Set_Part(ContainerObject::EPartSlot slot, const wstring& modelAssetTag)
    {
        partTags[ETOI(slot)] = modelAssetTag;
    }

    const wstring& Get_Part(ContainerObject::EPartSlot slot) const
    {
        return partTags[ETOI(slot)];
    }

};

class Customizer_Manager : public Base
{
public:
    Customizer_Manager();
    ~Customizer_Manager() override = default;

public:
    void    Reset_ToDefault();
    void    Set_Part(ContainerObject::EPartSlot slot, const wstring& modelAssetTag);
    const FCustomizerDesc& Get_CustomizerDesc() const { return _customizerDesc; }

private:
    FCustomizerDesc _customizerDesc{};

public:
    static Unique<Customizer_Manager> Create();
    void Free() override;
};

NS_END
