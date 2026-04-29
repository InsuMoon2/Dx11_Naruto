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
#include "AnimationStateComponent.h"
#include "Model.h"

static const FCameraPreset s_CameraPresets[] =
{
    { Vec3(6.6f, 1.7f, 0.78f),   Vec3(9.6f, 169.f, 0.f) },
    { Vec3(6.6f, 1.7f, 0.78f),   Vec3(9.6f, 169.f, 0.f) },
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) },
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) },
    { Vec3(6.6f, 1.1f, 1.48f),  Vec3(9.6f, 169.f, 0.f) },
};

Level_CharacterSetup::Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level{ device, context }
{
}

HRESULT Level_CharacterSetup::Initialize()
{
    Build_PartCatalog();
    Sync_EquippedIndicesFromCustomizer();

    CHECK_FAILED(Ready_Layer_UI(), E_FAIL);
    CHECK_FAILED(Ready_PreviewScene(), E_FAIL);

    CHECK_FAILED(Ready_NameInputUI(), E_FAIL);

    _selectedTabIndex = 0;
    _selectedOptionIndex = _equippedIndices[_selectedTabIndex];

    Refresh_TabSelection();
    Refresh_SelectDescText();

    Refresh_UI_Visibility();

    Apply_CustomizerToPreview();

    return S_OK;
}

void Level_CharacterSetup::Update(float timeDelta)
{
    Level::Update(timeDelta);

    

    Update_CameraLerp(timeDelta);

    Handle_RotationInput(timeDelta);

    if (_setupState == ESetupState::Category)
    {
        Handle_TabInput();
    }
    else if (_setupState == ESetupState::Item)
    {
        Handle_OptionInput();
    }
    else if (_setupState == ESetupState::NameInput)
    {
        Handle_NameInput();
        return;
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
    wintitleDesc.textDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
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

    {
        const array<wstring, TAB_COUNT> menuLabels =
        {
            L"머리",
            L"얼굴장식",
            L"한벌옷",
            L"상의",
            L"하의"
        };

        const float spacing = 15.f;
        const float tabWidth = 718.f * 0.75f;
        const float tabHeight = 64.f * 0.65f;

        for (int32 i = 0; i < static_cast<int32>(TAB_COUNT); ++i)
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
    selectDesc.textDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
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
    GAME->Clear_Lights();

    {
        FLightDesc lightDesc{};
        lightDesc.type = ELightType::Directional;
        lightDesc.direction = Vec4(0.15f, -0.55f, -0.82f, 0.f);
        lightDesc.diffuse = Vec4(1.f, 0.96f, 0.9f, 1.f);
        lightDesc.ambient = Vec4(0.35f, 0.35f, 0.38f, 1.f);
        lightDesc.specular = Vec4(0.65f, 0.65f, 0.65f, 1.f);
        lightDesc.castShadow = false;

        CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);
    }

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

        if (auto animState = _previewPlayer->Get_Component<AnimationStateComponent>())
        {
            animState->Play_State("Setup_Idle");

            if (auto model = _previewPlayer->Get_Component<Model>())
                model->Play_Animation(0.f, false);
        }
    }

    return S_OK;
}

