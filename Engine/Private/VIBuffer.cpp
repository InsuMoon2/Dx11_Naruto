#include "pch.h"
#include "VIBuffer.h"

VIBuffer::VIBuffer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

VIBuffer::VIBuffer(const VIBuffer& rhs)
    : Component(rhs)
    , _vertexBuffer{ rhs._vertexBuffer }
    , _indexBuffer{ rhs._indexBuffer }
    , _numVertexBuffers{ rhs._numVertexBuffers }
    , _vertexStride{ rhs._vertexStride }
    , _numVertices{ rhs._numVertices }
    , _indexStride{ rhs._indexStride }
    , _numIndices{ rhs._numIndices }
    , _primitiveType{ rhs._primitiveType }
{
}

VIBuffer::~VIBuffer()
{
}

HRESULT VIBuffer::Initialize_Prototype()
{

    return S_OK;
}

HRESULT VIBuffer::Initialize(void* arg)
{

    return S_OK;
}

HRESULT VIBuffer::Bind_Resources()
{
    ComPtr<Buffer> vertexBuffers[] =
    {
        _vertexBuffer,
    };

    uint32  vertexStrides[] =
    {
        _vertexStride,
    };

    uint32 offsets[] =
    {
        0
    };


    _context->IASetVertexBuffers(0, _numVertexBuffers, vertexBuffers->GetAddressOf(), vertexStrides, offsets);
    _context->IASetIndexBuffer(_indexBuffer.Get(), 2 == _indexStride ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(_primitiveType);

    return S_OK;
}

HRESULT VIBuffer::Render()
{
    _context->DrawIndexed(_numIndices, 0, 0);

    return S_OK;
}

void VIBuffer::Free()
{
    Component::Free();
}
