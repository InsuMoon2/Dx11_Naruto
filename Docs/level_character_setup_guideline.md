# Level_CharacterSetup 가이드라인

> 현재 `Level_CharacterSetup` 진행 상태를 기준으로,  
> `하단 중앙 SelectDesc 안내 문구 -> 파츠 리스트 표시 -> 실제 파츠 교체`까지 이어지는 작업 흐름을 정리한 문서.

---

## 1. 현재 상태 정리

### 이미 있는 것

- `Client/Public/Level_CharacterSetup.h`
- `Client/Private/Level_CharacterSetup.cpp`
- `Client/Public/UI_TabButton.h`
- `Client/Private/UI_TabButton.cpp`
- `Engine/Public/UI_Text.h`
- `Client/Public/Player.h`
- `Client/Private/Player.cpp`
- `Engine/Public/ContainerObject.h`
- `Engine/Private/ContainerObject.cpp`
- `Engine/Public/PartObject.h`
- `Engine/Private/PartObject.cpp`
- `Client/Public/Player_BodyUpper.h`
- `Client/Private/Player_BodyUpper.cpp`

### 현재 확인된 상태

1. `Level_CharacterSetup`에는 장비창 배경 / 윈도우 / 타이틀 / 탭 버튼 뼈대가 이미 있음.
2. `ECharacterSetupTexture`에는 `SelectDesc`, `TitleBG`, `Title_Symbol`, `SelectButton`, `TabButton` 슬롯이 이미 정의돼 있음.
3. `Player`는 이미 `ContainerObject` 기반이라 파츠 갈아끼우는 방향 자체는 맞음.
4. `PartObject`는 `masterPoseModel`을 받을 수 있어서, 파츠 쪽 애니메이션 동기화 구조는 이미 있음.

### 지금 바로 보이는 주의점

1. `Client/Private/Level_CharacterSetup.cpp`에서 메뉴 라벨은 6개인데 생성 루프가 `for (int32 i = 0; i < 5; ++i)`라서 실제로는 5개만 생성됨.
2. `UI_TabButton`은 `Render()`에서 `ECharacterSetupTexture::TabButton`을 직접 바인딩하고 있어서, `tabDesc.textureIndex` 값은 현재 사용되지 않음.
3. `UI_TabButton::Set_Selected()`가 비어 있어서 선택 강조 로직이 아직 없음.
4. `ContainerObject::EPartSlot`에는 `Hair` 슬롯이 없고 `Headegear`만 있음.  
   지금 사용자가 말한 "머리"를 진짜 헤어로 볼지, 모자/머리장식으로 볼지 먼저 정리해야 함.
5. 현재 실제 등록된 커스텀 파츠 모델은 아래 4개뿐임.
   - `Model_Body_Upper_Armor1`
   - `Model_Body_Upper_Coat15`
   - `Model_Face_Face1`
   - `Model_Headgear_Man_Cap1`
6. 즉 "머리 리스트를 쫙 보여주고 교체"를 하려면, UI 작업과 별개로 실제 헤어 모델 리소스 등록도 같이 필요함.
7. `Engine/Public/Input_Manager.h`에서 `KEY_TYPE::ENTER = VK_END`로 되어 있음.  
   나중에 키보드 확정 입력까지 넣을 거면 여기 영향도 체크해야 함.

---

## 2. 추천 구현 순서

### 1차 목표

- 좌측 탭을 선택하면
- 하단 중앙에 `"머리를 선택중입니다."`, `"얼굴장식을 선택중입니다."` 같은 문구가 뜨고
- 우측 또는 중앙 우측에 해당 카테고리의 파츠 리스트 버튼이 뜨고
- 리스트 버튼을 누르면 프리뷰 캐릭터의 파츠가 즉시 교체되는 것

### 추천 순서

1. `Level_CharacterSetup`에 "현재 어떤 카테고리를 선택 중인지" 상태를 추가한다.
2. `SelectDesc`용 `UI_Text`를 만든다.
3. 파츠 리스트용 버튼 UI를 만든다.
   - 처음엔 `UI_TabButton` 재사용 가능
   - 나중에 스타일이 다르면 `UI_SelectButton`으로 분리
