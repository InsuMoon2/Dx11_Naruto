# 플레이어 이름 입력 시스템 가이드라인

## 구조 개요

현재 흐름:
```
결정 선택 → Finish_CharacterSetup() → Change_Level(GamePlay)
```

목표 흐름:
```
결정 선택 → ESetupState::NameInput 진입 → 이름 입력 UI 표시
           → 확인(ENTER) → 이름 저장 → Finish_CharacterSetup() → Change_Level(GamePlay)
```

---

## 1. ESetupState 확장

`Level_CharacterSetup.h` 안의 `ESetupState` enum에 `NameInput` 추가.

```cpp
enum class ESetupState
{
    Category,
    Item,
    NameInput,  // [추가] 이름 입력 단계
};
```

---

## 2. 멤버 변수 추가

`Level_CharacterSetup.h`에 이름 입력용 변수 추가.

```cpp
// 이름 입력 관련
wstring         _pendingPlayerName = L"";       // 입력 버퍼 (wstring)
Shared<Background>  _nameInputBg    = nullptr;  // 이름 입력창 배경 UI
Shared<UI_Text>     _nameInputText  = nullptr;  // 입력 중 이름 표시 UI
```

> `UI_Text`는 `Engine/Public/UI_Text.h` 기반. 기존 `Background`의 `_textDesc`로도 가능.

---

## 3. UI 구성 — `Ready_Layer_UI()` 또는 별도 함수

이름 입력 창은 처음엔 숨겨두고, `NameInput` 상태 진입 시 보이게 한다.

```cpp
HRESULT Level_CharacterSetup::Ready_NameInputUI()
{
    const Vec2 viewport = { GAME->Get_UIReferenceWidth(), GAME->Get_UIReferenceHeight() };

    // 입력창 배경
    Background::FBackgroundDesc bgDesc{};
    bgDesc.name       = TEXT("NameInput_Background");
    bgDesc.posX       = viewport.x * 0.5f;
    bgDesc.posY       = viewport.y * 0.5f;
    bgDesc.sizeX      = 600.f;
    bgDesc.sizeY      = 200.f;
    bgDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    bgDesc.textureType  = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    bgDesc.textureIndex = ETOI(ECharacterSetupTexture::Window); // 적당한 배경 텍스처
    bgDesc.zOrder     = 0.6f;

    // 안내 텍스트
    bgDesc.textDesc.text            = L"이름을 입력하세요";
    bgDesc.textDesc.size            = Vec2(500.f, 50.f);
    bgDesc.textDesc.offset          = Vec2(0.f, -60.f);
    bgDesc.textDesc.zOrderOffset    = 0.01f;
    bgDesc.textDesc.style.fontFamily = L"Malgun Gothic";
    bgDesc.textDesc.style.fontSize  = 22.f;
    bgDesc.textDesc.style.color     = Color(1.f, 1.f, 1.f, 1.f);
    bgDesc.textDesc.style.hAlign    = ETextHAlign::Center;
    bgDesc.textDesc.style.vAlign    = ETextVAlign::Middle;

    _nameInputBg = static_pointer_cast<Background>(
        GAME->Add_UI(Protocol::OBJECT_TYPE_BACKGROUND, EUILayer::Overlay, &bgDesc));

    CHECK_NULL(_nameInputBg, E_FAIL);
    _nameInputBg->Set_Visibility(false); // 초기에는 숨김

    // 실시간 입력 표시 텍스트
    UI_Text::FUITextDesc textDesc{};
    textDesc.name       = L"NameInput_Text";
    textDesc.posX       = 0.f;
    textDesc.posY       = 20.f;
    textDesc.sizeX      = 480.f;
    textDesc.sizeY      = 60.f;
    textDesc.zOrder     = bgDesc.zOrder + 0.01f;
    textDesc.levelIndex = ETOI(ELevelType::CharacterSetup);
    textDesc.text       = L"";
    textDesc.style.fontFamily = L"Malgun Gothic";
    textDesc.style.fontSize   = 28.f;
    textDesc.style.color      = Color(1.f, 0.9f, 0.2f, 1.f);
    textDesc.style.hAlign     = ETextHAlign::Center;
    textDesc.style.vAlign     = ETextVAlign::Middle;

    _nameInputText = static_pointer_cast<UI_Text>(
        GAME->Clone_UI(Protocol::OBJECT_TYPE_UI_TEXT, &textDesc));

    CHECK_NULL(_nameInputText, E_FAIL);

    _nameInputText->Get_Transform()->Set_Parent(_nameInputBg->Get_Transform());
    GAME->Register_UI(EUILayer::Overlay, _nameInputText);
    _nameInputText->Set_Visibility(false); // 초기에는 숨김

    return S_OK;
}
```

