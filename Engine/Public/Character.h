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
    // FDamageEvent의 hitSound 값을 실제 사운드 파일로 해석해서 피격음을 재생한다.
    void Play_HitSoundFromDamageEvent(const FDamageEvent& damageEvent);
    // ComboProfile에서 문자열 파일명으로 들어온 피격음을 우선 재생한다.
    bool Try_PlayDirectHitSoundFile(const FDamageEvent& damageEvent);
    // ComboProfile 등에서 넘긴 hitSound 정수값을 실제 리소스 파일명으로 매핑한다.
    static const wchar_t* Resolve_HitSoundFile(int32 hitSound);

protected:
    void Start_HitColor();
    void Update_HitColor(float timeDelta);
    float Get_HitColorStrength() const;
    HRESULT Bind_HitColor_ShaderParams(const Shared<Shader>& shader) const;

protected:
    Vec4  _hitColor = Vec4(1.f, 1.f, 1.f, 1.f);
    float _hitColorDuration = 0.2f;
    float _hitColorRemainTime = 0.f;
    float _hitColorMaxStrength = 0.8f;

protected:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

public:
    void Free() override;
};

NS_END
