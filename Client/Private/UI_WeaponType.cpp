#include "pch.h"
#include "UI_WeaponType.h"
#include "Shader.h"
#include "VIBuffer_Rect.h"
#include "Texture.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(UI_WeaponType, Protocol::OBJECT_TYPE_UI_WEAPON_TYPE)
IMPLEMENT_REFLECTION(UI_WeaponType)

bool UI_WeaponType::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_WeaponType";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();
    PROPERTY_ENUM_CUSTOM("무기 타입", _weaponTypeBG,
        (vector<string>{"지원형", "방어형", "격투형", "검술형"}));

    return true;
}

UI_WeaponType::UI_WeaponType(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Background(device, context)
{
    
}

UI_WeaponType::UI_WeaponType(const UI_WeaponType& rhs)
    : Background(rhs)
    , _weaponTypeBG(rhs._weaponTypeBG)
{
}

HRESULT UI_WeaponType::Initialize_Prototype()
{
    CHECK_FAILED(Background::Initialize_Prototype(), E_FAIL);


    return S_OK;
}

HRESULT UI_WeaponType::Initialize(void* arg)
{
    auto* desc = static_cast<FWeaponTypeDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);
    
    _weaponTypeBG = desc->weaponTypeBG;

    desc->textureType = Protocol::COMPONENT_TYPE_TEXTURE_WEAPON_TYPE;
    desc->textureIndex = To_TextureIndex(_weaponTypeBG);

    Setup_DefaultTextDesc(desc->textDesc);
    desc->textDesc.text = To_TypeName(_weaponTypeBG);

    CHECK_FAILED(Background::Initialize(arg), E_FAIL);

    Apply_WeaponTypeVisual();

    return S_OK;
}

void UI_WeaponType::Update(float timeDelta)
{
    Background::Update(timeDelta);

    __super::Update_Transform();
}

HRESULT UI_WeaponType::Render()
{
    return Background::Render();
}

void UI_WeaponType::Set_WeaponType(EWeaponTypeBG weaponTypeName)
{
    _weaponTypeBG = weaponTypeName;

    Apply_WeaponTypeVisual();
}

void UI_WeaponType::Setup_DefaultTextDesc(FBackgroundTextDesc& textDesc)
{
    textDesc.offset = Vec2(0.f, 0.f);
    textDesc.size = Vec2(180.f, 32.f);
    textDesc.zOrderOffset = 0.01f;

    textDesc.style.fontFamily = UI_DEFAULT_FONT_FAMILY;
    textDesc.style.fontSize = 22.f;
    textDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    textDesc.style.hAlign = ETextHAlign::Center;
    textDesc.style.vAlign = ETextVAlign::Middle;
    textDesc.style.wordWrap = false;
}

void UI_WeaponType::Apply_WeaponTypeVisual()
{
    Set_BackgroundTextureIndex(To_TextureIndex(_weaponTypeBG));
    Set_LabelText(To_TypeName(_weaponTypeBG));
}

uint32 UI_WeaponType::To_TextureIndex(EWeaponTypeBG type)
{
    switch (type)
    {
    case EWeaponTypeBG::Support:    return 0;
    case EWeaponTypeBG::Defense:    return 1;
    case EWeaponTypeBG::Fighter:    return 2;
    case EWeaponTypeBG::Sword:      return 3;
    default:                        return 0;
    }
}

wstring UI_WeaponType::To_TypeName(EWeaponTypeBG type)
{
    switch (type)
    {
    case EWeaponTypeBG::Support:    return L"지원형";
    case EWeaponTypeBG::Defense:    return L"방어형";
    case EWeaponTypeBG::Fighter:    return L"격투형";
    case EWeaponTypeBG::Sword:      return L"검술형";
    default:                        return L"격투형";
    }
}

Shared<UI_WeaponType> UI_WeaponType::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_WeaponType>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : UI_WeaponType");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_WeaponType::Clone(void* arg)
{
    auto clone = make_shared<UI_WeaponType>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_WeaponType");

        return nullptr;
    }

    return clone;
}

void UI_WeaponType::Free()
{
    Background::Free();
}
