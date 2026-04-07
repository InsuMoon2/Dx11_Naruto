#include "pch.h"
#include "Level_CharacterSetup.h"
#include "UI_Text.h"
#include "Background.h"
#include "Camera_Target.h"
#include "Loader.h"
#include "GameInstance.h"
#include "Level_Loading.h"
#include "UI_MainTitleMenuButton.h"
#include "UI_TabButton.h"
#include "Character.h"
#include "Player.h"
#include "Camera_Free.h"
#include "Camera.h"
#include "NetworkManager.h"
#include "Spawn_Helper.h"
#include "Customizer_Manager.h"

// 카메라 줌인 프리셋
static const FCameraPreset s_CameraPresets[] =
{
    { Vec3(6.6f, 1.7f, 0.78f),   Vec3(9.6f, 169.f, 0.f) }, // Headgear
    { Vec3(6.6f, 1.7f, 0.78f),   Vec3(9.6f, 169.f, 0.f) }, // Face
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) }, // Onepiece
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) }, // BodyUpper
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) }, // BodyLower
    { Vec3(6.8f, 1.2f, 2.18f), Vec3(9.6f, 163.f, 0.f) }, // Accessory
};

Level_CharacterSetup::Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

HRESULT Level_CharacterSetup::Initialize()
{
    Build_PartCatalog();

    CHECK_FAILED(Ready_Layer_UI(), E_FAIL);
    CHECK_FAILED(Ready_PreviewScene(), E_FAIL);

    _selectedTabIndex = 0;
    _selectedOptionIndex = 0;

    Refresh_TabSelection();
    Refresh_SelectDescText();

    Refresh_UI_Visibility();

    if (!Get_SelectedOptions().empty())
    {
        Apply_SelectedOption();
    }

    return S_OK;
}

void Level_CharacterSetup::Update(float timeDelta)
{
    Level::Update(timeDelta);

    Update_CameraLerp(timeDelta);

    // 플레이어 회전
    Handle_RotationInput(timeDelta);

    if (_setupState == ESetupState::Category)
    {
        Handle_TabInput();
    }
    else if (_setupState == ESetupState::Item)
    {
        Handle_OptionInput();
    }
}

void Level_CharacterSetup::Late_Update(float timeDelta)
{
    Level::Late_Update(timeDelta);


}

HRESULT Level_CharacterSetup::Render()
{
    #ifdef _DEBUG
    SetWindowText(g_hWnd, TEXT("현재 레벨 : 파츠 선택"));
    #endif
    

    return S_OK;
}