4. 카테고리별 파츠 목록 데이터를 하드코딩으로 먼저 붙인다.
5. 탭 선택 시 리스트를 다시 구성한다.
6. 리스트 선택 시 `Player` 또는 프리뷰 전용 캐릭터에 실제 파츠 변경 함수를 호출한다.
7. 구조가 안정되면 하드코딩 목록을 JSON/테이블로 분리한다.

---

## 3. 추천 구조

### 카테고리 enum

`Level_CharacterSetup` 안에서 아래처럼 따로 두는 게 가장 관리가 편함.

```cpp
enum class ECustomizeCategory : uint8
{
    Hair,
    FaceDeco,
    Onepiece,
    Top,
    Bottom,
    Accessory,

    END
};
```

### 파츠 옵션 데이터

```cpp
struct FCustomizeOption
{
    wstring displayName;                    // 버튼에 보여줄 이름
    wstring descriptionText;               // 하단 설명 보강용
    ContainerObject::EPartSlot slot;       // 실제 장착 슬롯
    wstring modelAssetTag;                 // 예: Model_Hair_Short01
};

struct FCustomizeCategoryData
{
    wstring categoryLabel;                 // 예: 머리
    wstring selectDescText;                // 예: 머리를 선택중입니다.
    vector<FCustomizeOption> options;
};
```

### 추천 포인트

- 처음에는 `Level_CharacterSetup.cpp`에 `static const array<FCustomizeCategoryData, ...>` 형태로 하드코딩하는 걸 추천.
- 이유는 지금은 UI 흐름을 먼저 잡는 게 중요하고, 아직 실제 헤어 리소스도 충분히 안 들어와 있기 때문.
- 나중에 데이터가 늘어나면 `Client/Bin/Resources/Data/json/CharacterPartCatalog.json` 같은 별도 파일로 분리.
- enum 문자열 저장/복원이나 디버그 로그가 필요하면 `magic_enum` 사용.

---

## 4. 슬롯 설계 먼저 정리하기

현재 `ContainerObject::EPartSlot`은 아래 상태다.

```cpp
enum class EPartSlot : uint8
{
    Headegear,
    Accessory,
    BodyUpper,
    BodyLower,
    Face,
    Weapon,

    END
};
```

이 상태로는 "머리"를 `Headegear`에 억지로 넣게 되는데, 그렇게 가면 나중에 `헤어`와 `모자`를 동시에 못 끼게 된다.

### 권장안

```cpp
enum class EPartSlot : uint8
{
    Hair,
    Headgear,
    FaceDeco,
    Accessory,
    BodyUpper,
    BodyLower,
    Onepiece,
    Face,
    Weapon,

    END
};
```

### 최소 수정안

지금 빠르게 붙일 거면 아래처럼 타협할 수는 있다.

- `머리` -> `Headegear` 슬롯 재사용
- `얼굴장식` -> `Face` 슬롯 재사용
- `악세사리` -> `Accessory`
- `상의` -> `BodyUpper`
- `하의` -> `BodyLower`

하지만 이건 어디까지나 임시안이다.  
실제 커스터마이징 시스템으로 갈 생각이면 슬롯은 초반에 바로잡는 편이 낫다.

---

## 5. UI 배치 추천

### 추천 레이아웃

- 좌측: 카테고리 탭 6개
- 중앙: 프리뷰 캐릭터
- 우측: 선택 가능한 파츠 리스트
- 하단 중앙: 현재 선택 카테고리 설명 텍스트

### 하단 중앙 설명 텍스트

이건 굳이 별도 배경 없이 `UI_Text` 하나만 먼저 붙여도 충분하다.

예시 문구:

- `"머리를 선택중입니다."`
- `"얼굴장식을 선택중입니다."`
- `"한벌옷을 선택중입니다."`
- `"상의를 선택중입니다."`
- `"하의를 선택중입니다."`
- `"악세사리를 선택중입니다."`

필요하면 여기에 2줄로 확장:

- 1줄: `"머리를 선택중입니다."`
- 2줄: `"원하는 스타일을 눌러 바로 프리뷰를 확인하세요."`

### 우측 파츠 리스트

처음엔 `UI_TabButton`을 그대로 재활용해도 된다.

이유:

- 이미 텍스트를 붙이는 구조가 있음
- 선택 상태를 나중에 확장 가능
- 새 UI 클래스를 바로 늘리지 않아도 흐름 확인이 가능

