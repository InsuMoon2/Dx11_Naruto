#include "pch.h"
#include "VIBuffer_Particle_Point.h"

VIBuffer_Particle_Point::VIBuffer_Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer_Instance(device, context)
{
}

VIBuffer_Particle_Point::VIBuffer_Particle_Point(const VIBuffer_Particle_Point& rhs)
    : VIBuffer_Instance(rhs)
    , _instances(rhs._instances)
    , _initialInstances(rhs._initialInstances)
    , _speeds(rhs._speeds)
    , _directions(rhs._directions)
    , _pivot(rhs._pivot)
    , _isLoop(rhs._isLoop)
    , _moveMode(rhs._moveMode)
{
}

HRESULT VIBuffer_Particle_Point::Initialize_Prototype()
{
    return S_OK;
}

HRESULT VIBuffer_Particle_Point::Initialize(void* arg)
{
    auto* desc = static_cast<FParticlePointDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _numVertexBuffers = 2;
    _vertexStride = sizeof(VTXPOS);
    _instanceStride = sizeof(VTXPARTICLE_INSTANCE);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    _numVertices = 1;
    _numInstances = desc->numInstances;

    _pivot = desc->pivot;
    _isLoop = desc->isLoop;
    _moveMode = desc->moveMode;

    VTXPOS vertices[1]{};
    vertices[0].position = Vec3(0.f, 0.f, 0.f);

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = sizeof(vertices);
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.StructureByteStride = sizeof(VTXPOS);

    D3D11_SUBRESOURCE_DATA vertexData{};
    vertexData.pSysMem = vertices;

    CHECK_FAILED(_device->CreateBuffer(&vertexBufferDesc, &vertexData, &_vertexBuffer), E_FAIL);

    CHECK_FAILED(Build_Instances(*desc), E_FAIL);

    _instanceBufferDesc.ByteWidth = static_cast<UINT>(_instanceStride * _numInstances);
    _instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    _instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    _instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    _instanceBufferDesc.StructureByteStride = _instanceStride;

    D3D11_SUBRESOURCE_DATA instanceData{};
    instanceData.pSysMem = _instances.data();

    CHECK_FAILED(_device->CreateBuffer(&_instanceBufferDesc, &instanceData, &_instanceBuffer), E_FAIL);

    return S_OK;
}

HRESULT VIBuffer_Particle_Point::Build_Instances(const FParticlePointDesc& desc)
{
    _instances.clear();
    _initialInstances.clear();
    _speeds.clear();
    _directions.clear();

    _instances.resize(desc.numInstances);
    _initialInstances.resize(desc.numInstances);
    _speeds.resize(desc.numInstances);
    _directions.resize(desc.numInstances);

    for (uint32 i = 0; i < desc.numInstances; ++i)
    {
        const float scaleX = Utils::RandomRange(desc.scale.x, desc.scale.y);
        const float scaleY = Utils::RandomRange(desc.scale.x, desc.scale.y);
        const float speed = Utils::RandomRange(desc.speed.x, desc.speed.y);
        const float maxLifeTime = Utils::RandomRange(desc.lifeTime.x, desc.lifeTime.y);

        Vec3 offset(
            Utils::RandomRange(-desc.range.x, desc.range.x),
            Utils::RandomRange(-desc.range.y, desc.range.y),
            Utils::RandomRange(-desc.range.z, desc.range.z));

        const Vec3 startPos = desc.center + _pivot + offset;
        const Vec3 direction = (_moveMode == EMoveMode::Spread) ? Utils::RandomDirection() : Vec3(0.f, -1.f, 0.f);

        VTXPARTICLE_INSTANCE instance{};

        const float scale = Utils::RandomRange(desc.scale.x, desc.scale.y);

        instance.right = Vec4(scale, 0.f, 0.f, 0.f);
        instance.up = Vec4(0.f, scale, 0.f, 0.f);
        instance.look = Vec4(0.f, 0.f, scale, 0.f);

        instance.translation = Vec4(startPos.x, startPos.y, startPos.z, 1.f);
        instance.lifetime = Vec2(maxLifeTime, 0.f);

        _instances[i] = instance;
        _initialInstances[i] = instance;
        _speeds[i] = speed;
        _directions[i] = direction;
    }

    return S_OK;
}

