#pragma once

#include "Base.h"

NS_BEGIN(Client)

class ResourceLoader : public Base
{
public:
    explicit ResourceLoader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~ResourceLoader() = default;

public:
    HRESULT Initialize();

    HRESULT Load_Table(const wstring& tablePath, uint32 levelIndex);

private:
    HRESULT Load_Components(const json& data, uint32 levelIndex, const string& typeName);

    uint32  Get_ComponentID_From_String(const string& idStr);

private:
    ComPtr<Device> _device;
    ComPtr<DeviceContext> _context;

public:
    static shared_ptr<ResourceLoader> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    void Free() override;
};

NS_END
