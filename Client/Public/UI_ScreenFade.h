#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_ScreenFade final : public UIObject
{
    GENERATED_BODY(UI_ScreenFade)

public:
    struct FScreenFadeDesc : public UIObject::FUIDesc
    {
        Color fadeColor = Color(0.f, 0.f, 0.f, 1.f); 
        float initialAlpha = 0.f;                    
    };

public:
    explicit UI_ScreenFade(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_ScreenFade(const UI_ScreenFade& rhs);
    virtual ~UI_ScreenFade() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Set_FadeAlpha(float alpha);          
    void    Set_FadeColor(const Color& color);   

protected:
    HRESULT Ready_Components() override;

private:
    Shared<Shader>        _shaderCom;         
    Shared<VIBuffer_Rect> _bufferCom;         

    Color _fadeColor = Color(0.f, 0.f, 0.f, 1.f); 
    float _fadeAlpha = 0.f;

public:
    static Shared<UI_ScreenFade> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
