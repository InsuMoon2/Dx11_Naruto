#include "pch.h"
#include "VIBuffer_Trail.h"


VIBuffer_Trail::VIBuffer_Trail(ComPtr<Device> device, ComPtr<DeviceContext> context)
   : VIBuffer(device, context)
{
}

VIBuffer_Trail::VIBuffer_Trail(const VIBuffer_Trail& rhs)
    : VIBuffer(rhs)
{
}

HRESULT VIBuffer_Trail::Initialize_Prototype()
{
    _numVertexBuffers = 1;
    _vertexStride = sizeof(VTXTEX);
    _maxPoints = 200;
    _numVertices = _maxPoints * 2;

    // Triangle List로
    _numIndices = (_maxPoints - 1) * 6;
    _indexStride = sizeof(uint16);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    D3D11_BUFFER_DESC vertexDesc{};
    vertexDesc.ByteWidth = _vertexStride * _numVertices;
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    CHECK_FAILED(_device->CreateBuffer(&vertexDesc, nullptr, _vertexBuffer.GetAddressOf()), E_FAIL);

    vector<uint16> indices;
    indices.resize(_numIndices);

    for (uint32 i = 0; i < _maxPoints - 1; ++i)
    {
        indices[i * 6 + 0] = i * 2 + 0;
        indices[i * 6 + 1] = i * 2 + 1;
        indices[i * 6 + 2] = i * 2 + 2;

        indices[i * 6 + 3] = i * 2 + 1;
        indices[i * 6 + 4] = i * 2 + 3;
        indices[i * 6 + 5] = i * 2 + 2;
    }

    D3D11_BUFFER_DESC indexDesc{};
    indexDesc.ByteWidth = _indexStride * _numIndices;
    indexDesc.Usage = D3D11_USAGE_DEFAULT;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA indexData{};
    indexData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(&indexDesc, &indexData, _indexBuffer.GetAddressOf()), E_FAIL);

    // 최초에는 그릴 게 없으니 0으로 초기화
    _numIndices = 0;

    return S_OK;
}

HRESULT VIBuffer_Trail::Initialize(void* arg)
{
    _vertexBuffer.Reset();
    _indexBuffer.Reset();

    return Initialize_Prototype();
}

HRESULT VIBuffer_Trail::Update_Trail(const deque<FTrailPoint>& points)
{
    if (points.size() < 2)
    {
        _numIndices = 0; // 점이 2개가 안되면 그리지 않음, top bottom. 그런데 이거 소켓으로 위치세팅할지?
        return S_OK;
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(_context->Map(_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        VTXTEX* vertices = static_cast<VTXTEX*>(mapped.pData);
        uint32 limit = min(static_cast<uint32>(points.size()), _maxPoints);

        for (uint32 i = 0; i < limit; ++i)
        {
            float uRatio = static_cast<float>(i) / (limit - 1); 

            vertices[i * 2 + 0].position = points[i].topPos;
            vertices[i * 2 + 0].texCoord = Vec2(uRatio, 0.f);

            vertices[i * 2 + 1].position = points[i].bottomPos;
            vertices[i * 2 + 1].texCoord = Vec2(uRatio, 1.f);
        }

        _context->Unmap(_vertexBuffer.Get(), 0);

        _numIndices = (limit - 1) * 6;
    }

    return S_OK;
}

Shared<VIBuffer_Trail> VIBuffer_Trail::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<VIBuffer_Trail>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : VIBuffer_Trail");
        return nullptr;
    }

    return instance;
}

Shared<Component> VIBuffer_Trail::Clone(void* arg)
{
    auto clone = make_shared<VIBuffer_Trail>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : VIBuffer_Trail");
        return nullptr;
    }

    return clone;
}

void VIBuffer_Trail::Free()
{
    VIBuffer::Free();
}