단, 아래 둘은 추가해줘야 한다.

1. `HitTest()` 또는 `ContainsMouse()` 함수
2. `Set_Selected()` 시 색/알파/텍스트 틴트 변경

---

## 6. 프리뷰 캐릭터 처리 방향

### 권장

`Level_CharacterSetup` 전용 프리뷰 플레이어를 하나 스폰해서 그 객체만 갈아끼운다.

### 이유

- 실제 게임 진입용 플레이어와 분리 가능
- 서버 연결 여부와 상관없이 테스트 가능
- 커스터마이징 중 애니메이션, 카메라, 조명 세팅을 독립적으로 관리 가능

### 추천 형태

- 레벨 진입 시 프리뷰용 `Player` 1개 생성
- 카메라는 `Camera_Target` 또는 전용 프리뷰 카메라 사용
- 프리뷰 플레이어는 제자리 Idle 애니메이션만 재생

### 당장 최소로 가는 방법

일단 `Level_CharacterSetup` 초기화에서 캐릭터를 하나 생성하고,  
`Apply_SelectedPart()`에서 그 캐릭터의 파츠만 교체한다.

---

## 7. 파츠 교체 로직 권장안

현재 구조상 가장 중요한 건 `ContainerObject::Change_PartObject()`를 실제로 완성하는 것이다.

### 핵심 흐름

1. 기존 슬롯에 장착된 파츠를 찾는다.
2. 기존 파츠가 있으면 `Set_Destroy(true)` 처리한다.
3. 새 desc를 만들어서 새 파츠 오브젝트를 다시 clone 한다.
4. `_partObjects[slot]`를 새 파츠로 교체한다.

### 권장 변경 방향

`Player_BodyUpper` 한 클래스로 모든 파츠를 처리하려 하지 말고,  
`Player_CostumePart` 같은 공용 클래스로 일반화하는 걸 추천한다.

이유:

- 상의 / 하의 / 얼굴 / 머리 / 악세사리 구조가 거의 똑같음
- 모델 태그만 다르고 렌더 흐름은 동일함
- 파일 복붙이 줄어듦

---

## 8. 파일별 작업 가이드

## [추가] `Client/Public/Level_CharacterSetup.h`

```cpp
#pragma once

#include "Level.h"

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

    END
};

enum class ECustomizeCategory : uint8
{
    Hair,
    FaceDeco,
    Onepiece,
    Top,
    Bottom,
    Accessory,

    END
};

struct FCustomizeOption
{
    wstring displayName;
    wstring descriptionText;
    ContainerObject::EPartSlot slot = ContainerObject::EPartSlot::Accessory;
    wstring modelAssetTag;
};

struct FCustomizeCategoryData
{
    wstring categoryLabel;
    wstring selectDescText;
    vector<FCustomizeOption> options;
};

class Level_CharacterSetup final : public Level
{
public:
    explicit Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_CharacterSetup();

public:
    HRESULT Initialize() override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Layer_UI();
    HRESULT Ready_PreviewCharacter();

private:
    void    Build_CustomizeCatalog();
    void    Apply_CategorySelection();
    void    Refresh_OptionButtons();
    void    Update_SelectDesc();
    void    Handle_TabInput();
    void    Handle_OptionInput();
    void    Apply_SelectedPart(const FCustomizeOption& option);

    const FCustomizeCategoryData& Get_CurrentCategoryData() const;

private:
    array<Shared<UI_TabButton>, ETOI(ECustomizeCategory::END)> _tabButtons {};
    vector<Shared<UI_TabButton>> _optionButtons;

    Shared<UI_Text> _selectDescText;
    Shared<Player>  _previewPlayer;

    array<FCustomizeCategoryData, ETOI(ECustomizeCategory::END)> _catalog {};

    ECustomizeCategory _selectedCategory = ECustomizeCategory::Hair;
    int32 _selectedOptionIndex = 0;

public:
    static shared_ptr<Level_CharacterSetup> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
```

### 핵심 포인트

- `SelectDesc`는 `Shared<UI_Text>`로 따로 잡는 게 가장 단순함.
- 탭 버튼과 옵션 버튼을 같은 `UI_TabButton`으로 시작하면 빠르게 확인 가능.
- `_catalog`을 먼저 코드에 들고 있으면 JSON 파서 작업 없이 화면 흐름부터 완성할 수 있음.

