#pragma once

#include "UIObject.h"
#include "Text_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL UIText : public UIObject
{
    GENERATED_BODY(UIText)

public:
    struct FUITextDesc : public UIObject::FUIDesc
    {
        wstring      text;
        FTextStyle   style;
    };

public:
    explicit UIText(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UIText(const UIText& rhs);
    virtual ~UIText() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void           Set_Text(const wstring& text) { _text = text; }
    const wstring& Get_Text() const { return _text; }

    void              Set_TextStyle(const FTextStyle& style) { _style = style; }
    const FTextStyle& Get_TextStyle() const { return _style; }

private:
    RECT Build_ScreenRect() const;

private:
    wstring     _text;
    FTextStyle  _style;

public:
    static Shared<UIText>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg);
    Shared<GameObject>      Clone(void* arg) override;
    void                    Free() override;
};

NS_END
