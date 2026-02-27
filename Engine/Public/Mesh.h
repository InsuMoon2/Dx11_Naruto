#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class Mesh final : public VIBuffer
{
    GENERATED_COMPONENT(Mesh, Protocol::COMPONENT_TYPE_MESH)

public:
    explicit Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Mesh(const Mesh& rhs);
    virtual ~Mesh();

public:
    HRESULT Initialize_Prototype(const aiMesh* aiMesh);
    HRESULT Initialize(void* arg) override;

public:
    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const aiMesh* aiMesh);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
