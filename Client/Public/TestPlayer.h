#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Texture;
class Shader;
class VIBuffer_Rect;
NS_END;

NS_BEGIN(Client)

class CombatStat;

class TestPlayer : public GameObject
{
    GENERATED_BODY(TestPlayer)

public:
    explicit TestPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit TestPlayer(const TestPlayer& rhs);
    virtual ~TestPlayer() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    Shared<Shader>          _shaderCom;
    Shared<Texture>         _textureCom;
    Shared<VIBuffer_Rect>   _bufferCom;

private:
    Shared<CombatStat>      _combatStat;


public:
    static shared_ptr<GameObject>  Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual shared_ptr<GameObject> Clone(void* arg) override;

};

NS_END