HRESULT Level_CharacterSetup::Ready_Layer_UI()
{
    const Vec2 viewport = { GAME->Get_UIReferenceWidth(), GAME->Get_UIReferenceHeight() };

    // Background
    {
        Background::FBackgroundDesc desc{};
        desc.name = TEXT("CharacterSetup_Background");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y * 0.5f;
        desc.sizeX = viewport.x;
        desc.sizeY = viewport.y;

        desc.levelIndex = ETOI(ELevelType::CharacterSetup);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
        desc.textureIndex = ETOI(ECharacterSetupTexture::Background);

        desc.zOrder = 0.5f;
        
        auto mainTitle = static_pointer_cast<Background>(
            GAME->Add_UI(
                Protocol::OBJECT_TYPE_BACKGROUND,
                EUILayer::HUD,
                &desc));

        CHECK_NULL(mainTitle, E_FAIL);
        mainTitle->Set_RenderGroup(ERenderGroup::BackgroundUI);
    }

    // Window
    Background::FBackgroundDesc windowDesc{};
    windowDesc.name = TEXT("Setup_Window");
    windowDesc.posX = viewport.x * 0.3f - 60.f;
    windowDesc.posY = viewport.y * 0.5f;
    windowDesc.sizeX = 824.f * 0.8f;
    windowDesc.sizeY = 624.f * 0.8f;

    windowDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    windowDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    windowDesc.textureIndex = ETOI(ECharacterSetupTexture::Window);

    windowDesc.zOrder = 0.51f;

    auto window = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &windowDesc));

    if (!window)
        return E_FAIL;

    // WinTitle
    Background::FBackgroundDesc wintitleDesc{};
    wintitleDesc.name = TEXT("WinTitle");
    wintitleDesc.posX = windowDesc.posX;
    wintitleDesc.posY = windowDesc.posY - 200.f;
    wintitleDesc.sizeX = 792.f * 0.8f;
    wintitleDesc.sizeY = 62.f;

    wintitleDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    wintitleDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    wintitleDesc.textureIndex = ETOI(ECharacterSetupTexture::WinTitle);

    wintitleDesc.zOrder = 0.51f;

    wintitleDesc.textDesc.text = L"파츠 선택";
    wintitleDesc.textDesc.offset = Vec2(0.f, 0.f);
    wintitleDesc.textDesc.size = Vec2(420.f, 52.f);
    wintitleDesc.textDesc.zOrderOffset = 0.01f;
    wintitleDesc.textDesc.style.fontFamily = L"Malgun Gothic";
    wintitleDesc.textDesc.style.fontSize = 26.f;
    wintitleDesc.textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    wintitleDesc.textDesc.style.hAlign = ETextHAlign::Center;
    wintitleDesc.textDesc.style.vAlign = ETextVAlign::Middle;
    wintitleDesc.textDesc.style.wordWrap = false;

    auto winTitle = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &wintitleDesc));

    if (!winTitle)
        return E_FAIL;

    // Menu Button
    {
        const array<wstring, 6> menuLabels =
        {
            L"머리",
            L"얼굴장식",
            L"한벌옷",
            L"상의",
            L"하의",
            L"악세사리"
        };

        const float spacing = 15.f;
        const float tabWidth = 718.f * 0.75f;
        const float tabHeight = 64.f * 0.65f;

        for (int32 i = 0; i < 6; ++i)
        {
            UI_TabButton::FUITabDesc tabDesc{};
            tabDesc.name = ::format(L"TabButton {}", i);
            tabDesc.posX = windowDesc.posX;
            tabDesc.posY = windowDesc.posY + i * (tabHeight + spacing) - 140.f;
            tabDesc.sizeX = tabWidth;
            tabDesc.sizeY = tabHeight;

            tabDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
            tabDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
            tabDesc.textureIndex = ETOI(ECharacterSetupTexture::TabButton);
            tabDesc.selectedTextureIndex = ETOI(ECharacterSetupTexture::TabSelectedButton);

            tabDesc.zOrder = 0.52f + i * 0.001f;

            tabDesc.labelText = menuLabels[i];
            tabDesc.labelOffset = Vec2(0.f, 0.f);
            tabDesc.labelSize = Vec2(tabWidth - 20.f, tabHeight);
            tabDesc.fontSize = 24.f;

            _tabButtons[i] = static_pointer_cast<UI_TabButton>(
                GAME->Add_UI(
                    Protocol::OBJECT_TYPE_UI_TAB,
                    EUILayer::HUD,
                    &tabDesc));

            CHECK_NULL(_tabButtons[i], E_FAIL);
        }
    }

    // Select Button
    UI_TabButton::FUITabDesc selectButtonDesc{};
    selectButtonDesc.name = TEXT("SelectButton");
    selectButtonDesc.posX = windowDesc.posX;
    selectButtonDesc.posY = windowDesc.posY + 300.f;
    selectButtonDesc.sizeX = 556.f * 0.8f;
    selectButtonDesc.sizeY = 72.f;

    selectButtonDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    selectButtonDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    selectButtonDesc.textureIndex = ETOI(ECharacterSetupTexture::SelectButton);
    selectButtonDesc.selectedTextureIndex = ETOI(ECharacterSetupTexture::SelectedButton);
    selectButtonDesc.labelColor = Color(1.f, 1.f, 1.f, 1.f);

    selectButtonDesc.zOrder = 0.51f;

    selectButtonDesc.labelText = L"결정";
    selectButtonDesc.labelOffset = Vec2(0.f, 0.f);
    selectButtonDesc.labelSize = Vec2(420.f, 52.f);
    selectButtonDesc.fontSize = 26.f;

    _selectButton = static_pointer_cast<UI_TabButton>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_UI_TAB,
            EUILayer::HUD,
            &selectButtonDesc));

    CHECK_NULL(_selectButton, E_FAIL);

    // Select Desc
    Background::FBackgroundDesc selectDesc{};
    selectDesc.name = TEXT("Select Desc");
    selectDesc.sizeX = 664.f * 1.4f;
    selectDesc.sizeY = 46.f;
    selectDesc.posX = viewport.x * 0.5f;
    selectDesc.posY = viewport.y * 0.92f;

    selectDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    selectDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    selectDesc.textureIndex = ETOI(ECharacterSetupTexture::SelectDesc);

    selectDesc.zOrder = 0.51f;

    selectDesc.textDesc.text = L"탭 선택에 따라 텍스트 변경";
    selectDesc.textDesc.offset = Vec2(0.f, 0.f);
    selectDesc.textDesc.size = Vec2(420.f, 52.f);
    selectDesc.textDesc.zOrderOffset = 0.01f;
    selectDesc.textDesc.style.fontFamily = L"Malgun Gothic";
    selectDesc.textDesc.style.fontSize = 26.f;
    selectDesc.textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    selectDesc.textDesc.style.hAlign = ETextHAlign::Center;
    selectDesc.textDesc.style.vAlign = ETextVAlign::Middle;
    selectDesc.textDesc.style.wordWrap = false;

    _selectDescBg = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &selectDesc));

    CHECK_NULL(_selectDescBg, E_FAIL);

    // Title Bg
    Background::FBackgroundDesc titleBgDesc{};
    titleBgDesc.name = TEXT("Title BG");
    titleBgDesc.sizeX = 738.f;
    titleBgDesc.sizeY = 98.f;
    titleBgDesc.posX = titleBgDesc.sizeX * 0.5f;
    titleBgDesc.posY = windowDesc.posY - 375.f;

    titleBgDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    titleBgDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    titleBgDesc.textureIndex = ETOI(ECharacterSetupTexture::TitleBG);

    titleBgDesc.zOrder = 0.51f;

    auto titleBg = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &titleBgDesc));

    CHECK_NULL(titleBg, E_FAIL);

    // Title Symbol
    Background::FBackgroundDesc titleSymbolDesc{};
    titleSymbolDesc.name = TEXT("Title Symbol");
    titleSymbolDesc.posX = titleBgDesc.posX - 300.f;
    titleSymbolDesc.posY = titleBgDesc.posY;
    titleSymbolDesc.sizeX = 128.f;
    titleSymbolDesc.sizeY = 128.f;

    titleSymbolDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    titleSymbolDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    titleSymbolDesc.textureIndex = ETOI(ECharacterSetupTexture::Title_Symbol);

    titleSymbolDesc.zOrder = 0.51f;

    auto titleSymbol = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &titleSymbolDesc));

    CHECK_NULL(titleSymbol, E_FAIL);

    // 캐릭터 작성 텍스트
    Background::FBackgroundDesc charSetupTextDesc{};
    charSetupTextDesc.name = TEXT("Character Setup Text");
    charSetupTextDesc.posX = titleBgDesc.posX + 200.f;
    charSetupTextDesc.posY = titleBgDesc.posY;
    charSetupTextDesc.sizeX = 900.f;
    charSetupTextDesc.sizeY = 200.f;

    charSetupTextDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    charSetupTextDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    charSetupTextDesc.textureIndex = ETOI(ECharacterSetupTexture::CharacterSetupText);

    charSetupTextDesc.zOrder = 0.52f;

    auto charSetupText = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &charSetupTextDesc));

    CHECK_NULL(titleSymbol, E_FAIL);

    return S_OK;
}

