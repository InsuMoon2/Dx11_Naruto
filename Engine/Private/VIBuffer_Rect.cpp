#include "pch.h"
#include "VIBuffer_Rect.h"
#include "Component_Factory.h"

//REGISTER_COMPONENT_FACTORY(VIBuffer_Rect, Protocol::COMPONENT_TYPE_RECT)

VIBuffer_Rect::VIBuffer_Rect(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

VIBuffer_Rect::VIBuffer_Rect(const VIBuffer_Rect& rhs)
    : VIBuffer(rhs)
{
}

VIBuffer_Rect::~VIBuffer_Rect()
{

}

HRESULT VIBuffer_Rect::Initialize_Prototype()
{
    _numVertexBuffers = 1;
    _numVertices = 4;
    _vertexStride = sizeof(FVertexTex);
    _numIndices = 6;
    _indexStride = sizeof(uint16);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // Vertex Buffer
    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = _vertexStride * _numVertices;
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = 0;
    vertexDesc.MiscFlags = 0;

    vector<FVertexTex> vertices;
    vertices.resize(_numVertices);

    vertices[0].position = Vec3(-0.5f, 0.5f, 0.f);
    vertices[0].texCoord = Vec2(0.f, 0.f);

    vertices[1].position = Vec3(0.5f, 0.5f, 0.f);
    vertices[1].texCoord = Vec2(1.f, 0.f);

    vertices[2].position = Vec3(0.5f, -0.5f, 0.f);
    vertices[2].texCoord = Vec2(1.f, 1.f);

    vertices[3].position = Vec3(-0.5f, -0.5f, 0.f);
    vertices[3].texCoord = Vec2(0.f, 1.f);

    D3D11_SUBRESOURCE_DATA vertexData;
    vertexData.pSysMem = vertices.data();

    CHECK_FAILED(_device->CreateBuffer(&vertexDesc, &vertexData, _vertexBuffer.GetAddressOf()), E_FAIL);

    // Index Buffer
    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = _indexStride * _numIndices;
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDesc.CPUAccessFlags = 0;
    indexDesc.MiscFlags = 0;

    vector<uint16> indices;
    indices.resize(_numIndices);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    indices[3] = 0;
    indices[4] = 2;
    indices[5] = 3;

    D3D11_SUBRESOURCE_DATA indexData;
    indexData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(&indexDesc, &indexData, _indexBuffer.GetAddressOf()), E_FAIL);

    return S_OK;
}

HRESULT VIBuffer_Rect::Initialize(void* pArg)
{

    return S_OK;
}

void VIBuffer_Rect::BeginPlay()
{
    VIBuffer::BeginPlay();


}

Shared<VIBuffer_Rect> VIBuffer_Rect::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<VIBuffer_Rect>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : VIBuffer_Rect");

        return nullptr;
    }

    return instance;
}

Shared<Component> VIBuffer_Rect::Clone(void* pArg)
{
    auto instance = make_shared<VIBuffer_Rect>(*this);

    if (FAILED(instance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : VIBuffer_Rect");

        return nullptr;
    }

    return instance;
}

void VIBuffer_Rect::Free()
{
    VIBuffer::Free();
}