---

## [추가] `Client/Private/Level_CharacterSetup.cpp`

아래는 구조를 잡기 위한 기준 코드다.  
실제 좌표는 작업하면서 조금씩 조정하면 된다.

```cpp
#include "pch.h"
#include "Level_CharacterSetup.h"

#include "Background.h"
#include "GameInstance.h"
#include "Loader.h"
#include "Player.h"
#include "UI_TabButton.h"
#include "UI_Text.h"

Level_CharacterSetup::Level_CharacterSetup(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Level(device, context)
{
}

Level_CharacterSetup::~Level_CharacterSetup()
{
}

HRESULT Level_CharacterSetup::Initialize()
{
    Build_CustomizeCatalog();

    CHECK_FAILED(Ready_Layer_UI(), E_FAIL);
    CHECK_FAILED(Ready_PreviewCharacter(), E_FAIL);

    Apply_CategorySelection();

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
    SetWindowText(g_hWnd, TEXT("현재 레벨 : Character Setup"));
#endif

    return S_OK;
}

void Level_CharacterSetup::Build_CustomizeCatalog()
{
    // 머리
    _catalog[ETOI(ECustomizeCategory::Hair)] =
    {
        .categoryLabel = L"머리",
        .selectDescText = L"머리를 선택중입니다.",
        .options =
        {
            // 주의:
            // 현재 프로젝트에는 실제 Hair 모델이 아직 없으므로,
            // 임시로 Headgear 슬롯 태그 예시를 넣어둔 상태.
            { L"기본 모자 01", L"기본형 머리/머리장식 프리뷰", ContainerObject::EPartSlot::Headegear, L"Model_Headgear_Man_Cap1" },
        }
    };

    // 얼굴장식
    _catalog[ETOI(ECustomizeCategory::FaceDeco)] =
    {
        .categoryLabel = L"얼굴장식",
        .selectDescText = L"얼굴장식을 선택중입니다.",
        .options =
        {
            { L"기본 얼굴 01", L"기본 얼굴형 프리뷰", ContainerObject::EPartSlot::Face, L"Model_Face_Face1" },
        }
    };

    // 한벌옷
    _catalog[ETOI(ECustomizeCategory::Onepiece)] =
    {
        .categoryLabel = L"한벌옷",
        .selectDescText = L"한벌옷을 선택중입니다.",
        .options = {}
    };

    // 상의
    _catalog[ETOI(ECustomizeCategory::Top)] =
    {
        .categoryLabel = L"상의",
        .selectDescText = L"상의을 선택중입니다.",
        .options =
        {
            { L"Armor 01", L"상체 방어구 프리뷰", ContainerObject::EPartSlot::BodyUpper, L"Model_Body_Upper_Armor1" },
            { L"Coat 15",  L"상체 코트 프리뷰",   ContainerObject::EPartSlot::BodyUpper, L"Model_Body_Upper_Coat15" },
        }
    };

    // 하의
    _catalog[ETOI(ECustomizeCategory::Bottom)] =
    {
        .categoryLabel = L"하의",
        .selectDescText = L"하의를 선택중입니다.",
        .options = {}
    };

    // 악세사리
    _catalog[ETOI(ECustomizeCategory::Accessory)] =
    {
        .categoryLabel = L"악세사리를 선택중입니다.",
        .selectDescText = L"악세사리를 선택중입니다.",
        .options = {}
    };
}

HRESULT Level_CharacterSetup::Ready_Layer_UI()
{
    const Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    // 배경 / 윈도우 / 윈도우 타이틀은 기존 코드 유지
    // 기존 코드의 좌표 구성은 그대로 두고, 아래 UI만 추가해도 충분함.

    // 탭 버튼 6개
    {
        const array<wstring, ETOI(ECustomizeCategory::END)> labels =
        {
            L"머리",
            L"얼굴장식",
            L"한벌옷",
            L"상의",
            L"하의",
            L"악세사리"
        };

        const float tabWidth = 718.f * 0.75f;
        const float tabHeight = 64.f * 0.8f;
        const float spacing = 15.f;
        const float baseX = viewport.x * 0.3f - 60.f;
        const float baseY = viewport.y * 0.5f - 140.f;

        for (int32 i = 0; i < ETOI(ECustomizeCategory::END); ++i)
        {
            UI_TabButton::FUITabDesc desc {};
            desc.name = format(L"CategoryTab {}", i);
            desc.posX = baseX;
            desc.posY = baseY + i * (tabHeight + spacing);
            desc.sizeX = tabWidth;
            desc.sizeY = tabHeight;
            desc.levelIndex = ETOI(ELevelType::CharacterSetup);
            desc.zOrder = 0.52f + i * 0.001f;
            desc.labelText = labels[i];
            desc.labelSize = Vec2(400.f, 60.f);
            desc.fontSize = 24.f;

            _tabButtons[i] = static_pointer_cast<UI_TabButton>(
                GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TAB, EUILayer::HUD, &desc));
            CHECK_NULL(_tabButtons[i], E_FAIL);
        }
    }

    // 하단 중앙 설명 텍스트
    {
        UI_Text::FUITextDesc desc {};
        desc.name = TEXT("CharacterSetup_SelectDescText");
        desc.posX = viewport.x * 0.5f;
        desc.posY = viewport.y - 80.f;
        desc.sizeX = 700.f;
        desc.sizeY = 40.f;
        desc.zOrder = 0.62f;
        desc.levelIndex = ETOI(ELevelType::CharacterSetup);
        desc.text = L"머리를 선택중입니다.";
        desc.style.fontFamily = L"Malgun Gothic";
        desc.style.fontSize = 24.f;
        desc.style.color = Color(1.f, 1.f, 1.f, 1.f);
        desc.style.hAlign = ETextHAlign::Center;
        desc.style.vAlign = ETextVAlign::Middle;
        desc.style.wordWrap = false;

        _selectDescText = static_pointer_cast<UI_Text>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TEXT, EUILayer::HUD, &desc));
        CHECK_NULL(_selectDescText, E_FAIL);
    }

    return S_OK;
}

HRESULT Level_CharacterSetup::Ready_PreviewCharacter()
{
    // 여기서는 실제 프로젝트 상황에 맞춰 프리뷰 캐릭터 생성.
    // 이미 프리팹이 있으면 Prefab Spawn Helper를 쓰고,
    // 아직 없으면 Player prototype을 직접 Add_GameObject로 생성하면 된다.
    return S_OK;
}

void Level_CharacterSetup::Apply_CategorySelection()
{
    for (int32 i = 0; i < ETOI(ECustomizeCategory::END); ++i)
    {
        if (_tabButtons[i])
            _tabButtons[i]->Set_Selected(i == ETOI(_selectedCategory));
    }

    _selectedOptionIndex = 0;

    Update_SelectDesc();
    Refresh_OptionButtons();
}

void Level_CharacterSetup::Update_SelectDesc()
{
    if (!_selectDescText)
        return;

    _selectDescText->Set_Text(Get_CurrentCategoryData().selectDescText);
}

void Level_CharacterSetup::Refresh_OptionButtons()
{
    // 기존 버튼은 파괴 처리
    for (auto& button : _optionButtons)
    {
        if (button)
            button->Set_Destroy(true);
    }
    _optionButtons.clear();

    const auto& categoryData = Get_CurrentCategoryData();
    const Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    const float startX = viewport.x * 0.72f;
    const float startY = viewport.y * 0.35f;
    const float width = 320.f;
    const float height = 56.f;
    const float spacing = 14.f;

    for (int32 i = 0; i < static_cast<int32>(categoryData.options.size()); ++i)
    {
        const auto& option = categoryData.options[i];

        UI_TabButton::FUITabDesc desc {};
        desc.name = format(L"OptionButton {}", i);
        desc.posX = startX;
        desc.posY = startY + i * (height + spacing);
        desc.sizeX = width;
        desc.sizeY = height;
        desc.levelIndex = ETOI(ELevelType::CharacterSetup);
        desc.zOrder = 0.60f + i * 0.001f;
        desc.labelText = option.displayName;
        desc.labelSize = Vec2(width - 20.f, height);
        desc.fontSize = 20.f;

        auto button = static_pointer_cast<UI_TabButton>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_TAB, EUILayer::HUD, &desc));
        CHECK_NULL(button);

        button->Set_Selected(i == _selectedOptionIndex);
        _optionButtons.push_back(button);
    }

    if (!categoryData.options.empty())
    {
        Apply_SelectedPart(categoryData.options[_selectedOptionIndex]);
    }
}

void Level_CharacterSetup::Handle_TabInput()
{
    // 마우스 클릭 기준으로 탭 판정
    if (!INPUT->KeyDown(KEY_TYPE::LBUTTON))
        return;

    const POINT mousePos = INPUT->GetMousePos();

    for (int32 i = 0; i < ETOI(ECustomizeCategory::END); ++i)
    {
        auto& button = _tabButtons[i];
        if (!button)
            continue;

        if (button->HitTest(mousePos))
        {
            _selectedCategory = static_cast<ECustomizeCategory>(i);
            Apply_CategorySelection();
            break;
        }
    }
}

void Level_CharacterSetup::Handle_OptionInput()
{
    if (!INPUT->KeyDown(KEY_TYPE::LBUTTON))
        return;

    const auto& categoryData = Get_CurrentCategoryData();
    const POINT mousePos = INPUT->GetMousePos();

    for (int32 i = 0; i < static_cast<int32>(_optionButtons.size()); ++i)
    {
        auto& button = _optionButtons[i];
        if (!button)
            continue;

        if (button->HitTest(mousePos))
        {
            _selectedOptionIndex = i;

            for (int32 buttonIndex = 0; buttonIndex < static_cast<int32>(_optionButtons.size()); ++buttonIndex)
            {
                if (_optionButtons[buttonIndex])
                    _optionButtons[buttonIndex]->Set_Selected(buttonIndex == _selectedOptionIndex);
            }

            if (i < static_cast<int32>(categoryData.options.size()))
            {
                Apply_SelectedPart(categoryData.options[i]);
            }
            break;
        }
    }
}

void Level_CharacterSetup::Apply_SelectedPart(const FCustomizeOption& option)
{
    if (!_previewPlayer)
        return;

    // 이 함수에서 실제 프리뷰 캐릭터 파츠를 교체한다.
    _previewPlayer->Apply_CustomizingPart(option.slot, option.modelAssetTag);
}

const FCustomizeCategoryData& Level_CharacterSetup::Get_CurrentCategoryData() const
{
    return _catalog[ETOI(_selectedCategory)];
}
```

