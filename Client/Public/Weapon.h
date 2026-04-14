#pragma once

#include "PartObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
class Collider;
NS_END

NS_BEGIN(Client)

class Weapon : public PartObject
{
    GENERATED_BODY(Weapon)

public:
    struct FWeaponDesc : public PartObject::FPartObjectDesc
    {
        const Matrix* socketMatrix = nullptr;
    };

public:
    explicit Weapon(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Weapon(const Weapon& rhs);
    virtual ~Weapon() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

public:
    void    Set_SocketMatrix(const Matrix* matrix) { _socketMatrix = matrix; }
    void    Set_ColliderActive(bool active);

    void    Set_SwordTrailLocalPoints(const Vec3& rootLocal, const Vec3& tipLocal);
    bool    Get_SwordTrailWorldPoints(Vec3& outRootWorld, Vec3& outTipWorld) const;

private:
    HRESULT Ready_Components(const wstring& modelAssetTag);
    HRESULT Bind_ShaderResources();
    HRESULT Bind_Lights();

private:
    Shared<Shader>  _shader;
    Shared<Model>   _model;

    Shared<Collider> _collider;

private:
    const Matrix*  _socketMatrix = nullptr;

    Vec3 _swordTrailRootLocal = Vec3::Zero;
    Vec3 _swordTrailTipLocal = Vec3(0.f, 10.f, 0.f);

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