HRESULT Level_CharacterSetup::Ready_PreviewScene()
{
    // 라이트
    {
        FLightDesc lightDesc{};
        lightDesc.type = ELightType::Directional;
        lightDesc.direction = Vec4(1.f, -1.f, 1.f, 0.f);
        lightDesc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
        lightDesc.ambient = Vec4(0.8f, 0.8f, 0.8f, 1.f);
        lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

        CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);
    }

    // 프리뷰 카메라
    {
        Camera_Target::FCameraTargetDesc cameraDesc{};
        cameraDesc.speedPerSec = 10.f;
        cameraDesc.rotationPerSec = 90.f;
        cameraDesc.eye = Vec3(0.f, 8.f, 12.f);
        cameraDesc.at = Vec3(0.f, 3.f, 0.f);
        cameraDesc.fovY = XMConvertToRadians(60.f);
        cameraDesc.nearZ = 0.1f;
        cameraDesc.farZ = 1000.f;
        cameraDesc.scale = Vec3(1.f, 1.f, 1.f);
        cameraDesc.offset = Vec3(0.f, 3.f, -6.f); 
        cameraDesc.enableMouseRotation = false;
        cameraDesc.bindOnPlayerSpawned = false;

        _previewCamera = static_pointer_cast<Camera_Free>(
            GAME->Clone_And_Add_GameObject(
            ETOI(ELevelType::Static),
            Protocol::OBJECT_TYPE_CAMERA_FREE,
            ETOI(ELevelType::CharacterSetup),
            TEXT("Layer_Camera"),
            &cameraDesc));

        if (_previewCamera)
        {
            _previewCamera->Get_Transform()->Set_LocalPosition(_cameraTargetPos);
            _previewCamera->Get_Transform()->Set_LocalRotation(9.6f, 163.f, 0.f);
        }
            
    }

    // 프리뷰 플레이어
    {
        auto previewObj = Spawn_Helper::Prefab("PreviewPlayer")
            .AtLevel(ETOI(ELevelType::CharacterSetup))
            .InLayer(TEXT("Layer_Player"))
            .Position(Vec3(6.5f, 0.f, 0.f))
            .Scale(Vec3(1.f, 1.f, 1.f))
            .Spawn();
                

        CHECK_NULL(previewObj, E_FAIL);

        _previewPlayer = dynamic_pointer_cast<Player>(previewObj);
        CHECK_NULL(_previewPlayer, E_FAIL);
    }

    return S_OK;
}

