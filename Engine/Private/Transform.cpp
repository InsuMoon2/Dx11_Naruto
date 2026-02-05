#include "pch.h"
#include "Transform.h"

Transform::Transform(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
    
}

Transform::Transform(const Transform& protoType)
    : Component(protoType)
    , _localPosition(protoType._localPosition)
    , _localRotation(protoType._localRotation)
    , _localScale(protoType._localScale)
    , _speedPerSec(protoType._speedPerSec)          
    , _rotationPerSec(protoType._rotationPerSec)    
{
    _isDirty = true;
}

Transform::~Transform()
{
    
}

HRESULT Transform::Initialize_Prototype()
{
    return S_OK;
}

HRESULT Transform::Initialize(void* arg)
{
    CHECK_NULL(arg, E_FAIL);

    FTransformDesc* desc = static_cast<FTransformDesc*>(arg);

    _speedPerSec = desc->speedPerSec;
    _rotationPerSec = XMConvertToRadians(desc->rotationPerSec);

    _localPosition = desc->position;
    Set_LocalRotation(desc->pitch, desc->yaw, desc->roll);
    _localScale = desc->scale;

    Mark_Dirty();

    return S_OK;
}

json Transform::To_Json() const
{
    json j = Component::To_Json();

    Vec3 euler = Get_LocalEulerAngles(); // Degree

    j["position"] = { _localPosition.x, _localPosition.y, _localPosition.z };
    j["rotation"] = { euler.x, euler.y, euler.z };
    j["scale"] = { _localScale.x, _localScale.y, _localScale.z };

    return j;
}

void Transform::From_Json(const json& data)
{
    Component::From_Json(data);

    if (data.contains("position"))
    {
        auto pos = data["position"];
        Set_LocalPosition(pos[0], pos[1], pos[2]);
    }

    if (data.contains("rotation"))
    {
        auto rot = data["rotation"];
        Set_LocalRotation(rot[0], rot[1], rot[2]);
    }

    if (data.contains("scale"))
    {
        auto scale = data["scale"];
        Set_LocalScale(scale[0], scale[1], scale[2]);
    }
}

void Transform::Set_LocalPosition(const Vec3& position)
{
    _localPosition = position;

    Mark_Dirty();
}

void Transform::Set_LocalPosition(float x, float y, float z)
{
    Set_LocalPosition(Vec3(x, y, z));
}

void Transform::Add_LocalOffset(const Vec3& offset)
{
    _localPosition += offset;

    Mark_Dirty();
}

Vec3 Transform::Get_WorldPosition() const
{
    return Get_WorldMatrix().Translation(); // _41, _42, _43 가져옴
}

void Transform::Set_WorldPosition(const Vec3& worldPos)
{
    if (auto parent = _parent.lock())
    {
        Matrix parentInverse = parent->Get_WorldMatrix().Invert();  
        Vec3 localPos = Vec3::Transform(worldPos, parentInverse);   // 월드 -> 로컬 역행렬
        Set_LocalPosition(localPos);
    }
    else
    {
        Set_LocalPosition(worldPos);    // 부모가 없다면 World = Local 동일
    }
}

void Transform::Set_WorldPosition(float x, float y, float z)
{
    Set_WorldPosition(Vec3(x, y, z));
}

void Transform::Add_WorldOffset(const Vec3& offset)
{
    Set_WorldPosition(Get_WorldPosition() + offset);
}

void Transform::Set_LocalRotation(const Quat& rotation)
{
    _localRotation = rotation;
    _localRotation.Normalize();

    Mark_Dirty();
}

void Transform::Set_LocalRotation(float pitch, float yaw, float roll)
{
    float pitchRad = XMConvertToRadians(pitch);
    float yawRad = XMConvertToRadians(yaw);
    float rollRad = XMConvertToRadians(roll);

    Quat quat = Quat::CreateFromYawPitchRoll(yawRad, pitchRad, rollRad);
    Set_LocalRotation(quat);
}

void Transform::Add_LocalRotation(const Quat& deltaRotation)
{
    _localRotation = _localRotation * deltaRotation;
    _localRotation.Normalize();

    Mark_Dirty();
}

Quat Transform::Get_WorldRotation() const
{
    Vec3 scale, translation;
    Quat rotation;

    Matrix worldMatrix = Get_WorldMatrix();
    worldMatrix.Decompose(scale, rotation, translation);

    return rotation;
}

