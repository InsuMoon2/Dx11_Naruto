# MainTitle UI (텍스트 동적 생성 방식) A to Z 구현 가이드

사용자 요청 내용: **"메뉴 버튼의 텍스처는 공용 배경을 쓰고, 버튼 이름은 텍스트(UI_Text)로 렌더링하도록 만들어 달라. 코딩 컨벤션을 기존 하위 UI(_bufferCom, CHECK_FAILED 등) 구조와 완벽히 맞출 것."**
추가 요청: **"UIObject를 상속받는 클래스이므로, Create_Child(HUD 전용) 대신 GAME->Add_UI를 통해 UI_Text를 생성하고 제어할 것."**

---

## 1. UI_MainTitleText (메뉴 버튼 객체) 완성하기

### A. UI_MainTitleText.h
```cpp
#pragma once
#include "UIObject.h"

NS_BEGIN(Engine)
class UI_Text; 
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_MainTitleText : public UIObject
{
    GENERATED_BODY(UI_MainTitleText)

public:
    struct FMainTitleMenuDesc : public Engine::UIObject::FUIDesc
    {
        uint32  bgTextureIndex = 0; // 공용 배경 텍스처 배열 번호
        wstring labelText = L"";    // 버튼 텍스트 (예: L"게임 시작")
    };

public:
    explicit UI_MainTitleText(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_MainTitleText(const UI_MainTitleText& rhs);
    virtual ~UI_MainTitleText() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

public:
    void        Set_Selected(bool bSelected);
    virtual void Set_Visibility(bool active) override; // 텍스트도 같이 꺼지도록 오버라이드

protected:
    HRESULT     Ready_Components() override;
    HRESULT     Ready_ChildText(const FMainTitleMenuDesc* desc);

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

    Shared<UI_Text>         _labelUI; // GAME->Add_UI 로 얻어올 텍스트 포인터

    bool    _isSelected = false;
    float   _baseScaleX = 1.f;
    float   _baseScaleY = 1.f;

public:
    static Shared<UI_MainTitleText> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
```