void Level_CharacterSetup::Build_PartCatalog()
{
    // 실제로 존재하는 리소스만 카탈로그에 넣기. 일단 테스트용은 하드코딩
    _catalog[ETOI(ContainerObject::EPartSlot::Headegear)] =
    {
        { L"기본 모자", L"Model_Headgear_Man_Cap1" }
    };

    _catalog[ETOI(ContainerObject::EPartSlot::Face)] =
    {
        { L"기본 얼굴장식", L"Model_Face_Face1" }
    };

    _catalog[ETOI(ContainerObject::EPartSlot::Onepiece)] =
    {
        { L"한벌옷 1", L"Model_Body_Upper_Armor1" },
        { L"코트 1",  L"Model_Body_Upper_Coat15" }
    };
}

void Level_CharacterSetup::Build_OptionButtons()
{
    for (auto& button : _optionButtons)
    {
        if (button)
            CHECK_FAILED(button->Remove_FromUIManager());
    }

    _optionButtons.clear();

    const auto& options = Get_SelectedOptions();
    float startX = 0.f;

    float startY = 0.f;

    if (_tabButtons[0] != nullptr)
    {
        startX = _tabButtons[0]->Get_UIPosX(); 
        startY = _tabButtons[0]->Get_UIPosY(); 
    }

    const float width = 718.f * 0.75f;
    const float height = 64.f * 0.65f;
    const float spacing = 15.f;

    for (int32 i = 0; i < static_cast<int32>(options.size()); ++i)
    {
        UI_TabButton::FUITabDesc desc{};
        desc.name = format(L"OptionButton {}", i);

        desc.posX = startX;
        desc.posY = startY + i * (height + spacing);
        desc.sizeX = width;
        desc.sizeY = height;

        desc.levelIndex = ETOI(ELevelType::CharacterSetup);
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
        desc.textureIndex = ETOI(ECharacterSetupTexture::TabButton);
        desc.selectedTextureIndex = ETOI(ECharacterSetupTexture::TabSelectedButton);

        desc.zOrder = 0.52f + i * 0.001f;
        desc.labelText = options[i].displayName;
        desc.labelSize = Vec2(width - 20.f, height);
        desc.fontSize = 24.f;

        auto button = static_pointer_cast<UI_TabButton>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TAB, EUILayer::HUD, &desc));

        if (button)
            _optionButtons.push_back(button);
    }

    _selectedOptionIndex = 0;
}

void Level_CharacterSetup::Refresh_TabSelection()
{
    for (int32 i = 0; i < static_cast<int32>(TAB_COUNT); ++i)
    {
        if (_tabButtons[i])
            _tabButtons[i]->Set_Selected(i == _selectedTabIndex);
    }

    if (_selectButton)
        _selectButton->Set_Selected(_selectedTabIndex == static_cast<int32>(TAB_COUNT));
}

void Level_CharacterSetup::Refresh_OptionSelection()
{
    for (int32 i = 0; i < static_cast<int32>(_optionButtons.size()); ++i)
    {
        if (_optionButtons[i])
            _optionButtons[i]->Set_Selected(i == _selectedOptionIndex);
    }

    if (_selectButton)
    {
        _selectButton->Set_Selected(_selectedOptionIndex == static_cast<int32>(Get_SelectedOptions().size()));
    }
}

