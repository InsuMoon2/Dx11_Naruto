#include "pch.h"
#include "Shader.h"

Shader::Shader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Shader::Shader(const Shader& rhs)
    : Component(rhs)
    , _effect(rhs._effect)
{
}

Shader::~Shader()
{
}

HRESULT Shader::Initialize_Prototype(const wstring& shaderFilePath)
{
    uint32 hlslFlag = {};

#ifdef _DEBUG
    hlslFlag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
    hlslFlag |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

    //ComPtr<ID3DBlob> blob;
    CHECK_FAILED(D3DX11CompileEffectFromFile(shaderFilePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        hlslFlag, 0, _device.Get(), &_effect, nullptr), E_FAIL);

    ComPtr<ID3DX11EffectTechnique> technique = _effect->GetTechniqueByIndex(0);
    CHECK_NULL(technique, E_FAIL);

    D3DX11_TECHNIQUE_DESC techniqueDesc {};

    technique->GetDesc(&techniqueDesc);

    _numPasses = techniqueDesc.Passes;

    for (size_t i = 0; i < _numPasses; i++)
    {
        ComPtr<ID3DX11EffectPass> pass = technique->GetPassByIndex(i);
        CHECK_NULL(pass, E_FAIL);

        D3DX11_PASS_DESC passDesc{};
        pass->GetDesc(&passDesc);

        //passDesc.pIAInputSignature, passDesc.IAInputSignatureSize
    }


    return S_OK;
}

HRESULT Shader::Initialize(void* arg)
{

    return S_OK;
}

Shared<Shader> Shader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& shaderPath)
{
    auto instance = make_shared<Shader>(device, context);

    if (FAILED(instance->Initialize_Prototype(shaderPath)))
    {
        MSG_BOX("Failed to Created : Shader");

        return nullptr;
    }

    return instance;
}

Shared<Component> Shader::Clone(void* arg)
{
    auto instance = make_shared<Shader>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : Shader");

        return nullptr;
    }

    return instance;
}

void Shader::Free()
{
    Component::Free();
}
