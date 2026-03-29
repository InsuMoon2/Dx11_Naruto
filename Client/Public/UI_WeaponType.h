#pragma once

#include "Background.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_WeaponType final : public Background
{
    GENERATED_BODY(UI_WeaponType)

public:
    enum class EWeaponTypeBG { Support, Defense, Fighter, Sword, END };

    struct FWeaponTypeDesc : public Background::FBackgroundDesc
    {
        EWeaponTypeBG weaponTypeBG = EWeaponTypeBG::Fighter;
    };

public:
    explicit UI_WeaponType(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_WeaponType(const UI_WeaponType& rhs);
    virtual ~UI_WeaponType() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Set_WeaponType(EWeaponTypeBG weaponTypeName);

    static void Setup_DefaultTextDesc(FBackgroundTextDesc& textDesc);
    void        Apply_WeaponTypeVisual();

private: // Enum변환용
    static uint32       To_TextureIndex(EWeaponTypeBG type);
    static wstring      To_TypeName(EWeaponTypeBG type);

private:
    EWeaponTypeBG         _weaponTypeBG = EWeaponTypeBG::Fighter;

private:
    Shared<Shader>        _shaderCom;

    Shared<Texture>       _iconTextureCom;
    Shared<VIBuffer_Rect> _bufferCom;

public:
    static Shared<UI_WeaponType> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};



NS_END