void Transform::Set_WorldRotation(const Quat& rotation)
{
    if (auto parent = _parent.lock())
    {
        Quat parentRotInverse = parent->Get_WorldRotation();
        parentRotInverse.Inverse(parentRotInverse);

        Quat localRot = rotation * parentRotInverse;
        Set_LocalRotation(localRot);
    }
    else
    {
        Set_LocalRotation(rotation);
    }
}

void Transform::Set_WorldRotation(float pitch, float yaw, float roll)
{
    float pitchRad = XMConvertToRadians(pitch);
    float yawRad = XMConvertToRadians(yaw);
    float rollRad = XMConvertToRadians(roll);

    Quat quat = Quat::CreateFromYawPitchRoll(yawRad, pitchRad, rollRad);
    Set_WorldRotation(quat);
}

void Transform::Add_WorldRotation(const Quat& deltaRotation)
{
    Quat worldRot = Get_WorldRotation();
    worldRot = worldRot * deltaRotation;

    Set_WorldRotation(worldRot);
}

Vec3 Transform::Get_LocalEulerAngles() const
{
    Vec3 euler = _localRotation.ToEuler();  // Radian

    euler.x = XMConvertToDegrees(euler.x);  // Pitch
    euler.y = XMConvertToDegrees(euler.y);  // Yaw
    euler.z = XMConvertToDegrees(euler.z);  // Roll

    return euler;  // Degree
}

void Transform::Set_LocalEulerAngles(float pitch, float yaw, float roll)
{
    Set_LocalRotation(pitch, yaw, roll);
}

void Transform::Rotate_Axis(const Vec3& axis, float degrees)
{
    float radians = XMConvertToRadians(degrees);
    Quat deltaRot = Quat::CreateFromAxisAngle(axis, radians);

    Add_LocalRotation(deltaRot);
}

void Transform::Set_LocalScale(const Vec3& scale)
{
    _localScale = scale;

    Mark_Dirty();
}

void Transform::Set_LocalScale(float x, float y, float z)
{
    Set_LocalScale(Vec3(x, y, z));
}

void Transform::Set_LocalScale(float uniformScale)
{
    Set_LocalScale(uniformScale, uniformScale, uniformScale);
}

Vec3 Transform::Get_WorldScale() const
{
    Vec3 scale, translation;
    Quat rotation;

    Matrix worldMatrix = Get_WorldMatrix();
    worldMatrix.Decompose(scale, rotation, translation);

    return scale;
}

Vec3 Transform::Get_WorldForward() const
{
    return Get_WorldMatrix().Forward();
}

Vec3 Transform::Get_WorldRight() const
{
    return Get_WorldMatrix().Right();
}

Vec3 Transform::Get_WorldUp() const
{
    return Get_WorldMatrix().Up();
}

Vec3 Transform::Get_LocalForward() const
{
    return Vec3::Transform(Vec3::Forward, _localRotation);
}

Vec3 Transform::Get_LocalRight() const
{
    return Vec3::Transform(Vec3::Right, _localRotation);
}

Vec3 Transform::Get_LocalUp() const
{
    return Vec3::Transform(Vec3{0.f, 1.f, 0.f}, _localRotation);
}

void Transform::Move_Forward(float timeDelta)
{
    Vec3 forward = Get_WorldForward();

    Add_WorldOffset(forward * _speedPerSec * timeDelta);
}

void Transform::Move_Backward(float timeDelta)
{
    Vec3 forward = Get_WorldForward();

    Add_WorldOffset(-forward * _speedPerSec * timeDelta);
}

void Transform::Move_Right(float timeDelta)
{
    Vec3 right = Get_WorldRight();

    Add_WorldOffset(right * _speedPerSec * timeDelta);
}

void Transform::Move_Left(float timeDelta)
{
    Vec3 right = Get_WorldRight();

    Add_WorldOffset(-right * _speedPerSec * timeDelta);
}

void Transform::Move_Up(float timeDelta)
{
    Vec3 up = Get_WorldUp();

    Add_WorldOffset(up * _speedPerSec * timeDelta);
}

void Transform::Move_Down(float timeDelta)
{
    Vec3 up = Get_WorldUp();

    Add_WorldOffset(-up * _speedPerSec * timeDelta);
}

