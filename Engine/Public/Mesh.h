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
    HRESULT Initialize_Prototype(const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices);

    HRESULT Initialize(void* arg) override;

    const uint32  Get_MaterialIndex() const { return _materialIndex; }
    const string& Get_MeshName() const { return _meshName; }

private:
    HRESULT Create_Buffers(const vector<VTXMESH>& vertices, const vector<uint32>& indices);

private:
    uint32  _materialIndex = 0;
    string  _meshName;

public:
    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices);

    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