### B. UI_MainTitleText.cpp
```cpp
#include "pch.h"
#include "UI_MainTitleText.h"
#include "UI_Text.h"
#include "Texture.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "GameInstance.h"

UI_MainTitleText::UI_MainTitleText(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT);
}

UI_MainTitleText::UI_MainTitleText(const UI_MainTitleText& rhs)
    : UIObject(rhs)
{
}

HRESULT UI_MainTitleText::Initialize_Prototype()
{
    return UIObject::Initialize_Prototype();
}

HRESULT UI_MainTitleText::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    const auto* desc = static_cast<FMainTitleMenuDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    CHECK_FAILED(Ready_Components(), E_FAIL);
    CHECK_FAILED(Ready_ChildText(desc), E_FAIL);

    _baseScaleX = _transformCom->Get_Scale().x;
    _baseScaleY = _transformCom->Get_Scale().y;

    Set_Selected(false); // 초기 오프 상태

    return S_OK;
}

void UI_MainTitleText::Priority_Update(float timeDelta)
{
    UIObject::Priority_Update(timeDelta);
}

void UI_MainTitleText::Update(float timeDelta)
{
    UIObject::Update(timeDelta);
    
    __super::Update_Transform();

    // 부모 위치가 이동하거나 스케일이 바뀌면 텍스트도 동기화 (옵션)
    if (_labelUI && _isVisible)
    {
        _labelUI->Set_UIPosition(_posX, _posY);
    }
}

void UI_MainTitleText::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);

    if (_active && _isVisible)
    {
        // 렌더링 큐 등록
        // 텍스트는 GAME->Add_UI를 통해 추가된 독립된 GameObject이므로 자신의 렌더링을 스스로 수행합니다.
        GAME->Add_RenderGroup(ERenderGroup::UI, shared_from_this());
    }
}

HRESULT UI_MainTitleText::Render()
{
    if (!_isVisible) return S_OK;

    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_worldMatrix), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ViewMatrix", ETransformState::View), E_FAIL);
    CHECK_FAILED(__super::Bind_ShaderResource(_shaderCom, "g_ProjMatrix", ETransformState::Proj), E_FAIL);

    // 전달받은 bgTextureIndex를 기반으로 배경을 그림 (이 프로젝트 구조에 따라 Bind_SRV 인덱스 혹은 컴포넌트를 조절해야할 수 있음)
    // 아래는 0번에 그 텍스처가 있다는 가정
    CHECK_FAILED(_textureCom->Bind_SRV(_shaderCom, "g_Texture", 0), E_FAIL);

    // 하이라이트 색상 배율
    float highlight = _isSelected ? 1.0f : 0.6f;
    Vec4 color = { highlight, highlight, highlight, 1.f };
    CHECK_FAILED(_shaderCom->Bind_RawValue("g_Color", &color, sizeof(Vec4)), E_FAIL);

    CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL); 

    CHECK_FAILED(_bufferCom->Bind_Resources(), E_FAIL);
    CHECK_FAILED(_bufferCom->Render(), E_FAIL);

    return S_OK;
}

void UI_MainTitleText::Set_Selected(bool bSelected)
{
    _isSelected = bSelected;

    // 크기 연출
    if (_isSelected)
        _transformCom->Set_LocalScale(_baseScaleX * 1.15f, _baseScaleY * 1.15f, 1.f);
    else
        _transformCom->Set_LocalScale(_baseScaleX, _baseScaleY, 1.f);

    // 자식 텍스트 폰트 색상 연출
    if (_labelUI)
    {
        FTextStyle style = _labelUI->Get_TextStyle();
        if (_isSelected)
            style.color = Color(1.f, 0.9f, 0.2f, 1.f); // 선택: 밝은 노랑
        else
            style.color = Color(0.7f, 0.7f, 0.7f, 1.f); // 평소: 회색
        
        _labelUI->Set_TextStyle(style);
    }
}

void UI_MainTitleText::Set_Visibility(bool active)
{
    UIObject::Set_Visibility(active);
    if (_labelUI)
        _labelUI->Set_Visibility(active);
}

HRESULT UI_MainTitleText::Ready_Components()
{
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_TEXTURE_MAIN_TITLE, _textureCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_UI, _shaderCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_RECT, _bufferCom), E_FAIL);

    return S_OK;
}

HRESULT UI_MainTitleText::Ready_ChildText(const FMainTitleMenuDesc* desc)
{
    // GAME->Add_UI 를 통해 텍스트 객체를 생성하여 메뉴 객체 위에 렌더링되게 만듭니다.
    UI_Text::FUITextDesc textDesc{};
    textDesc.name = desc->name + TEXT("_Label");
    textDesc.posX = desc->posX; 
    textDesc.posY = desc->posY;
    textDesc.sizeX = desc->sizeX; // 글씨가 그려질 영역의 크기
    textDesc.sizeY = desc->sizeY;
    
    // 배경판보다 위에 그려지도록 Z 좌표 조정 (작을수록 앞으로 옴)
    textDesc.zOrder = desc->zOrder - 0.001f; 
    textDesc.levelIndex = desc->levelIndex;

    textDesc.text = desc->labelText;   
    
    textDesc.style.fontFamily = L"Malgun Gothic";
    textDesc.style.fontSize = 28.f;
    textDesc.style.color = Color(0.7f, 0.7f, 0.7f, 1.f);
    textDesc.style.hAlign = ETextHAlign::Center;
    textDesc.style.vAlign = ETextVAlign::Center;
    textDesc.style.wordWrap = false;

    // GAME 인스턴스를 활용해 UI 시스템에 직접 등록
    // 반환된 객체의 소유권(Shared pointer)을 _labelUI에 보관하여 연출 시 제어
    _labelUI = static_pointer_cast<UI_Text>(
        GAME->Add_UI(
            Protocol::OBJECT_TYPE_UI_TEXT,
            EUILayer::Overlay,
            &textDesc));
            
    CHECK_NULL(_labelUI, E_FAIL);

    return S_OK;
}

Shared<UI_MainTitleText> UI_MainTitleText::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_MainTitleText>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_MainTitleText");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_MainTitleText::Clone(void* arg)
{
    auto clone = make_shared<UI_MainTitleText>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_MainTitleText");
        return nullptr;
    }

    return clone;
}

void UI_MainTitleText::Free()
{
    // 본체 파괴 시 등록되었던 자식(종속) 텍스트도 같이 날려준다
    if (_labelUI)
        _labelUI->Set_Dead(true);

    UIObject::Free();
}
```

---

## 2. Level_MainTitle 제어 흐름 완성하기