void Level_CharacterSetup::Build_PartCatalog()
{
    _catalog[ETOI(ContainerObject::EPartSlot::Headegear)] =
    {
        { L"머리 1", L"Model_ManHat_03" },
        { L"머리 2", L"Model_SnowHead" },
        { L"머리 3", L"Model_PajamaHat" },
        { L"머리 4", L"Model_Umbrella" },
        { L"머리 5", L"Model_FrogHead" },
        { L"머리 6", L"Model_MadaraHead" },
    };

    _catalog[ETOI(ContainerObject::EPartSlot::Face)] =
    {
        { L"얼굴 1", L"Model_Face_Face1" },
        { L"얼굴 2", L"Model_Face_Mask1" },
        { L"얼굴 3", L"Model_Face_Mask2" },
    };

    _catalog[ETOI(ContainerObject::EPartSlot::Onepiece)] =
    {
        { L"한벌옷 1", L"Model_OnePiece_Armor2" },
        { L"한벌옷 2", L"Model_OnePiece_Armor3" },
        { L"한벌옷 3", L"Model_OnePiece_Minato" },
        { L"한벌옷 4", L"Model_OnePiece_Akachiki" },
        { L"한벌옷 5", L"Model_OnePiece_Frog" },
        { L"한벌옷 6", L"Model_OnePiece_Jaket" },
    };

    _catalog[ETOI(ContainerObject::EPartSlot::BodyUpper)] =
    {
        { L"상의 1", L"Model_Upper_Jiraiya" },
        { L"상의 2", L"Model_Upper_LeatherJaket" },
        { L"상의 3", L"Model_Upper_Logo" },
        { L"상의 4", L"Model_Upper_Sai" },
        { L"상의 5", L"Model_Upper_Saske" },
    };

    _catalog[ETOI(ContainerObject::EPartSlot::BodyLower)] =
    {
        { L"하의 1", L"Model_Lower_LeatherPants" },
        { L"하의 2", L"Model_Lower_Lower_PantsCut" },
        { L"하의 3", L"Model_Lower_Lower_StonePants" },
        { L"하의 4", L"Model_Lower_Lower_Trainer" },
    };
}

void Level_CharacterSetup::Sync_EquippedIndicesFromCustomizer()
{
    const auto& customDesc = GET_SINGLE(Customizer_Manager)->Get_CustomizerDesc();

    const wstring& onepieceTag = customDesc.Get_Part(ContainerObject::EPartSlot::Onepiece);
    const wstring& upperTag = customDesc.Get_Part(ContainerObject::EPartSlot::BodyUpper);
    const wstring& lowerTag = customDesc.Get_Part(ContainerObject::EPartSlot::BodyLower);

    const bool hasOnepiece = !onepieceTag.empty() && onepieceTag != TEXT("None");
    const bool hasUpper = !upperTag.empty() && upperTag != TEXT("None");
    const bool hasLower = !lowerTag.empty() && lowerTag != TEXT("None");

    _usesOnepieceOutfit = hasOnepiece || (!hasUpper && !hasLower);

    for (uint32 i = 0; i < TAB_COUNT; ++i)
    {
        const ContainerObject::EPartSlot slot = _tabSlots[i];
        const wstring& assetTag = customDesc.Get_Part(slot);

        _equippedIndices[i] = Find_CatalogIndex(slot, assetTag);
    }
}

void Level_CharacterSetup::Apply_CustomizerToPreview()
{
    if (!_previewPlayer)
        return;

    const auto& customDesc = GET_SINGLE(Customizer_Manager)->Get_CustomizerDesc();

    for (uint32 i = 0; i < TAB_COUNT; ++i)
    {
        const ContainerObject::EPartSlot slot = _tabSlots[i];

        if (_usesOnepieceOutfit &&
            (slot == ContainerObject::EPartSlot::BodyUpper || slot == ContainerObject::EPartSlot::BodyLower))
        {
            CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(slot, TEXT("None")));
            continue;
        }

        if (!_usesOnepieceOutfit && slot == ContainerObject::EPartSlot::Onepiece)
        {
            CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(slot, TEXT("None")));
            continue;
        }

        const wstring& assetTag = customDesc.Get_Part(slot);
        if (!assetTag.empty() && assetTag != TEXT("None"))
        {
            CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(slot, assetTag));
        }
    }
}

int32 Level_CharacterSetup::Find_CatalogIndex(ContainerObject::EPartSlot slot, const wstring& modelAssetTag) const
{
    const auto& options = _catalog[ETOI(slot)];

    for (int32 i = 0; i < static_cast<int32>(options.size()); ++i)
    {
        if (options[i].modelAssetTag == modelAssetTag)
            return i;
    }

    return 0;
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
    const int32 tabCount = static_cast<int32>(TAB_COUNT) + 1;

    if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
    {
        GAME->Play_Sound(L"UI_Select.wav", ESoundChannel::UI, 0.4f);

        _selectedTabIndex = (_selectedTabIndex - 1 + tabCount) % tabCount;

        Refresh_TabSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        GAME->Play_Sound(L"UI_Select.wav", ESoundChannel::UI, 0.4f);

        _selectedTabIndex = (_selectedTabIndex + 1) % tabCount;
        Refresh_TabSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        GAME->Play_Sound(L"UI_OK.wav", ESoundChannel::UI, 0.45f);

        if (_selectedTabIndex == static_cast<int32>(TAB_COUNT))
        {
            Enter_NameInput();

            return;
        }

        _setupState = ESetupState::Item;

        Build_OptionButtons();
        Refresh_UI_Visibility();
        Apply_CameraPreset(_selectedTabIndex);

        const auto& options = Get_SelectedOptions();

        if (!options.empty())
        {
            _selectedOptionIndex = _equippedIndices[_selectedTabIndex];
        }

        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }
}

