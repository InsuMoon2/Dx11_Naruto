#include "pch.h"
#include "Shader.h"

Shader::Shader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
    
}

Shader::Shader(const Shader& rhs)
    : Component(rhs), _effect(rhs._effect), _numPasses(rhs._numPasses),
    _inputLayouts(rhs._inputLayouts)
{
    
}

Shader::~Shader()
{
    
}

HRESULT Shader::Initialize_Prototype(const wstring& shaderFilePath,
    const D3D11_INPUT_ELEMENT_DESC* desc, uint32 numElements)
{
    uint32 hlslFlag = {};

#ifdef _DEBUG
    hlslFlag |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
    hlslFlag |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#endif

    // 셰이더 effect 컴파일 단계가 실패했는지 확인하기 위한 에러 버퍼
    ComPtr<ID3DBlob> errorBlob;

    const HRESULT compileHr = D3DX11CompileEffectFromFile(
        shaderFilePath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        hlslFlag,
        0,
        _device.Get(),
        &_effect,
        errorBlob.GetAddressOf());

    if (FAILED(compileHr))
    {
        string errorText = "(no compiler error text)";
        if (errorBlob && errorBlob->GetBufferPointer())
        {
            errorText.assign(
                static_cast<const char*>(errorBlob->GetBufferPointer()),
                errorBlob->GetBufferSize());
        }

        LOG_ERROR(
            "Shader effect compile failed. path='{}', hr=0x{:08X}, inputElements={}, error={}",
            Utils::ToString(shaderFilePath),
            static_cast<uint32>(compileHr),
            numElements,
            errorText);

        return E_FAIL;
    }

    ComPtr<ID3DX11EffectTechnique> technique = _effect->GetTechniqueByIndex(0);
    CHECK_NULL(technique, E_FAIL);

    D3DX11_TECHNIQUE_DESC techniqueDesc{};
    technique->GetDesc(&techniqueDesc);

    _numPasses = techniqueDesc.Passes;

    for (size_t i = 0; i < _numPasses; ++i)
    {
        ComPtr<ID3D11InputLayout> inputLayout;
        ComPtr<ID3DX11EffectPass> pass = technique->GetPassByIndex(i);
        CHECK_NULL(pass, E_FAIL);

        D3DX11_PASS_DESC passDesc{};
        pass->GetDesc(&passDesc);

        // 어느 pass의 input layout 생성이 실패하는지 보기 위한 진단 로그
        const HRESULT layoutHr = _device->CreateInputLayout(
            desc,
            numElements,
            passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize,
            &inputLayout);

        if (FAILED(layoutHr))
        {
            LOG_ERROR(
                "Shader input layout create failed. path='{}', passIndex={}, hr=0x{:08X}, inputElements={}",
                Utils::ToString(shaderFilePath),
                static_cast<uint32>(i),
                static_cast<uint32>(layoutHr),
                numElements);

            return E_FAIL;
        }

        _inputLayouts.push_back(inputLayout);
    }

    return S_OK;
}


HRESULT Shader::Initialize(void* arg)
{
    return S_OK;
}

HRESULT Shader::Begin_Pass(uint32 passIndex)
{
    if (passIndex >= _numPasses || _inputLayouts[passIndex] == nullptr)
        return E_FAIL;

    _effect->GetTechniqueByIndex(0)->GetPassByIndex(passIndex)->Apply(0, _context.Get());

    _context->IASetInputLayout(_inputLayouts[passIndex].Get());

    return S_OK;
}

HRESULT Shader::Bind_SRV(const char* constantName, ComPtr<ShaderResourceView> SRV)
{
    ComPtr<ID3DX11EffectVariable> variable = _effect->GetVariableByName(constantName);
    if (!variable->IsValid())
        return E_FAIL;

    ComPtr<ID3DX11EffectShaderResourceVariable> srvVariable = variable->AsShaderResource();
    if (!srvVariable->IsValid())
        return E_FAIL;

    CHECK_FAILED(srvVariable->SetResource(SRV.Get()), E_FAIL);

    return S_OK;
}

HRESULT Shader::Bind_Matrix(const char* constantName, const Matrix* matrix)
{
    ComPtr<ID3DX11EffectVariable> variable = _effect->GetVariableByName(constantName);
    if (!variable->IsValid())
        return E_FAIL;

    ComPtr<ID3DX11EffectMatrixVariable> matrixVariable = variable->AsMatrix();
    if (!matrixVariable->IsValid())
        return E_FAIL;

    CHECK_FAILED(
        matrixVariable->SetMatrix(reinterpret_cast<const float*>(matrix)),
        E_FAIL);

    return S_OK;
}

HRESULT Shader::Bind_RawValue(const char* constantName, const void* data, uint32 length)
{
    ComPtr<ID3DX11EffectVariable> variable = _effect->GetVariableByName(constantName);
    if (!variable->IsValid())
        return E_FAIL;

    return variable->SetRawValue(data, 0, length);
}

json Shader::To_Json() const
{
    json j = Component::To_Json();



    return j;
}

void Shader::From_Json(const json& data)
{
    Component::From_Json(data);


}

Shared<Shader> Shader::Create(ComPtr<Device> device,
                              ComPtr<DeviceContext> context)
{
    return Create(device, context, L"../../Client/Bin/Shaders/Shader_VtxTex.hlsl",
        FVertexTex::Elements,
        FVertexTex::numElements);
}

Shared<Shader> Shader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const wstring& shaderPath, const D3D11_INPUT_ELEMENT_DESC* desc, uint32 numElements)
{
    auto instance = make_shared<Shader>(device, context);

    if (FAILED(instance->Initialize_Prototype(shaderPath, desc, numElements)))
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
