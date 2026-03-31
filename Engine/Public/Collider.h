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

public:
    void    Add_Overlap(Shared<Collider> other) { _overlapSet.insert(other); }
    void    Remove_Overlap(Shared<Collider> other) { _overlapSet.erase(other); }

    bool    Is_Overlapping(Shared<Collider> other) const;
    void    Clear_Overlap() { _overlapSet.clear(); }

public: /* Getter */
    Shared<Bounding>    Get_Bounding()      const { return _bounding; }
    EShape              Get_Shape()         const { return _shape; }
    Collision_Channel   Get_Channel()       const { return _channel; }
    uint32              Get_CollisionMask() const { return _collisionMask; }
    bool                Get_IsActive()      const { return _isActive; }

public: /* Setter */
    void Set_CollisionPreset(Collision_Preset preset);
    void Set_IsActive(bool active)  { _isActive = active; }
    void Set_IsColl(bool isColl)    { _isColl = isColl; }

#ifdef _DEBUG
    HRESULT Render_Debug();
#endif

private:
    EShape                  _shape = EShape::AABB;
    Collision_Channel       _channel = Collision_Channel::CHANNEL_NONE;
    uint32                  _collisionMask = 0;

    bool                    _isActive = true;
    bool                    _isColl = false;

    Shared<Bounding>        _bounding;

    // 아무 생각없이 Shared로 만드니까, 순환참조터짐 ㅋ
    set<Weak<Collider>, owner_less<>> _overlapSet;

#ifdef _DEBUG
    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _batch;
    Shared<DirectX::BasicEffect>                                   _effect;
    ComPtr<ID3D11InputLayout>                                      _inputLayout;
#endif


public:
    static Shared<Collider> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EShape shape);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