void Level_CharacterSetup::Handle_OptionInput()
{
    if (INPUT->KeyDown(KEY_TYPE::ESCAPE) || INPUT->KeyDown(KEY_TYPE::BACK))
    {
        GAME->Play_Sound(L"UI_Cancel.wav", ESoundChannel::UI, 0.45f);

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
        GAME->Play_Sound(L"UI_Select.wav", ESoundChannel::UI, 0.4f);

        _selectedOptionIndex = (_selectedOptionIndex - 1 + totalCount) % totalCount;
        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
    {
        GAME->Play_Sound(L"UI_Select.wav", ESoundChannel::UI, 0.4f);

        _selectedOptionIndex = (_selectedOptionIndex + 1 + totalCount) % totalCount;
        Refresh_OptionSelection();
        Refresh_SelectDescText();
    }

    if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
    {
        GAME->Play_Sound(L"UI_OK.wav", ESoundChannel::UI, 0.45f);

        if (_selectedOptionIndex == optionCount)
        {
			Enter_NameInput();
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
    const ContainerObject::EPartSlot selectedSlot = Get_SelectSlot();
    CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(selectedSlot, option.modelAssetTag));

    if (selectedSlot == ContainerObject::EPartSlot::Onepiece)
    {
        _usesOnepieceOutfit = true;

        CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(ContainerObject::EPartSlot::BodyUpper, TEXT("None")));
        CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(ContainerObject::EPartSlot::BodyLower, TEXT("None")));
    }
    else if (selectedSlot == ContainerObject::EPartSlot::BodyUpper ||
             selectedSlot == ContainerObject::EPartSlot::BodyLower)
    {
        _usesOnepieceOutfit = false;

        CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(ContainerObject::EPartSlot::Onepiece, TEXT("None")));
    }

    if (selectedSlot == ContainerObject::EPartSlot::BodyUpper)
    {
        const ContainerObject::EPartSlot lowerSlot = ContainerObject::EPartSlot::BodyLower;
        const auto& lowerOptions = _catalog[ETOI(lowerSlot)];

        if (!lowerOptions.empty() && _previewPlayer->Get_PartObject(lowerSlot) == nullptr)
        {
            const int32 defaultLowerIndex = 0;
            CHECK_FAILED(_previewPlayer->Apply_CustomizingPart(
                lowerSlot,
                lowerOptions[defaultLowerIndex].modelAssetTag));

            for (uint32 i = 0; i < TAB_COUNT; ++i)
            {
                if (_tabSlots[i] == lowerSlot)
                {
                    _equippedIndices[i] = defaultLowerIndex;
                    break;
                }
            }
        }
    }

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

    for (int i = 0; i < TAB_COUNT; ++i)
    {
        ContainerObject::EPartSlot slot = _tabSlots[i];

        if (_usesOnepieceOutfit &&
            (slot == ContainerObject::EPartSlot::BodyUpper || slot == ContainerObject::EPartSlot::BodyLower))
        {
            custom->Set_Part(slot, TEXT("None"));
            continue;
        }

        if (!_usesOnepieceOutfit && slot == ContainerObject::EPartSlot::Onepiece)
        {
            custom->Set_Part(slot, TEXT("None"));
            continue;
        }

        if (!_catalog[ETOI(slot)].empty())
        {
            const wstring& assetTag = _catalog[ETOI(slot)][_equippedIndices[i]].modelAssetTag;
            custom->Set_Part(slot, assetTag);
        }
    }

    const EGameplaySpawnMode spawnMode =
        GAME->Is_EditorRuntime() ? EGameplaySpawnMode::LocalOnly
                                 : EGameplaySpawnMode::Server;

    if (spawnMode == EGameplaySpawnMode::Server)
    {
        if (!NetworkManager::GetInstance()->IsNetworkEnabled())
        {
            if (!NetworkManager::GetInstance()->Initialize())
            {
                LOG_WARN("[Client] Network service did not start before lobby transition.");
            }
        }
    }

    custom->Set_PlayerName(_pendingPlayerName);

    GAME->Change_Level(
        ETOI(ELevelType::Loading),
        Level_Loading::Create(_device, _context, ELevelType::Lobby, true, spawnMode));
}