`Ready_Layer_UI()` 안이나 `Initialize()` 안에서 `Ready_NameInputUI()` 호출.

---

## 4. NameInput 상태 진입

`Finish_CharacterSetup()` 대신 이름 입력 상태로 전환하도록  
`Handle_TabInput()` / `Handle_OptionInput()`의 결정 분기를 수정.

```cpp
// 기존
Finish_CharacterSetup();

// 변경
Enter_NameInput();
```

```cpp
void Level_CharacterSetup::Enter_NameInput()
{
    _setupState = ESetupState::NameInput;
    _pendingPlayerName.clear();

    // 탭/옵션 UI 숨기기
    for (auto& btn : _tabButtons)
        if (btn) btn->Set_Visibility(false);
    for (auto& btn : _optionButtons)
        if (btn) btn->Set_Visibility(false);
    if (_selectButton) _selectButton->Set_Visibility(false);

    // 이름 입력 UI 표시
    if (_nameInputBg)   _nameInputBg->Set_Visibility(true);
    if (_nameInputText) _nameInputText->Set_Visibility(true);

    // 입력 표시 초기화
    Refresh_NameInputText();
}
```

---

## 5. 키보드 입력 처리 — `Handle_NameInput()`

`Update()`에 `NameInput` 분기 추가 후, WM_CHAR 기반 입력 처리.

```cpp
// Update()
else if (_setupState == ESetupState::NameInput)
{
    Handle_NameInput();
}
```

```cpp
void Level_CharacterSetup::Handle_NameInput()
{
    // ENTER: 이름 확정
    if (INPUT->KeyDown(KEY_TYPE::ENTER))
    {
        if (_pendingPlayerName.empty())
            return; // 이름 없으면 무시

        Finish_CharacterSetup();
        return;
    }

    // ESC: 이름 입력 취소 → 카테고리로 복귀
    if (INPUT->KeyDown(KEY_TYPE::ESCAPE))
    {
        _setupState = ESetupState::Category;
        _pendingPlayerName.clear();

        if (_nameInputBg)   _nameInputBg->Set_Visibility(false);
        if (_nameInputText) _nameInputText->Set_Visibility(false);

        Refresh_UI_Visibility();
        Refresh_TabSelection();
        return;
    }

    // BACK: 한 글자 지우기
    if (INPUT->KeyDown(KEY_TYPE::BACK))
    {
        if (!_pendingPlayerName.empty())
        {
            _pendingPlayerName.pop_back();
            Refresh_NameInputText();
        }
        return;
    }
}

void Level_CharacterSetup::Refresh_NameInputText()
{
    // 커서 깜빡임 없이 현재 이름 + '_' 표시
    wstring display = _pendingPlayerName + L"_";
    if (_nameInputText)
        _nameInputText->Set_Text(display);
}
```

> [!IMPORTANT]
> 한글 등 유니코드 문자 입력은 `WM_CHAR`를 처리해야 합니다.
> `Input_Manager`는 현재 `GetKeyboardState()` 기반이라 **문자 입력을 직접 처리하지 않습니다.**
> 아래 WM_CHAR 방식 중 하나를 선택해야 합니다.

---

## 6. 문자 입력 처리 전략 (중요)

