#pragma once

#include "ContainerObject.h"
#include "ContainerObject.h"

NS_BEGIN(Engine)

class Texture;
class Shader;
class VIBuffer_Rect;
class Controller;

class ENGINE_DLL Character abstract : public ContainerObject
{
    GENERATED_BODY(Character)

public:
    struct FCharacterDesc : public FContainerObjectDesc
    {
        // Temp
        float moveSpeed = 5.f;
        float turnSpeed = 360.f;
    };

public:
    explicit Character(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Character(const Character& rhs);
    virtual ~Character();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
	void	BeginPlay() override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    virtual HRESULT Bind_Lights();

public:
    virtual void TakeDamage(const FDamageEvent& damageEvent);

    // 피격 처리 후 캐릭터별 반응 -> 예를 들면 몬스터 비헤이비어 상태변화, 플레이어 Hit 처리
    virtual void OnDamaged(const FDamageEvent& damageEvent);
    virtual void OnDead(const FDamageEvent& damageEvent);

protected:
    virtual HRESULT Ready_Components();

protected:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

public:
    void Free() override;
};

NS_END
