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
    Build_OptionButtons();
    Refresh_OptionSelection();
    Refresh_SelectDescText();

    if (!Get_SelectedOptions().empty())
    {
        Apply_SelectedOption();
    }

    return S_OK;
}

void Level_CharacterSetup::Update(float timeDelta)
{
    Level::Update(timeDelta);

    Handle_TabInput();
    Handle_OptionInput();
    
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

        if (!mainTitle)
            return E_FAIL;
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
            tabDesc.selectedTextureIndex = ETOI(ECharacterSetupTexture::SelectedButton);

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
    Background::FBackgroundDesc selectButtonDesc{};
    selectButtonDesc.name = TEXT("SelectButton");
    selectButtonDesc.posX = windowDesc.posX;
    selectButtonDesc.posY = windowDesc.posY + 300.f;
    selectButtonDesc.sizeX = 556.f * 0.8f;
    selectButtonDesc.sizeY = 72.f;

    selectButtonDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    selectButtonDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    selectButtonDesc.textureIndex = ETOI(ECharacterSetupTexture::SelectButton);

    selectButtonDesc.zOrder = 0.51f;

    selectButtonDesc.textDesc.text = L"결정";
    selectButtonDesc.textDesc.offset = Vec2(0.f, 0.f);
    selectButtonDesc.textDesc.size = Vec2(420.f, 52.f);
    selectButtonDesc.textDesc.zOrderOffset = 0.01f;
    selectButtonDesc.textDesc.style.fontFamily = L"Malgun Gothic";
    selectButtonDesc.textDesc.style.fontSize = 26.f;
    selectButtonDesc.textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    selectButtonDesc.textDesc.style.hAlign = ETextHAlign::Center;
    selectButtonDesc.textDesc.style.vAlign = ETextVAlign::Middle;
    selectButtonDesc.textDesc.style.wordWrap = false;

    auto selectButton = static_pointer_cast<Background>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &selectButtonDesc));

    CHECK_NULL(selectButton, E_FAIL);

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
    selectDesc.textDesc.style.hAlign = ETextHAlign::Left;
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

    titleBgDesc.textDesc.text = L"캐릭터 작성";
    titleBgDesc.textDesc.offset = Vec2(0.f, 0.f);
    titleBgDesc.textDesc.size = Vec2(420.f, 52.f);
    titleBgDesc.textDesc.zOrderOffset = 0.01f;
    titleBgDesc.textDesc.style.fontFamily = L"Malgun Gothic";
    titleBgDesc.textDesc.style.fontSize = 26.f;
    titleBgDesc.textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    titleBgDesc.textDesc.style.hAlign = ETextHAlign::Left;
    titleBgDesc.textDesc.style.vAlign = ETextVAlign::Middle;
    titleBgDesc.textDesc.style.wordWrap = false;

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
        cameraDesc.eye = Vec3(0.f, 8.f, -12.f);
        cameraDesc.at = Vec3(0.f, 3.f, 0.f);
        cameraDesc.fovY = XMConvertToRadians(60.f);
        cameraDesc.nearZ = 0.1f;
        cameraDesc.farZ = 1000.f;
        cameraDesc.scale = Vec3(1.f, 1.f, 1.f);
        cameraDesc.offset = Vec3(0.f, 3.f, -6.f);
        cameraDesc.followSpeed = 5.f;
        cameraDesc.enableMouseRotation = false;
        cameraDesc.bindOnPlayerSpawned = true;

        CHECK_FAILED(
            GAME->Add_GameObject(
                ETOI(ELevelType::CharacterSetup),
                Protocol::OBJECT_TYPE_CAMERA_TARGET,
                TEXT("Layer_Camera"),
                &cameraDesc),
            E_FAIL);
    }

    // 프리뷰 플레이어
    {
        Character::FCharacterDesc previewDesc{};
        previewDesc.name = TEXT("Preview Player");
        previewDesc.position = Vec3(6.5f, 0.f, 0.f);
        previewDesc.scale = Vec3(1.f, 1.f, 1.f);

        auto previewObj = GAME->Clone_And_Add_GameObject(
            0,
            Protocol::OBJECT_TYPE_PLAYER,
            ETOI(ELevelType::CharacterSetup),
            TEXT("Layer_Preview"),
            &previewDesc);

        CHECK_NULL(previewObj, E_FAIL);

        _previewPlayer = dynamic_pointer_cast<Player>(previewObj);
        CHECK_NULL(_previewPlayer, E_FAIL);

        // 플레이어 위치 전달
        GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(_previewPlayer->Get_Transform());
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
    const Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    const float startX = viewport.x * 0.78f;
    const float startY = viewport.y * 0.34f;
    const float width = 280.f;
    const float height = 56.f;
    const float spacing = 14.f;

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
        desc.selectedTextureIndex = ETOI(ECharacterSetupTexture::SelectedButton);
        desc.zOrder = 0.60f + i * 0.001f;
        desc.labelText = options[i].displayName;
        desc.labelSize = Vec2(width - 20.f, height);
        desc.fontSize = 20.f;

        auto button = static_pointer_cast<UI_TabButton>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TAB, EUILayer::HUD, &desc));
        CHECK_NULL(button);

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
}

void Level_CharacterSetup::Refresh_OptionSelection()
{
    for (int32 i = 0; i < static_cast<int32>(_optionButtons.size()); ++i)
    {
        if (_optionButtons[i])
            _optionButtons[i]->Set_Selected(i == _selectedOptionIndex);
    }
}

void Level_CharacterSetup::Refresh_SelectDescText()
{
    if (!_selectDescBg)
        return;

    _selectDescBg->Set_LabelText(Get_SelectDescText(Get_SelectSlot()));
}

void Level_CharacterSetup::Handle_TabInput()
{
    const int32 tabCount = static_cast<int32>(TAB_COUNT);

    if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
    {
        _selectedTabIndex = (_selectedTabIndex - 1 + tabCount) % tabCount;
        Apply_TabSelection();

        return;
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        _selectedTabIndex = (_selectedTabIndex + 1) % tabCount;
        Apply_TabSelection();

        return;
    }
}

void Level_CharacterSetup::Handle_OptionInput()
{
    const auto& options = Get_SelectedOptions();

    if (options.empty())
        return;

    const int32 optionCount = static_cast<int32>(options.size());

    if (INPUT->KeyDown(KEY_TYPE::LEFT) || INPUT->KeyDown(KEY_TYPE::A))
    {
        _selectedOptionIndex = (_selectedOptionIndex - 1 + optionCount) % optionCount;
        Refresh_OptionSelection();
        Apply_SelectedOption();
        return;
    }

    if (INPUT->KeyDown(KEY_TYPE::RIGHT) || INPUT->KeyDown(KEY_TYPE::D))
    {
        _selectedOptionIndex = (_selectedOptionIndex + 1) % optionCount;
        Refresh_OptionSelection();
        Apply_SelectedOption();
        return;
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
}

void Level_CharacterSetup::Apply_TabSelection()
{
    Refresh_TabSelection();
    Build_OptionButtons();
    Refresh_OptionSelection();
    Refresh_SelectDescText();
    Apply_SelectedOption();
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