### 여기서 중요한 점

1. 카테고리 전환 시
   - 탭 강조 변경
   - 설명 문구 변경
   - 우측 리스트 재생성
   - 첫 번째 항목 자동 프리뷰 적용
2. 이 순서로 가면 사용자가 "어떤 탭을 눌렀는지" 즉시 피드백을 받는다.

---

## [추가] `Client/Public/UI_TabButton.h`

버튼 클릭 판정과 선택 표현을 위해 아래 정도는 추가하는 걸 추천.

```cpp
public:
    bool    HitTest(const POINT& mousePos) const;
    RECT    Get_ScreenRect() const;
```

---

## [추가] `Client/Private/UI_TabButton.cpp`

```cpp
void UI_TabButton::Set_Selected(bool bSelected)
{
    _isSelected = bSelected;

    // 주석:
    // 선택된 버튼은 글자색을 밝게, 비선택은 어둡게 줘서
    // 현재 포커스를 바로 알 수 있게 한다.
    if (_labelUI)
    {
        _labelUI->Set_UITint(
            _isSelected
            ? Color(1.f, 1.f, 1.f, 1.f)
            : Color(0.15f, 0.15f, 0.15f, 1.f));
    }

    // 주석:
    // 이후 전용 셰이더 값을 넣을 계획이면 여기에
    // 하이라이트 알파 / 스케일 값을 같이 관리하면 된다.
}

RECT UI_TabButton::Get_ScreenRect() const
{
    float halfW = _sizeX * 0.5f;
    float halfH = _sizeY * 0.5f;

    RECT rc {};
    rc.left   = static_cast<LONG>(_posX - halfW);
    rc.top    = static_cast<LONG>(_posY - halfH);
    rc.right  = static_cast<LONG>(_posX + halfW);
    rc.bottom = static_cast<LONG>(_posY + halfH);

    return rc;
}

bool UI_TabButton::HitTest(const POINT& mousePos) const
{
    const RECT rc = Get_ScreenRect();
    return PtInRect(&rc, mousePos);
}
```

