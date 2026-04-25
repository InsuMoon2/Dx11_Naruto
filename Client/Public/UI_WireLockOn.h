#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Shader;
class Texture;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_WireLockOn : public UIObject
{
    GENERATED_BODY(UI_WireLockOn)

public:
    struct FWireLockOnDesc : public UIObject::FUIDesc
    {
        // 원본 사이즈가 둘다 512 512
        float bgSize = 512.f * 0.5f;
        float lockSize = 512.f;
    };

public:
    explicit UI_WireLockOn(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_WireLockOn(const UI_WireLockOn& rhs);
    virtual ~UI_WireLockOn() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void Show_LockOn(float x, float y);
    void Hide_LockOn();

private:
    HRESULT Ready_Components() override;    
    HRESULT Render_Texture(uint32 textureIndex, const Matrix& worldMatrix, float alpha);

private:
    Shared<Shader>        _shaderCom;       
    Shared<Texture>       _textureCom;      
    Shared<VIBuffer_Rect> _bufferCom;       

private:
    float _bgSize = 180.f;
    float _lockSize = 96.f;
    float _animTime = 0.f;
    float _innerAlpha = 1.f;

private:
    static constexpr uint32 LOCK_BG_TEXTURE_INDEX = 0;
    static constexpr uint32 LOCK_INNER_TEXTURE_INDEX = 1;

public:
    static Shared<UI_WireLockOn> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;


};

NS_END
