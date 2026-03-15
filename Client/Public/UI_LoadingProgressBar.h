#pragma once

#include "UIObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END

NS_BEGIN(Client)

class Loader;

class UI_LoadingProgressBar : public UIObject
{
    GENERATED_BODY(UI_LoadingProgressBar)

public:
    struct FLoadingProgressBarDesc : public UIObject::FUIDesc
    {
        uint32 textureIndex = 0;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;
    };

public:
    explicit UI_LoadingProgressBar(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_LoadingProgressBar(const UI_LoadingProgressBar& rhs);
    virtual ~UI_LoadingProgressBar();

public:
    HRESULT Initialize(void* arg) override;
    HRESULT Initialize_Prototype() override;
    void    Update(float timeDelta) override;
    HRESULT Render() override;

public:
    void    Bind_Loader(Shared<Loader> loader) { _loader = loader; }

protected:
    HRESULT Ready_Components() override;

private:
    Weak<Loader> _loader;
    float  _ratio = 0.f;
    uint32 _textureIndex = 0;
    uint32 _textureType = Protocol::COMPONENT_TYPE_TEXTURE_LOADING;

    Shared<Texture>       _textureCom;
    Shared<Shader>        _shaderCom;
    Shared<VIBuffer_Rect> _bufferCom;

public:
    static Shared<UIObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;


};

NS_END
