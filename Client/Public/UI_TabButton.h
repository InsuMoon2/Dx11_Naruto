#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
class UI_Text;
NS_END

NS_BEGIN(Client)

class UI_TabButton : public UIObject
{
    GENERATED_BODY(UI_TabButton)

public:
    struct FUITabDesc : public FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;

        wstring labelText = L"";

        Vec2 labelOffset = Vec2::Zero;
        Vec2 labelSize = Vec2::Zero;

        float fontSize = 24.f;;
    };

public:
    explicit UI_TabButton(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_TabButton(const UI_TabButton& rhs);
    virtual ~UI_TabButton() = default;

public:
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Set_Selected(bool bSelected);
    void    Set_Visibility(bool active) override;

private:
    HRESULT Ready_Components();
    HRESULT Ready_ChildText(const FUITabDesc* desc);

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;
    Shared<UI_Text>         _labelUI;

private:
    bool    _isSelected = false;

public:
    static Shared<UI_TabButton> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
