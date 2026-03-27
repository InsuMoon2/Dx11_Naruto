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
class Camera_Free;

enum class ESetupState { Category, Item, NameInput };

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
    TabSelectedButton,
    SelectedButton,

    CharacterSetupText,

    PlayerTextBG,
    PlayerTextInput,

    END
};

// 선택된 탭 별로 카메라 위치 설정
struct FCameraPreset
{
    Vec3 position;
    Vec3 rotation;
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
    void            Handle_RotationInput(float timeDelta);

    void            Apply_SelectedOption();
    void            Apply_TabSelection();

    void            Update_CameraLerp(float timeDelta);
    void            Apply_CameraPreset(int32 tabIndex);

    ContainerObject::EPartSlot      Get_SelectSlot() const;
    const vector<FCustomizeOption>& Get_SelectedOptions() const;
    const tchar*                    Get_SlotLabel(ContainerObject::EPartSlot slot) const;
    const tchar*                    Get_SelectDescText(ContainerObject::EPartSlot slot);

    void            Refresh_UI_Visibility();
    void            Finish_CharacterSetup();

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

    Shared<UI_TabButton> _selectButton;

    // 파츠 선택 인덱스
    int32 _selectedTabIndex = 0;
    int32 _selectedOptionIndex = 0;

    // 마지막으로 고른 파츠 인덱스 번호
    array<int32, TAB_COUNT> _equippedIndices = { 0, 0, 0, 0, 0, 0 };

    // 카메라 이동
    Shared<Camera_Free> _previewCamera;
    Vec3                _cameraTargetPos = Vec3(6.8f, 1.2f, 2.18f);
    Vec3                _cameraTargetRot = Vec3(9.6f, 163.f, 0.f);
    float               _cameraLerpSpeed = 5.f;

private:
    ESetupState         _setupState = ESetupState::Category;

    bool                _isDragging = false;
    float               _rotSensitivity = 0.4f;

private: /* Local Player Name */
    wstring             _playerName = L"";
    Shared<Background>  _nameInputBg;
    Shared<UI_Text>     _nameInputText;

public:
    static shared_ptr<Level_CharacterSetup> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};


NS_END