void Transform::Turn(const Vec3& axis, float timeDelta)
{
    float radians = _rotationPerSec * timeDelta;
    Quat deltaRot = Quat::CreateFromAxisAngle(axis, radians);

    Add_LocalRotation(deltaRot);
}

void Transform::LookAt(const Vec3& targetWorldPos)
{
    LookAt(targetWorldPos, Vec3::Up);
}

void Transform::LookAt(const Vec3& targetWorldPos, const Vec3& upVector)
{
    Vec3 currentWorldPos = Get_WorldPosition();

    Vec3 direction = targetWorldPos - currentWorldPos;
    if (direction.LengthSquared() < 0.00001f) // 근사치면 무시
        return;

    direction.Normalize();

    // 회전 행렬 생성
    Matrix lookAtMatrix = Matrix::CreateLookAt(Vec3::Zero, direction, upVector);
    lookAtMatrix = lookAtMatrix.Invert(); // View -> World로 전환

    // Quaternion 추출
    Quat rotation = Quat::CreateFromRotationMatrix(lookAtMatrix);

    Set_WorldRotation(rotation);
}

void Transform::Set_Parent(Shared<Transform> parent)
{
    // 기존 부모에서 분리
    auto oldParent = _parent.lock();

    if (oldParent)
        oldParent->Remove_Child(GetSharedThis());

    // 새 부모에 추가
    _parent = parent;

    if (parent)
        parent->Add_Child(GetSharedThis());

    Mark_Dirty();
}

void Transform::Detach_FromParent()
{
    Set_Parent(nullptr);
}

Shared<Transform> Transform::Get_Parent() const
{
    return _parent.lock();
}

void Transform::Add_Child(Shared<Transform> child)
{
    // 중복 방지
    auto iter = find_if(_children.begin(), _children.end(),
        [&](const Weak<Transform>& weak)
        {
            auto ptr = weak.lock();
            return ptr && ptr == child;
        });

    if (iter == _children.end())
    {
        _children.emplace_back(child);
    }
}

void Transform::Remove_Child(Shared<Transform> child)
{
    auto iter = find_if(_children.begin(), _children.end(),
        [&](const Weak<Transform>& weak)
        {
            auto ptr = weak.lock();
            return ptr && ptr == child;
        });

    if (iter != _children.end())
        _children.erase(iter);
}

const Matrix& Transform::Get_WorldMatrix() const
{
    if (_isDirty)
        Update_WorldMatrix();

    return _worldMatrix;
}

Matrix Transform::Get_LocalMatrix() const
{
    // S * R * T
    return Matrix::CreateScale(_localScale)
        * Matrix::CreateFromQuaternion(_localRotation)
        * Matrix::CreateTranslation(_localPosition);
}

void Transform::Set_LocalTransform(const Vec3& position, const Quat& rotation, const Vec3& scale)
{
    _localPosition = position;
    _localRotation = rotation;
    _localScale = scale;

    Mark_Dirty();
}

void Transform::Mark_Dirty()
{
    _isDirty = true;

    // 자식들도 Dirty 세팅
    for (auto weakChild : _children)
    {
        if (auto child = weakChild.lock())
        {
            child->Mark_Dirty();
        }
    }
}

void Transform::Update_WorldMatrix() const
{
    Matrix localMatrix = Get_LocalMatrix();

    if (auto parent = _parent.lock())
    {
        // Local * ParentWorld = World가 된다
        _worldMatrix = localMatrix * parent->Get_WorldMatrix();
    }
    else
    {
        _worldMatrix = localMatrix;
    }

    _isDirty = false;
}

shared_ptr<Transform> Transform::Create(ComPtr<Device> device,
                                        ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Transform>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Transform");

        return nullptr;
    }

    return instance;
}

shared_ptr<Component> Transform::Clone(void* arg)
{
    auto instance = make_shared<Transform>(*this);

    if (FAILED(instance->Initialize(arg))) {
        MSG_BOX("Failed to Clone Transform");

        return nullptr;
    }

    return instance;
}

void Transform::Free()
{
    if (auto parent = _parent.lock())
    {
        parent->Remove_Child(GetSharedThis());
    }

    _children.clear();
        
    Component::Free();
}