### 포인트

- 지금 UI 좌표는 디자인 해상도 기준으로 다루고 있으니, 먼저 이 방식으로 충분함.
- 나중에 UI viewport scaling까지 엄밀하게 맞추고 싶으면 `UI_Text::Build_ScreenRect()` 방식과 동일하게 비율 보정해주면 된다.

---

## [권장 변경] `Engine/Public/ContainerObject.h`

현재 `Change_PartObject(EPartSlot)`는 인자가 너무 부족하다.  
새 파츠를 뭘로 갈아끼울지 정보가 있어야 한다.

```cpp
public:
    HRESULT Change_PartObject(EPartSlot slot, uint32 objID, void* arg);
```

---

## [권장 변경] `Engine/Private/ContainerObject.cpp`

```cpp
HRESULT ContainerObject::Change_PartObject(EPartSlot slot, uint32 objID, void* arg)
{
    const int32 index = ETOI(slot);
    if (index >= ETOI(EPartSlot::END))
        return E_FAIL;

    if (_partObjects[index])
    {
        // 주석:
        // 기존 파츠는 바로 메모리 해제하지 않고 destroy 플래그만 세팅한다.
        // 현재 엔진 흐름에서 안전하게 프레임 종료 후 정리되게 맞추기 위함.
        _partObjects[index]->Set_Destroy(true);
        _partObjects[index] = nullptr;
    }

    Shared<PartObject> newPart = static_pointer_cast<PartObject>(
        GAME->Clone_GameObject(0, objID, arg));
    CHECK_NULL(newPart, E_FAIL);

    _partObjects[index] = newPart;
    return S_OK;
}
```

