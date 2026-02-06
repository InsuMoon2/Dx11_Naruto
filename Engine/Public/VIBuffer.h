#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL VIBuffer : public Component
{
public:
    explicit VIBuffer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit VIBuffer(const VIBuffer& rhs);
    virtual ~VIBuffer();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    HRESULT Bind_Resources();
    HRESULT Render();

protected:
    ComPtr<Buffer>              _vertexBuffer;
    ComPtr<Buffer>              _indexBuffer;

protected:
    uint32                      _numVertexBuffers = {};
    uint32                      _vertexStride = {};
    uint32                      _numVertices = {};

    uint32                      _indexStride = {};
    uint32                      _numIndices = {};
    D3D11_PRIMITIVE_TOPOLOGY    _primitiveType = {};

public:
    virtual Shared<Component> Clone(void* pArg) = 0;
    void Free() override;
};

NS_END
