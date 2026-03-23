#pragma once

#include "Level.h"
#include "ContainerObject.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Background;
class UI_TabButton;
class Player;

enum class ECharacterSetupTexture
{
    Background,
    Window,
    WinTitle,
    SelectDesc,
    TitleBG,
    Title_Symbol,
    SelectButton,
    TabButton,
    SelectedButton,

    END
};

class Level_CharacterSetup final : public Level
{
public:
    struct FCustomizeOption
    {
        wstring displayName;
        wstring modelAssetTag;
    };

public:
    explicit         Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual         ~Level_CharacterSetup() = default;

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_UI();
    HRESULT         Ready_PreviewScene();

private:
    void            Build_PartCatalog();
    void            Build_OptionButtons();
    void            Refresh_TabSelection();
    void            Refresh_OptionSelection();
    void            Refresh_SelectDescText();
    void            Handle_TabInput();
    void            Handle_OptionInput();
    void            Apply_SelectedOption();

    void            Apply_TabSelection();

    ContainerObject::EPartSlot Get_SelectSlot() const;
    const vector<FCustomizeOption>& Get_SelectedOptions() const;
    const tchar* Get_SlotLabel(ContainerObject::EPartSlot slot) const;

    const tchar* Get_SelectDescText(ContainerObject::EPartSlot slot);

private:
    static constexpr uint32 TAB_COUNT = 6;

    array<ContainerObject::EPartSlot, TAB_COUNT> _tabSlots =
    {
        ContainerObject::EPartSlot::Headegear,
        ContainerObject::EPartSlot::Face,
        ContainerObject::EPartSlot::Onepiece,
        ContainerObject::EPartSlot::BodyUpper,
        ContainerObject::EPartSlot::BodyLower,
        ContainerObject::EPartSlot::Accessory
    };

    array<Shared<UI_TabButton>, 6> _tabButtons {};
    vector<Shared<UI_TabButton>> _optionButtons {};
    array<vector<FCustomizeOption>, ETOI(ContainerObject::EPartSlot::END)> _catalog{};

    Shared<Background>  _selectDescBg;
    Shared<Player>      _previewPlayer;

    // 파츠 선택 인덱스
    int32 _selectedTabIndex = 0;
    int32 _selectedOptionIndex = 0;

public:
    static shared_ptr<Level_CharacterSetup> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};


NS_END