HRESULT VIBuffer_Particle_Point::Bind_Resources()
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
    _context->IASetPrimitiveTopology(_primitiveType);

    return S_OK;
}

HRESULT VIBuffer_Particle_Point::Render()
{
    _context->DrawInstanced(1, _numInstances, 0, 0);
    return S_OK;
}

void VIBuffer_Particle_Point::Update_Particles(float timeDelta)
{
    if (_instances.empty())
        return;

    switch (_moveMode)
    {
    case EMoveMode::Drop:
        Update_Drop(timeDelta);
        break;

    case EMoveMode::Spread:
        Update_Spread(timeDelta);
        break;

    case EMoveMode::Static:
        Update_Static(timeDelta);
        break;

    default:
        break;
    }

    Upload_InstanceBuffer();
}

void VIBuffer_Particle_Point::Update_Drop(float timeDelta)
{
    for (uint32 i = 0; i < _numInstances; ++i)
    {
        auto& instance = _instances[i];

        instance.lifetime.y += timeDelta;
        instance.translation.y -= _speeds[i] * timeDelta;

        if (instance.lifetime.y >= instance.lifetime.x)
        {
            if (_isLoop)
            {
                Reset_Instance(i);
            }
        }
    }
}

void VIBuffer_Particle_Point::Update_Spread(float timeDelta)
{
    for (uint32 i = 0; i < _numInstances; ++i)
    {
        auto& instance = _instances[i];

        instance.lifetime.y += timeDelta;

        const Vec3 velocity = _directions[i] * _speeds[i] * timeDelta;
        instance.translation.x += velocity.x;
        instance.translation.y += velocity.y;
        instance.translation.z += velocity.z;

        const float growAmount = timeDelta;
        instance.right.x += growAmount;
        instance.up.y += growAmount;

        if (instance.lifetime.y >= instance.lifetime.x)
        {
            if (_isLoop)
            {
                Reset_Instance(i);
            }
        }
    }
}

void VIBuffer_Particle_Point::Update_Static(float timeDelta)
{
    for (uint32 i = 0; i < _numInstances; ++i)
    {
        auto& instance = _instances[i];

        instance.lifetime.y += timeDelta;

        if (instance.lifetime.y >= instance.lifetime.x)
        {
            if (_isLoop)
            {
                Reset_Instance(i);
            }
        }
    }
}

HRESULT VIBuffer_Particle_Point::Upload_InstanceBuffer()
{
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    CHECK_FAILED(_context->Map(_instanceBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource), E_FAIL);

    memcpy(mappedResource.pData, _instances.data(), sizeof(VTXPARTICLE_INSTANCE) * _instances.size());

    _context->Unmap(_instanceBuffer.Get(), 0);
    return S_OK;
}

void VIBuffer_Particle_Point::Reset_Instance(uint32 index)
{
    if (index >= _instances.size())
        return;

    _instances[index] = _initialInstances[index];
    _instances[index].lifetime.y = 0.f;

    if (_moveMode == EMoveMode::Spread)
    {
        _directions[index] = Utils::RandomDirection();
    }
}

Shared<VIBuffer_Particle_Point> VIBuffer_Particle_Point::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<VIBuffer_Particle_Point>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : VIBuffer_Particle_Point");
        return nullptr;
    }

    return instance;
}

Shared<Component> VIBuffer_Particle_Point::Clone(void* arg)
{
    auto clone = make_shared<VIBuffer_Particle_Point>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : VIBuffer_Particle_Point");
        return nullptr;
    }

    return clone;
}

void VIBuffer_Particle_Point::Free()
{
    VIBuffer_Instance::Free();
}