void Level_CharacterSetup::On_CharInput(wchar_t ch)
{
    if (_setupState != ESetupState::NameInput)
        return;

    if (ch >= 0x20 && _pendingPlayerName.size() < 12)
    {
        _pendingPlayerName += ch;
        Refresh_NameInputText();
    }
}

HRESULT Level_CharacterSetup::Ready_NameInputUI()
{
    const Vec2 viewport = { GAME->Get_UIReferenceWidth(), GAME->Get_UIReferenceHeight() }; 

    Background::FBackgroundDesc bgDesc{};
    bgDesc.name = TEXT("NameInput_Background"); 
    bgDesc.posX = viewport.x * 0.5f;
    bgDesc.posY = viewport.y * 0.5f;
    bgDesc.sizeX = 600.f;
    bgDesc.sizeY = 200.f;
    bgDesc.levelIndex = ETOI(ELevelType::CharacterSetup); 
    bgDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT; 
    bgDesc.textureIndex = ETOI(ECharacterSetupTexture::PlayerTextBG);
    bgDesc.zOrder = 0.6f; 

    _nameInputBg = static_pointer_cast<Background>(
        GAME->Add_UI(Protocol::OBJECT_TYPE_BACKGROUND, EUILayer::Overlay, &bgDesc)); 

    _nameInputBg->Set_Visibility(false); 

    UI_Text::FUITextDesc textDesc{}; 
    textDesc.name = L"NameInput_Text"; 
    textDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    textDesc.zOrder = bgDesc.zOrder + 0.01f; 
    textDesc.style.fontSize = 28.f; 
    textDesc.style.color = Color(1.f, 0.9f, 0.2f, 1.f); 
    textDesc.style.hAlign = ETextHAlign::Center;

    _nameInputText = static_pointer_cast<UI_Text>(
        GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &textDesc));

    _nameInputText->Get_Transform()->Set_Parent(_nameInputBg->Get_Transform());
    GAME->Register_UI(EUILayer::Overlay, _nameInputText); 
    _nameInputText->Set_Visibility(false);

    return S_OK;
}

void Level_CharacterSetup::Enter_NameInput()
{
    _setupState = ESetupState::NameInput;
    _pendingPlayerName.clear();

    if (_nameInputBg)
        _nameInputBg->Set_Visibility(true);

    if (_nameInputText)
        _nameInputText->Set_Visibility(true);

    Refresh_NameInputText();
}

void Level_CharacterSetup::Handle_NameInput()
{
    if (INPUT->KeyDown(KEY_TYPE::ENTER)) 
    {
        if (!_pendingPlayerName.empty())
        {
            GAME->Play_Sound(L"UI_OK.wav", ESoundChannel::UI, 0.45f);
            Finish_CharacterSetup();
        }

        return;
    }

    if (INPUT->KeyDown(KEY_TYPE::BACK) || INPUT->KeyPress(KEY_TYPE::BACK))
    {
        if (!_pendingPlayerName.empty())
        {
            GAME->Play_Sound(L"UI_Cancel.wav", ESoundChannel::UI, 0.35f);

            _pendingPlayerName.pop_back(); 
            Refresh_NameInputText();
        }
        return;
    }

    if (INPUT->KeyDown(KEY_TYPE::ESCAPE)) 
    {
        GAME->Play_Sound(L"UI_Cancel.wav", ESoundChannel::UI, 0.45f);

        _setupState = ESetupState::Category; 
    }

}

void Level_CharacterSetup::Refresh_NameInputText()
{
    wstring display = _pendingPlayerName + L"_";

    if (_nameInputText)
    {
        _nameInputText->Set_Text(display);
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

