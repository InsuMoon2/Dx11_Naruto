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
    virtual HRESULT Initialize_Prototype(const wstring& shaderFilePath, const D3D11_INPUT_ELEMENT_DESC* desc, uint32 numElements);
    virtual HRESULT Initialize(void* arg) override;

    HRESULT Begin_Pass(uint32 passIndex);
    HRESULT Bind_SRV(const char* constantName, ComPtr<ShaderResourceView> SRV);
    HRESULT Bind_Matrix(const char* constantName, const Matrix* matrix);
    HRESULT Bind_RawValue(const char* constantName, const void* data, uint32 length);

    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    HRESULT Clone_EffectFrom(const Shader& rhs);

private:
    ComPtr<ID3DX11Effect>               _effect;
    uint32                              _numPasses = {};

    vector<ComPtr<ID3D11InputLayout>>   _inputLayouts;

public:
    static Shared<Shader> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    static Shared<Shader> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& shaderPath,
                                    const D3D11_INPUT_ELEMENT_DESC* desc, uint32 numElements);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
    
};

NS_END
