#pragma once

#include "UIObject.h"
#include "UI_Text.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Background : public UIObject
{
    GENERATED_BODY(Background)

public:
    struct FBackgroundTextDesc final
    {
        wstring     text = L"";
        Vec2        offset = Vec2::Zero;
        Vec2        size = Vec2::Zero;
        FTextStyle  style = {};

        float       zOrderOffset = 0.01f;
    };

    struct FBackgroundDesc : public UIObject::FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

        FBackgroundTextDesc textDesc{};
    };

public:
    explicit Background(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Background(const Background& rhs);
    virtual ~Background();

public:
    HRESULT     Initialize(void* arg) override;
    HRESULT     Initialize_Prototype() override;
    void        Priority_Update(float timeDelta) override;
    void        Update(float timeDelta) override;
    void        Late_Update(float timeDelta) override;
    HRESULT     Render() override;

    void        Set_Visibility(bool active) override;

    HRESULT     Setup_OptionalText(EUILayer uiLayer);

    HRESULT     On_UIRegistered() override;

    void        Set_LabelText(const wstring& text);

    void        Set_BackgroundTextureIndex(uint32 textureIndex) { _textureIndex = textureIndex; }

protected:
    HRESULT     Ready_Components() override;

private:
    HRESULT         Ready_ChildText(EUILayer uiLayer);
    Shared<UI_Text> Create_ChildText(EUILayer uiLayer, UI_Text::FUITextDesc& textDesc);

private:
    Shared<Texture>         _textureCom;
    Shared<Shader>          _shaderCom;
    Shared<VIBuffer_Rect>   _bufferCom;
    Shared<UI_Text>         _labelUI;

private:
    uint32                  _textureIndex = 0;
    uint32                  _textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;
    FBackgroundTextDesc     _textDesc;

public:
    static Shared<UIObject>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject>        Clone(void* arg) override;
    virtual void              Free() override;
    
};

NS_END
