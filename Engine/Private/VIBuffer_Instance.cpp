#include "pch.h"
#include "VIBuffer_Instance.h"

VIBuffer_Instance::VIBuffer_Instance(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

VIBuffer_Instance::VIBuffer_Instance(const VIBuffer_Instance& rhs)
    : VIBuffer(rhs)
    , _instanceBuffer(rhs._instanceBuffer)
    , _instanceBufferDesc(rhs._instanceBufferDesc)
    , _instanceStride(rhs._instanceStride)
    , _numInstances(rhs._numInstances)
    , _indexCountPerInstance(rhs._indexCountPerInstance)
{
}

HRESULT VIBuffer_Instance::Initialize_Prototype()
{
    return S_OK;
}

HRESULT VIBuffer_Instance::Initialize(void* arg)
{
    return S_OK;
}

HRESULT VIBuffer_Instance::Bind_Resources()
{
    ID3D11Buffer* vertexBuffers[] =
    {
        _vertexBuffer.Get(),
        _instanceBuffer.Get(),
    };

    uint32 strides[] =
    {
        _vertexStride,
        _instanceStride,
    };

    uint32 offsets[] = { 0, 0 };

    _context->IASetVertexBuffers(0, _numVertexBuffers, vertexBuffers, strides, offsets);
    _context->IASetIndexBuffer(_indexBuffer.Get(), _indexStride == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    _context->IASetPrimitiveTopology(_primitiveType);

    return S_OK;
}

HRESULT VIBuffer_Instance::Render()
{
    _context->DrawIndexedInstanced(_indexCountPerInstance, _numInstances, 0, 0, 0);

    return S_OK;
}

void VIBuffer_Instance::Free()
{
    VIBuffer::Free();
}
