#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

Mesh::Mesh(const Mesh& rhs)
    : VIBuffer(rhs)
{
}

Mesh::~Mesh()
{
}

HRESULT Mesh::Initialize_Prototype(const aiMesh* aiMesh)
{
    _numVertexBuffers = 1;
    _numVertices = aiMesh->mNumVertices;
    _vertexStride = sizeof(VTXMESH);
    _numIndices = aiMesh->mNumFaces * 3;
    _indexStride = 4;
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // Vertex Buffer
    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = _vertexStride * _numVertices;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.StructureByteStride = _vertexStride;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    VTXMESH* vertices = new VTXMESH[_numVertices];

    for (size_t i = 0; i < _numVertices; i++)
    {
        memcpy(&vertices[i].position, &aiMesh->mVertices[i], sizeof(Vec3));
        memcpy(&vertices[i].normal,   &aiMesh->mNormals[i], sizeof(Vec3));
        memcpy(&vertices[i].tangent,  &aiMesh->mTangents[i], sizeof(Vec3));
        memcpy(&vertices[i].texcoord, &aiMesh->mTextureCoords[0][i], sizeof(Vec2));
    }

    D3D11_SUBRESOURCE_DATA vertexInitialData{};
    vertexInitialData.pSysMem = vertices;

    CHECK_FAILED(_device->CreateBuffer(
        &vertexBufferDesc, &vertexInitialData, _vertexBuffer.GetAddressOf()), E_FAIL);

    // Index Buffer
    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = _indexStride * _numIndices;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.StructureByteStride = _indexStride;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;

    uint32 indexCount = {};
    uint32* indices = new uint32[_numIndices];

    for (size_t i = 0; i < aiMesh->mNumFaces; i++)
    {
        aiFace face = aiMesh->mFaces[i];
        indices[indexCount++] = face.mIndices[0];
        indices[indexCount++] = face.mIndices[1];
        indices[indexCount++] = face.mIndices[2];
    }

    D3D11_SUBRESOURCE_DATA indexInitialData{};
    indexInitialData.pSysMem = indices;

    CHECK_FAILED(_device->CreateBuffer(
        &indexBufferDesc, &indexInitialData, _indexBuffer.GetAddressOf()), E_FAIL);

    Safe_Delete_Array(vertices);
    Safe_Delete_Array(indices);

    return S_OK;
}

HRESULT Mesh::Initialize(void* arg)
{
    return VIBuffer::Initialize(arg);
}

Shared<Mesh> Mesh::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const aiMesh* aiMesh)
{
    auto instance = make_shared<Mesh>(device, context);

    if (FAILED(instance->Initialize_Prototype(aiMesh)))
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
}
