#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class UI_LoadingSpinner : public UIObject
{
    GENERATED_BODY(UI_LoadingSpinner)

public:
    struct FLoadingSpinnerDesc : public UIObject::FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
        float rotationSpeed = XMConvertToRadians(180.f);
    };

public:
    explicit UI_LoadingSpinner(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_LoadingSpinner(const UI_LoadingSpinner& rhs);
    virtual ~UI_LoadingSpinner() = default;

public:
    HRESULT Initialize(void* arg) override;
    HRESULT Initialize_Prototype() override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

protected:
    HRESULT Ready_Components() override;

private:
    Shared<Texture>       _textureCom;
    Shared<Shader>        _shaderCom;
    Shared<VIBuffer_Rect> _bufferCom;

private:
    uint32 _textureIndex = 0;
    uint32 _textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
    float  _rotationSpeed = 0.f;
    float  _angle = 0.f;

public:
    static Shared<UIObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject>      Clone(void* arg) override;
    virtual void Free() override;


};

NS_END