void Level_CharacterSetup::Refresh_SelectDescText()
{
    if (!_selectDescBg)
        return;

    bool isSelectButtonFocused = false;

    if (_setupState == ESetupState::Category && _selectedTabIndex == static_cast<uint32>(TAB_COUNT))
        isSelectButtonFocused = true;

    else if (_setupState == ESetupState::Item && _selectedOptionIndex == static_cast<int32>(Get_SelectedOptions().size()))
        isSelectButtonFocused = true;

    if (isSelectButtonFocused)
    {
        _selectDescBg->Set_LabelText(TEXT("캐릭터 생성을 완료합니다."));
    }
    else
    {
        _selectDescBg->Set_LabelText(Get_SelectDescText(Get_SelectSlot()));
    }
}

void Level_CharacterSetup::Handle_TabInput()
{
    // 탭 6개 + 버튼 1개
    const int32 tabCount = static_cast<int32>(TAB_COUNT) + 1;

    if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
    {
        _selectedTabIndex = (_selectedTabIndex - 1 + tabCount) % tabCount;

        Refresh_TabSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        _selectedTabIndex = (_selectedTabIndex + 1) % tabCount;
        Refresh_TabSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        if (_selectedTabIndex == static_cast<int32>(TAB_COUNT))
        {
            Finish_CharacterSetup();

            return;
        }

        _setupState = ESetupState::Item;

        Build_OptionButtons();
        Refresh_UI_Visibility();
        Apply_CameraPreset(_selectedTabIndex);

        const auto& options = Get_SelectedOptions();

        if (!options.empty())
        {
            _selectedOptionIndex = 0;
        }

        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }
}

void Level_CharacterSetup::Handle_OptionInput()
{
    if (INPUT->KeyDown(KEY_TYPE::ESCAPE) || INPUT->KeyDown(KEY_TYPE::BACK))
    {
        _setupState = ESetupState::Category;
        Refresh_UI_Visibility();

        Refresh_TabSelection();
        Refresh_SelectDescText();
    }

    

    const auto& options = Get_SelectedOptions();

    if (options.empty())
        return;

    const int32 optionCount = static_cast<int32>(options.size());
    const int32 totalCount = optionCount + 1;

    if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
    {
        _selectedOptionIndex = (_selectedOptionIndex - 1 + totalCount) % totalCount;
        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        _selectedOptionIndex = (_selectedOptionIndex + 1 + totalCount) % totalCount;
        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        if (_selectedOptionIndex == optionCount)
        {
            Finish_CharacterSetup();
            return;
        }

        Apply_SelectedOption();
    }
}

void Level_CharacterSetup::Handle_RotationInput(float timeDelta)
{
    if (!_previewPlayer)
        return;

    if (!GAME->Is_GameInputEnabled())
    {
        _isDragging = false;
        return;
    }

    if (INPUT->KeyDown(KEY_TYPE::LBUTTON))
    {
        _isDragging = true;
    }

    if (INPUT->KeyUp(KEY_TYPE::LBUTTON))
    {
        _isDragging = false;
    }

    if (_isDragging && INPUT->KeyPress(KEY_TYPE::LBUTTON))
    {
        Vec2 mouseDelta = INPUT->GetMouseDelta();

        // x축으로 이동량이 있을 때만
        if (std::abs(mouseDelta.x) > 0.001f)
        {
            auto transform = _previewPlayer->Get_Transform();

            float rotateDegress = -mouseDelta.x * _rotSensitivity;

            transform->Rotate_Axis(Vec3::Up, rotateDegress);
        }
    }

}

void Level_CharacterSetup::Apply_SelectedOption()
{
    if (!_previewPlayer)
        return;

    const auto& options = Get_SelectedOptions();
    if (options.empty())
        return;

    if (_selectedOptionIndex < 0 || _selectedOptionIndex >= static_cast<int32>(options.size()))
        return;

    const auto& option = options[_selectedOptionIndex];
    CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(Get_SelectSlot(), option.modelAssetTag));

    _equippedIndices[_selectedTabIndex] = _selectedOptionIndex;
}

void Level_CharacterSetup::Apply_TabSelection()
{
    Refresh_TabSelection();
    Build_OptionButtons();
    Refresh_OptionSelection();
    Refresh_SelectDescText();
    Apply_SelectedOption();
}

void Level_CharacterSetup::Update_CameraLerp(float timeDelta)
{
    if (!_previewCamera)
        return;

    auto transform = _previewCamera->Get_Transform();

    Vec3 currentPos = transform->Get_LocalPosition();
    Vec3 newPos = Vec3::Lerp(currentPos, _cameraTargetPos, timeDelta * _cameraLerpSpeed);

    transform->Set_LocalPosition(newPos);
    transform->Set_LocalRotation(_cameraTargetRot.x, _cameraTargetRot.y, _cameraTargetRot.z);
}

