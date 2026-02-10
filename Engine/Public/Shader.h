#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL Shader : public Component
{
    GENERATED_COMPONENT(Shader, Protocol::COMPONENT_TYPE_SHADER)

public:
    explicit Shader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Shader(const Shader& rhs);
    virtual ~Shader();

public:
    virtual HRESULT Initialize_Prototype(const wstring& shaderFilePath);
    virtual HRESULT Initialize(void* arg) override;

private:
    ComPtr<ID3DX11Effect>   _effect;
    uint32                  _numPasses = {};

public:
    static Shared<Shader> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& shaderPath);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
    
};

NS_END
