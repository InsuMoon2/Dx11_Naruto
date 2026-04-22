#pragma once

#include "Component.h"
#include <DirectXTK/Effects.h>
#include <DirectXTK/PrimitiveBatch.h>
#include <DirectXTK/VertexTypes.h>

NS_BEGIN(Engine)

class Bounding;

class ENGINE_DLL Collider : public Component
{
    GENERATED_COMPONENT(Collider, Protocol::COMPONENT_TYPE_COLLIDER)

public:
    explicit Collider(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Collider(const Collider& rhs);
    virtual ~Collider() = default;

public:
    HRESULT Initialize_Prototype(EShape shape);
    HRESULT Initialize(void* arg) override;

    void    Update_Collider(const Matrix& worldMatrix);
    bool    Intersect(Shared<Collider> target);

    bool    Intersect_WithDepth(Shared<Collider> target, Vec3& outNormal, float& outDepth);

public:
    void    Add_Overlap(Shared<Collider> other) { _overlapSet.insert(other); }
    void    Remove_Overlap(Shared<Collider> other) { _overlapSet.erase(other); }

    bool    Is_Overlapping(Shared<Collider> other) const;
    void    Clear_Overlap() { _overlapSet.clear(); }

    set<Weak<Collider>, owner_less<>> Get_OverlapSet() { return _overlapSet; }


public: /* Getter */
    Shared<Bounding>    Get_Bounding()      const { return _bounding; }
    EShape              Get_Shape()         const { return _shape; }
    Collision_Channel   Get_Channel()       const { return _channel; }

    uint32              Get_OverlapMask()  const { return _overlapMask; }
    uint32              Get_BlockMask()    const { return _blockMask; }

    bool                Get_IsActive()      const { return _isActive; }
    Collision_Preset    Get_CollisionPreset() const { return _preset; }

public: /* Setter */
    void Set_CollisionPreset(Collision_Preset preset);
    void Set_IsActive(bool active);
    void Set_IsColl(bool isColl)    { _isColl = isColl; }

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

#ifdef _DEBUG
    HRESULT Render_Debug();
#endif

private:
    EShape                  _shape   = EShape::AABB;

    Collision_Channel       _channel = Collision_Channel::CHANNEL_NONE;
    uint32                  _overlapMask = 0;
    uint32                  _blockMask = 0;

    Collision_Preset        _preset = Collision_Preset::Custom;

    bool                    _isActive = true;
    bool                    _isColl = false;

    Shared<Bounding>        _bounding;

    set<Weak<Collider>, owner_less<>> _overlapSet;

#ifdef _DEBUG
    /* 각 Collider 인스턴스가 자기 디버그 렌더 자원을 독립적으로 가지도록 초기화할 때 호출한다. */
    HRESULT                 Ready_DebugRenderResources();

    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  _batch;
    Shared<DirectX::BasicEffect>                                   _effect;
    ComPtr<ID3D11InputLayout>                                      _inputLayout;
#endif


public:
    static Shared<Collider> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EShape shape);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
