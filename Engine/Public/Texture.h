#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL Texture : public Component
{
public:
    explicit Texture(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Texture(const Texture& rhs);
    virtual ~Texture();

public:
    HRESULT Initialize_Prototype(const wstring& texturePath, uint32 numSRVs);
    HRESULT Initialize(any arg) override;

public:
    uint32 _numSRVs = 0;
    vector<ComPtr<ShaderResourceView>> _SRVs;

public:
    static shared_ptr<Texture> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const wstring& texturePath, uint32 numSRVs);
    shared_ptr<Component> Clone(any arg) override;
    void Free() override;
};

NS_END
