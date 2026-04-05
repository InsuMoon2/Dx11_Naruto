#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

Mesh::Mesh(const Mesh& rhs)
    : VIBuffer(rhs)
    , _materialIndex(rhs._materialIndex)
    , _meshName(rhs._meshName)
    , _vertexType(rhs._vertexType)
    , _keepCPUData(rhs._keepCPUData)   
    , _cpuPositions(rhs._cpuPositions)      
    , _cpuIndices(rhs._cpuIndices)          
{
}

HRESULT Mesh::Initialize_Prototype(const string& meshName, uint32 materialIndex, const vector<VTXMESH>& vertices,
    const vector<uint32>& indices, bool keepCPUData)
{
    _meshName = meshName;
    _materialIndex = materialIndex;
    _vertexType = EMeshVertexType::StaticMesh;
    _keepCPUData = keepCPUData;

    return Create_Buffers(vertices, indices);
}

HRESULT Mesh::Initialize_Prototype(const string& meshName, uint32 materialIndex, const vector<VTXANIM>& vertices,
    const vector<uint32>& indices)
{
    _meshName = meshName;
    _materialIndex = materialIndex;
    _vertexType = EMeshVertexType::SkeletalMesh;

    return Create_Buffers(vertices, indices);
}

HRESULT Mesh::Initialize(void* arg)
{
    return VIBuffer::Initialize(arg);
}

HRESULT Mesh::Create_Buffers(const vector<VTXMESH>& vertices, const vector<uint32>& indices)
{
    _numVertexBuffers = 1;
    _numVertices = static_cast<uint32>(vertices.size());
    _vertexStride = sizeof(VTXMESH);
    _numIndices = static_cast<uint32>(indices.size());
    _indexStride = sizeof(uint32);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = _vertexStride * _numVertices;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.StructureByteStride = _vertexStride;

    D3D11_SUBRESOURCE_DATA vertexInitialData{};
    vertexInitialData.pSysMem = vertices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &vertexBufferDesc, &vertexInitialData, _vertexBuffer.GetAddressOf()), E_FAIL);

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = _indexStride * _numIndices;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.StructureByteStride = _indexStride;

    D3D11_SUBRESOURCE_DATA indexInitialData{};
    indexInitialData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &indexBufferDesc, &indexInitialData, _indexBuffer.GetAddressOf()), E_FAIL);

    // CPU 데이터 캐싱 - 레이캐스트용으로 위치 복사
    if (_keepCPUData)
    {
        _cpuPositions.resize(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            _cpuPositions[i] = vertices[i].position;
        }

        _cpuIndices = indices;
    }

    return S_OK;
}

HRESULT Mesh::Create_Buffers(const vector<VTXANIM>& vertices, const vector<uint32>& indices)
{
    _numVertexBuffers = 1;
    _numVertices = static_cast<uint32>(vertices.size());
    _vertexStride = sizeof(VTXANIM);
    _numIndices = static_cast<uint32>(indices.size());
    _indexStride = sizeof(uint32);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = _vertexStride * _numVertices;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.StructureByteStride = _vertexStride;

    D3D11_SUBRESOURCE_DATA vertexInitialData{};
    vertexInitialData.pSysMem = vertices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &vertexBufferDesc, &vertexInitialData, _vertexBuffer.GetAddressOf()), E_FAIL);

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = _indexStride * _numIndices;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.StructureByteStride = _indexStride;

    D3D11_SUBRESOURCE_DATA indexInitialData{};
    indexInitialData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &indexBufferDesc, &indexInitialData, _indexBuffer.GetAddressOf()), E_FAIL);

    if (_keepCPUData)
    {
        _cpuPositions.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
            _cpuPositions[i] = vertices[i].position;

        _cpuIndices = indices;
    }

    return S_OK;
}

Shared<Mesh> Mesh::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const string& meshName,
                          uint32 materialIndex, const vector<VTXMESH>& vertices, const vector<uint32>& indices, bool keepCPUData)
{
    auto instance = make_shared<Mesh>(device, context);

    if (FAILED(instance->Initialize_Prototype(meshName, materialIndex, vertices, indices, keepCPUData)))
    {
        MSG_BOX("Failed to Create : Mesh");
        instance->Free();
        return nullptr;
    }

    return instance;
}

Shared<Mesh> Mesh::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const string& meshName,
    uint32 materialIndex, const vector<VTXANIM>& vertices, const vector<uint32>& indices)
{
    auto instance = make_shared<Mesh>(device, context);

    if (FAILED(instance->Initialize_Prototype(meshName, materialIndex, vertices, indices)))
    {
        MSG_BOX("Failed to Create : Mesh");
        instance->Free();
        return nullptr;
    }

    return instance;
}

Shared<Component> Mesh::Clone(void* arg)
{
    auto clone = make_shared<Mesh>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Mesh");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Mesh::Free()
{
    VIBuffer::Free();

    _cpuPositions.clear();
    _cpuPositions.shrink_to_fit();

    _cpuIndices.clear();
    _cpuIndices.shrink_to_fit();
}

