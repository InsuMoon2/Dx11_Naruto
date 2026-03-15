#pragma once

#include "UIObject.h"
#include "Text_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL UI_Text : public UIObject
{
    GENERATED_BODY(UI_Text)

public:
    struct FUITextDesc : public UIObject::FUIDesc
    {
        wstring      text;
        FTextStyle   style;
    };

public:
    explicit UI_Text(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_Text(const UI_Text& rhs);
    virtual ~UI_Text() = default;

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
    RECT        Build_ScreenRect() const;

private:
    wstring     _text;
    FTextStyle  _style;

public:
    static Shared<UI_Text>   Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject>      Clone(void* arg) override;
    void                    Free() override;
};

NS_END
