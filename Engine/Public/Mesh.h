#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class Mesh final : public VIBuffer
{
    GENERATED_COMPONENT(Mesh, Protocol::COMPONENT_TYPE_MESH)

public:
    explicit Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Mesh(const Mesh& rhs);
    virtual ~Mesh() = default;

public:
    HRESULT Initialize_Prototype(const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices, bool keepCPUData = false);

    HRESULT Initialize_Prototype(const string& meshName, uint32 materialIndex,
        const vector<VTXANIM>& vertices, const vector<uint32>& indices);

    HRESULT Initialize(void* arg) override;

    const uint32  Get_MaterialIndex() const { return _materialIndex; }
    const string& Get_MeshName() const      { return _meshName; }

    const vector<Vec3>& Get_CPUPositions() const { return _cpuPositions; }
    const vector<uint32>& Get_CPUIndices() const { return _cpuIndices; }

    bool  Get_KeepCPUData() const { return _keepCPUData; }
    void  Set_KeepCPUData(bool keep) { _keepCPUData = keep; }

private:
    HRESULT Create_Buffers(const vector<VTXMESH>& vertices, const vector<uint32>& indices);
    HRESULT Create_Buffers(const vector<VTXANIM>& vertices, const vector<uint32>& indices);

private:
    uint32  _materialIndex = 0;
    string  _meshName;

    EMeshVertexType _vertexType = EMeshVertexType::StaticMesh;

private:
    // CPU에 보관할 정점 위치, 인덱스
    bool            _keepCPUData = false;
    vector<Vec3>    _cpuPositions;
    vector<uint32>  _cpuIndices;

public:
    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices, bool keepCPUData = false);

    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const string& meshName, uint32 materialIndex,
        const vector<VTXANIM>& vertices, const vector<uint32>& indices);

    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
