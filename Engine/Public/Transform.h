#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL Transform : public Component
{
    GENERATED_COMPONENT(Transform, Protocol::COMPONENT_TYPE_TRANSFORM)

public:
    struct FTransformDesc
    {
        float speedPerSec = {};
        float rotationPerSec = {};

        Vec3 position   = Vec3(0.f, 0.f, 0.f);
        float pitch     = 0.f;
        float yaw       = 0.f;
        float roll      = 0.f;
        Vec3 scale      = Vec3(1.f, 1.f, 1.f);
    };

public:
    Transform(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Transform(const Transform& protoType);
    virtual ~Transform();

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;

    virtual json To_Json() const override;
    virtual void From_Json(const json& data) override;

public: /* Position Local */
    Vec3 Get_LocalPosition() const { return _localPosition; }
    void Set_LocalPosition(const Vec3& position);
    void Set_LocalPosition(float x, float y, float z);
    void Add_LocalOffset(const Vec3& offset);

public: /* Position World */
    Vec3 Get_WorldPosition() const;
    void Set_WorldPosition(const Vec3& worldPos);
    void Set_WorldPosition(float x, float y, float z);
    void Add_WorldOffset(const Vec3& offset);

public: /* Rotation Local */
    Quat Get_LocalRotation() const { return _localRotation; }
    void Set_LocalRotation(const Quat& rotation);
    void Set_LocalRotation(float pitch, float yaw, float roll);
    void Add_LocalRotation(const Quat& deltaRotation);

public: /* Rotation World */
    Quat Get_WorldRotation() const;
    void Set_WorldRotation(const Quat& rotation);
    void Set_WorldRotation(float pitch, float yaw, float roll);
    void Add_WorldRotation(const Quat& deltaRotation);

    /* Euler 각도 (Radian 변환) */
    Vec3 Get_LocalEulerAngles() const;
    void Set_LocalEulerAngles(float pitch, float yaw, float roll);

    /* 축 회전 */
    void Rotate_Axis(const Vec3& axis, float degrees);

public: /* Scale */
    Vec3 Get_LocalScale() const { return _localScale; }
    void Set_LocalScale(const Vec3& scale);
    void Set_LocalScale(float x, float y, float z);
    void Set_LocalScale(float uniformScale);
            
    Vec3 Get_WorldScale() const;

public: /* 방향 벡터 */
    Vec3 Get_WorldForward() const; // -Z
    Vec3 Get_WorldRight() const;
    Vec3 Get_WorldUp() const;

    Vec3 Get_LocalForward() const;
    Vec3 Get_LocalRight() const;
    Vec3 Get_LocalUp() const;

public: /* 이동/회전 */
    void Move_Forward(float timeDelta);
    void Move_Backward(float timeDelta);
    void Move_Right(float timeDelta);
    void Move_Left(float timeDelta);
    void Move_Up(float timeDelta);
    void Move_Down(float timeDelta);

    void Turn(const Vec3& axis, float timeDelta);  

    void LookAt(const Vec3& targetWorldPos);
    void LookAt(const Vec3& targetWorldPos, const Vec3& upVector);

public: /* 계층 */
    void Set_Parent(Shared<Transform> parent);
    void Detach_FromParent();
    Shared<Transform> Get_Parent() const;
    bool Has_Parent() const { return !_parent.expired(); }

    void Add_Child(Shared<Transform> child);
    void Remove_Child(Shared<Transform> child);
    const vector<Weak<Transform>>& Get_Children() const { return _children; }

public: /* Matrix */
    const Matrix& Get_WorldMatrix() const;
    Matrix Get_LocalMatrix() const;

    // 에디터용
    void Set_LocalTransform(const Vec3& position, const Quat& rotation, const Vec3& scale);

private:
    void Mark_Dirty();
    void Update_WorldMatrix() const;

private:
    Shared<Transform> GetSharedThis()
    {
        return static_pointer_cast<Transform>(GetSharedPtr());
    }

private: /* Local */
    Vec3    _localPosition = Vec3::Zero;
    Quat    _localRotation = Quat::Identity;
    Vec3    _localScale    = Vec3::Zero;

private: /* World */
    mutable Matrix  _worldMatrix = Matrix::Identity;
    mutable bool    _isDirty = true;

private:
    Weak<Transform> _parent;
    vector<Weak<Transform>> _children;

    float   _speedPerSec = 0.f;
    float   _rotationPerSec = 0.f;

public:
    static shared_ptr<Transform>  Create(ComPtr<Device> device,ComPtr<DeviceContext> context);
    virtual shared_ptr<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
