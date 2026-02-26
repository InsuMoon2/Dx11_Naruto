#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL Texture : public Component
{
    GENERATED_COMPONENT(Texture, Protocol::COMPONENT_TYPE_TEXTURE)

public:
    explicit Texture(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Texture(const Texture& rhs);
    virtual ~Texture();

public:
    HRESULT Initialize_Prototype(const wstring& texturePath, uint32 numSRVs);
    HRESULT Initialize(void* arg) override;

    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    HRESULT Bind_SRV(Shared<Shader> shader, const char* constantName, uint32 index);
    vector<ComPtr<ShaderResourceView>>& Get_SRVs() { return _SRVs; }

    HRESULT Add_SRV(const wstring& filePath);

    uint32 Get_CurrentIndex() const { return _currentIndex; }
    void   Set_CurrentIndex(uint32 idx) { _currentIndex = min(idx, _numSRVs - 1); }

private:
    uint32 _numSRVs = 0;
    uint32 _currentIndex = 0; // 현재 사용중인 Index

    vector<ComPtr<ShaderResourceView>> _SRVs;

    wstring _texturePath;

public:
    static shared_ptr<Texture> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& texturePath, uint32 numSRVs);
    shared_ptr<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