### 방법 A — WinProc WM_CHAR 후킹 (권장)

`MainApp` 또는 게임 메인 WinProc에서 `WM_CHAR` 메시지 수신 시  
`Level_CharacterSetup`에 전달하는 구조.

```cpp
// WinProc 내부
case WM_CHAR:
{
    wchar_t ch = static_cast<wchar_t>(wParam);

    // 현재 레벨이 CharacterSetup이면 전달
    auto level = dynamic_pointer_cast<Level_CharacterSetup>(GAME->Get_CurrentLevel());
    if (level)
        level->On_CharInput(ch);

    break;
}
```

```cpp
// Level_CharacterSetup에서
void Level_CharacterSetup::On_CharInput(wchar_t ch)
{
    if (_setupState != ESetupState::NameInput)
        return;

    // 제어문자 제외 (백스페이스, 엔터 등은 Handle_NameInput에서 처리)
    if (ch < 0x20)
        return;

    // 최대 글자 수 제한 (예: 12자)
    if (_pendingPlayerName.size() >= 12)
        return;

    _pendingPlayerName += ch;
    Refresh_NameInputText();
}
```

`Level_CharacterSetup.h`에 `On_CharInput(wchar_t ch)` public 선언 필요.

### 방법 B — IME 없이 영문/숫자만 (간단)

`Handle_NameInput()` 내에서 `GetKeyboardState()` + `ToUnicode()` 조합으로  
영문/숫자만 받는 방식. 한글 지원 불가.

---

## 7. 이름 저장 — `Customizer_Manager` 활용

`Finish_CharacterSetup()`에서 기존 파츠 저장 후 이름도 함께 저장.

```cpp
void Level_CharacterSetup::Finish_CharacterSetup()
{
    auto custom = GET_SINGLE(Customizer_Manager);

    // 파츠 저장 (기존)
    for (int i = 0; i < TAB_COUNT; ++i)
    {
        ContainerObject::EPartSlot slot = _tabSlots[i];
        if (!_catalog[ETOI(slot)].empty())
        {
            const wstring& assetTag = _catalog[ETOI(slot)][_equippedIndices[i]].modelAssetTag;
            custom->Set_Part(slot, assetTag);
        }
    }

    // [추가] 플레이어 이름 저장
    custom->Set_PlayerName(_pendingPlayerName);

    // 레벨 전환 (기존)
    EGameplaySpawnMode spawnMode = ...;
    GAME->Change_Level(...);
}
```

### `Customizer_Manager`에 이름 필드 추가

```cpp
// Customizer_Manager.h
public:
    void            Set_PlayerName(const wstring& name) { _playerName = name; }
    const wstring&  Get_PlayerName() const { return _playerName; }

private:
    wstring _playerName = L"닌자";  // 기본값
```

---

## 8. 인게임 적용

`Level_Gameplay` 또는 `MyPlayer` 스폰 시 이름 적용.

```cpp
// 예: MyPlayer 초기화 시
auto custom = GET_SINGLE(Customizer_Manager);
wstring name = custom->Get_PlayerName();
// Set_PlayerName(name) 또는 프로토콜 패킷에 이름 포함
```

---

## 요약 체크리스트

- [ ] `ESetupState::NameInput` 추가
- [ ] `_pendingPlayerName`, `_nameInputBg`, `_nameInputText` 멤버 추가
- [ ] `Ready_NameInputUI()` 구현 및 `Initialize()`에서 호출
- [ ] `Enter_NameInput()` 구현
- [ ] `Handle_NameInput()` 구현 및 `Update()` 분기 추가
- [ ] `Refresh_NameInputText()` 구현
- [ ] WM_CHAR 처리 (방법 A 권장) — `On_CharInput()` 추가
- [ ] `Customizer_Manager::Set_PlayerName()` / `Get_PlayerName()` 추가
- [ ] `Finish_CharacterSetup()`에서 이름 저장
- [ ] 인게임 스폰 시 이름 읽어서 적용