---

## [권장 변경] `Client/Public/Player.h`

프리뷰에서 바로 호출할 수 있는 전용 함수를 하나 두는 편이 좋다.

```cpp
public:
    HRESULT Apply_CustomizingPart(ContainerObject::EPartSlot slot, const wstring& modelAssetTag);
    Shared<Model> Get_BaseModel() const { return _model; }
```

---

## [권장 변경] `Client/Private/Player.cpp`

```cpp
HRESULT Player::Apply_CustomizingPart(ContainerObject::EPartSlot slot, const wstring& modelAssetTag)
{
    PartObject::FPartObjectDesc desc {};
    desc.parentMatrix = &_transformCom->Get_WorldMatrix();
    desc.modelAssetTag = modelAssetTag;
    desc.masterPoseModel = _model;

    // 주석:
    // 권장 방향은 Player_CostumePart 같은 공용 파츠 클래스를 하나 만들고
    // 모든 커스텀 파츠를 그 클래스로 통일하는 것이다.
    return Change_PartObject(slot, Protocol::OBJECT_TYPE_PART_OBJECT, &desc);
}
```

---

## [권장 신규] `Client/Public/Player_CostumePart.h`

현재 `Player_BodyUpper`는 사실상 "모든 스켈레탈 파츠" 공용 로직이다.  
이걸 일반화해서 재사용하는 쪽이 맞다.