void Level_CharacterSetup::Apply_CameraPreset(int32 tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int32>(TAB_COUNT))
        return;

    _cameraTargetPos  = s_CameraPresets[tabIndex].position;
    _cameraTargetRot = s_CameraPresets[tabIndex].rotation;
}

ContainerObject::EPartSlot Level_CharacterSetup::Get_SelectSlot() const
{
    return _tabSlots[_selectedTabIndex];
}

const vector<Level_CharacterSetup::FCustomizeOption>& Level_CharacterSetup::Get_SelectedOptions() const
{
    return _catalog[ETOI(Get_SelectSlot())];
}

const tchar* Level_CharacterSetup::Get_SlotLabel(ContainerObject::EPartSlot slot) const
{
    switch (slot)
    {
    case ContainerObject::EPartSlot::Headegear: return L"머리";
    case ContainerObject::EPartSlot::Face:      return L"얼굴장식";
    case ContainerObject::EPartSlot::Onepiece:  return L"한벌옷";
    case ContainerObject::EPartSlot::BodyUpper: return L"상의";
    case ContainerObject::EPartSlot::BodyLower: return L"하의";
    case ContainerObject::EPartSlot::Accessory: return L"악세사리";
    default:                                    return L"알 수 없음";
    }
}

const tchar* Level_CharacterSetup::Get_SelectDescText(ContainerObject::EPartSlot slot)
{
    switch (slot)
    {
    case ContainerObject::EPartSlot::Headegear:
        return TEXT("머리 선택중입니다.");

    case ContainerObject::EPartSlot::Face:
        return TEXT("얼굴장식 선택중입니다.");

    case ContainerObject::EPartSlot::Onepiece:
        return TEXT("한벌옷 선택중입니다.");

    case ContainerObject::EPartSlot::BodyUpper:
        return TEXT("상의 선택중입니다.");

    case ContainerObject::EPartSlot::BodyLower:
        return TEXT("하의 선택중입니다.");

    case ContainerObject::EPartSlot::Accessory:
        return TEXT("악세사리 선택중입니다.");

    default:
        return TEXT("파츠 선택중입니다.");
    }
}

void Level_CharacterSetup::Refresh_UI_Visibility()
{
    bool isCategory = (_setupState == ESetupState::Category);

    for (int32 i = 0; i < static_cast<int32>(TAB_COUNT); ++i)
    {
        if (_tabButtons[i])
        {
            _tabButtons[i]->Set_Visibility(isCategory);
        }
    }

    if (isCategory)
    {
        for (auto button : _optionButtons)
        {
            if (button)
            {
                button->Remove_FromUIManager();
            }
        }

        _optionButtons.clear();
    }

}

void Level_CharacterSetup::Finish_CharacterSetup()
{
    auto custom = GET_SINGLE(Customizer_Manager);

    // 인게임에 가져갈 수 있도록 게임인스턴스에 저장
    for (int i = 0; i < TAB_COUNT; ++i)
    {
        ContainerObject::EPartSlot slot = _tabSlots[i];
        if (!_catalog[ETOI(slot)].empty())
        {
            const wstring& assetTag = _catalog[ETOI(slot)][_equippedIndices[i]].modelAssetTag;
            custom->Set_Part(slot, assetTag);
        }
    }

    // 에디터면 싱글, 서버가 실제 연결되어 있으면 멀티플레이 판정
    const bool isServerConnected = NetworkManager::GetInstance()->IsConnected();
    EGameplaySpawnMode spawnMode =
        (GAME->Is_EditorRuntime() || !isServerConnected) ? EGameplaySpawnMode::LocalOnly
                                                         : EGameplaySpawnMode::Server;

    if (spawnMode == EGameplaySpawnMode::Server)
    {
        if (!NetworkManager::GetInstance()->IsNetworkEnabled())
        {
            NetworkManager::GetInstance()->Initialize();
        }
    }

    GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, ELevelType::GamePlay, true, spawnMode));
}

Shared<Level_CharacterSetup> Level_CharacterSetup::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Level_CharacterSetup>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Level_CharacterSetup");

        return nullptr;
    }

    return instance;
}

void Level_CharacterSetup::Free()
{
    Level::Free();

}