### A. Level_MainTitle.h 추가
```cpp
// 헤더 위쪽이나 내부에 추가 (enum class)
enum class ETitleState { PressSpace, SelectMenu };

// private 내부
private:
    ETitleState                 _titleState = ETitleState::PressSpace;
    
    Shared<GameObject>          _pressTextUI;
    Shared<class UI_MainTitleText> _menuBtns[3];
    int                         _selectedIndex = 0;

private:
    HRESULT Ready_Layer_MenuTebs();
```

### B. Level_MainTitle.cpp
```cpp
HRESULT Level_MainTitle::Ready_Layer_UI()
{
    // [수정] Press Text 생성 부분을 맴버로 받음
    {
        Background::FBackgroundDesc desc{};
        // ... (생략)
        
        _pressTextUI = GAME->Add_UI(Protocol::OBJECT_TYPE_BACKGROUND, EUILayer::Overlay, &desc);
        CHECK_NULL(_pressTextUI, E_FAIL);

        GAME->Play_UIAnimation(_pressTextUI, "PressText");
    }
}

HRESULT Level_MainTitle::Ready_Layer_MenuTebs()
{
    Vec2 viewport = { GAME->Get_WindowWidth(), GAME->Get_WindowHeight() };

    // 화면 정중앙 보다 살짝 아래부터 배치 시작
    float startX = viewport.x * 0.5f;
    float startY = viewport.y * 0.55f;
    float gapY   = 80.f;

    wstring menuLabels[3] = { L"게 임 시 작", L"환 경 설 정", L"게 임 종 료" };
    uint32 emptyBtnBgIndex = 15; // 실제 빈 배경 텍스처 배열 인덱스

    for (int i = 0; i < 3; ++i)
    {
        UI_MainTitleText::FMainTitleMenuDesc desc{};
        desc.name = TEXT("MenuBtn_") + to_wstring(i);
        desc.posX = startX;
        desc.posY = startY + (i * gapY);
        desc.sizeX = 250.f; 
        desc.sizeY = 60.f;
        desc.zOrder = 0.4f;
        desc.levelIndex = ETOI(ELevelType::MainTitle);
        
        desc.bgTextureIndex = emptyBtnBgIndex; 
        desc.labelText = menuLabels[i];

        _menuBtns[i] = static_pointer_cast<UI_MainTitleText>(
            GAME->Add_UI(Protocol::OBJECT_TYPE_UI_MAIN_TITLE_TEXT, EUILayer::Overlay, &desc));
            
        CHECK_NULL(_menuBtns[i], E_FAIL);
    }

    return S_OK;
}

void Level_MainTitle::Update(float timeDelta)
{
    Level::Update(timeDelta);

    if (_titleState == ETitleState::PressSpace)
    {
        if (INPUT->KeyDown(KEY_TYPE::SPACE))
        {
            if (_pressTextUI)
                _pressTextUI->Set_Dead(true); // Press Space 지우기

            Ready_Layer_MenuTebs();       // 메뉴 버튼 + UI_Text 생성 및 연동

            _selectedIndex = 0;
            _menuBtns[0]->Set_Selected(true);

            _titleState = ETitleState::SelectMenu;
        }
    }
    else if (_titleState == ETitleState::SelectMenu)
    {
        if (INPUT->KeyDown(KEY_TYPE::UP) || INPUT->KeyDown(KEY_TYPE::W))
        {
            _menuBtns[_selectedIndex]->Set_Selected(false);
            _selectedIndex = (_selectedIndex - 1 + 3) % 3;
            _menuBtns[_selectedIndex]->Set_Selected(true);
        }
        
        if (INPUT->KeyDown(KEY_TYPE::DOWN) || INPUT->KeyDown(KEY_TYPE::S))
        {
            _menuBtns[_selectedIndex]->Set_Selected(false);
            _selectedIndex = (_selectedIndex + 1) % 3;
            _menuBtns[_selectedIndex]->Set_Selected(true);
        }

        if (INPUT->KeyDown(KEY_TYPE::ENTER) || INPUT->KeyDown(KEY_TYPE::SPACE))
        {
            switch (_selectedIndex)
            {
            case 0:
                GAME->Change_Level(ETOI(ELevelType::Loading),
                    Level_Loading::Create(_device, _context, ELevelType::GamePlay, true));
                break;
            case 1:
                // TODO: 캐릭터/세팅 창 연결
                break;
            case 2:
                PostQuitMessage(0);
                break;
            }
        }
    }
}
```