```cpp
#pragma once

#include "PartObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class Player_CostumePart final : public PartObject
{
    GENERATED_BODY(Player_CostumePart)

public:
    explicit Player_CostumePart(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Player_CostumePart(const Player_CostumePart& rhs);
    virtual ~Player_CostumePart() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components(const wstring& modelAssetTag);
    HRESULT Bind_ShaderResources();
    HRESULT Bind_Lights();

private:
    Shared<Shader> _shader;
    Shared<Model>  _model;
    Shared<Model>  _masterPoseModel;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

### 구현은 사실상 `Player_BodyUpper.cpp`를 거의 그대로 가져오면 된다.

즉, 지금 구조에서는 `Player_BodyUpper`를 없애는 게 아니라

- 1단계: `Player_CostumePart` 신규 추가
- 2단계: `Player.cpp`에서 새 클래스 사용
- 3단계: 안정화 후 `Player_BodyUpper` 정리

순서로 가는 게 안전하다.

---

## 9. 실제 리소스 등록도 같이 필요함

현재 `DT_Model.json` 기준으로 커스텀 장비 모델은 아래 4개뿐이다.

```json
{
    "Id": "Model_Body_Upper_Armor1",
    "Path": "../../Client/Bin/Resources/Models/Custom/Body_Upper/Armor1.meshbin"
},
{
    "Id": "Model_Body_Upper_Coat15",
    "Path": "../../Client/Bin/Resources/Models/Custom/Body_Upper/Coat15.meshbin"
},
{
    "Id": "Model_Face_Face1",
    "Path": "../../Client/Bin/Resources/Models/Custom/Face/Face1.meshbin"
},
{
    "Id": "Model_Headgear_Man_Cap1",
    "Path": "../../Client/Bin/Resources/Models/Custom/Headgear/Man_Cap1.meshbin"
}
```

### 따라서 헤어 선택 UI를 진짜 헤어로 돌리려면

1. `Client/Bin/Resources/Models/Custom/Hair/` 아래에 hair meshbin 추가
2. `Client/Bin/Resources/Data/csv/ModelTable.csv` 등록
3. `Client/Bin/Resources/Data/json/DT_Model.json` 재생성 또는 동기화

예시:

```csv
Model,Model_Hair_Short01,../../Client/Bin/Resources/Models/Custom/Hair/Short01.meshbin,1,SkeletalMesh,Static,
Model,Model_Hair_Long02,../../Client/Bin/Resources/Models/Custom/Hair/Long02.meshbin,1,SkeletalMesh,Static,
```

---

## 10. 최소 동작 버전과 확장 버전

### 최소 동작 버전

- 탭 선택
- 하단 설명 문구 출력
- 우측 파츠 리스트 출력
- 클릭 시 프리뷰 파츠 교체
- 데이터는 `Level_CharacterSetup.cpp` 하드코딩

### 확장 버전

- JSON 기반 파츠 카탈로그 로드
- 카테고리별 아이콘 / 썸네일 표시
- 선택 사운드 / 하이라이트 애니메이션
- 장착 중 / 미장착 / 잠금 상태 표시
- 서버 저장 또는 플레이어 프로필 반영

---

## 11. 개인적으로 추천하는 구현 순서

### 가장 안 꼬이는 순서

1. `Level_CharacterSetup` 상태 enum 추가
2. `SelectDesc`용 `UI_Text` 추가
3. 탭 6개 정상 생성되도록 수정
4. `UI_TabButton::HitTest()` + `Set_Selected()` 구현
5. 우측 옵션 버튼 재생성 로직 구현
6. `Player::Apply_CustomizingPart()` 추가
7. `ContainerObject::Change_PartObject()` 구현
8. `Player_CostumePart` 공용화
9. 실제 Hair 리소스 등록

이 순서로 가면

- UI만 먼저 보이고
- 그 다음 프리뷰가 붙고
- 마지막에 데이터 확장으로 자연스럽게 넘어간다.

---

## 12. 바로 작업 들어갈 때 체크리스트

- `Level_CharacterSetup.cpp`의 탭 생성 루프를 6개로 수정했는가
- `UI_TabButton::Set_Selected()`가 실제로 색/선택 상태를 반영하는가
- `HitTest()`가 들어가서 마우스 클릭 판정이 가능한가
- 프리뷰 캐릭터가 `masterPoseModel` 기반 파츠를 정상 렌더하는가
- 기존 파츠를 교체할 때 이전 파츠가 화면에 중복으로 남지 않는가
- 실제 교체 가능한 모델 태그가 `DT_Model.json`에 등록돼 있는가
- 헤어를 진짜 Hair 슬롯으로 갈지, 임시 Headgear 슬롯으로 갈지 결정했는가

---

## 13. 결론

이번 작업의 핵심은 UI를 더 그리는 게 아니라 아래 3개를 동시에 묶는 것이다.

1. `현재 선택 카테고리 상태`
2. `카테고리별 파츠 목록 데이터`
3. `프리뷰 캐릭터에 실제 장착하는 함수`

이 3개만 먼저 안정화되면,

- `"머리를 선택중입니다"` 문구 출력
- `"머리 옵션 리스트 출력"`
- `"선택 즉시 모델 교체"`

까지는 아주 깔끔하게 이어진다.

그 다음부터는 데이터만 늘리면 된다.
