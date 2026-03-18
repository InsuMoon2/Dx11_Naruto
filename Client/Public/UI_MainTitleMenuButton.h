#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class VIBuffer_Rect;
class UI_Text;
NS_END

NS_BEGIN(Client)

class UI_MainTitleMenuButton : public UIObject
{
    GENERATED_BODY(UI_MainTitleMenuButton)

public:
    struct FMainTitleMenuDesc : public FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_DEFAULT;

        wstring labelText = L"";

        Vec2 labelOffset = Vec2::Zero;
        Vec2 labelSize = Vec2::Zero;

        float fontSize = 24.f;;
    };

public:
    explicit UI_MainTitleMenuButton(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_MainTitleMenuButton(const UI_MainTitleMenuButton& rhs);
    virtual ~UI_MainTitleMenuButton() = default;

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
    HRESULT Ready_ChildText(const FMainTitleMenuDesc* desc);

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;
    Shared<UI_Text>         _labelUI;

private:
    bool    _isSelected = false;
    float   _baseScaleX = 1.f;
    float   _baseScaleY = 1.f;

public:
    static Shared<UI_MainTitleMenuButton> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};


NS_END
