#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Model;
class Shader;
NS_END

NS_BEGIN(Client)

class CollisionProxyActor : public GameObject
{
     GENERATED_BODY(CollisionProxyActor)

public:
     struct FCollisionProxyDesc : FGameObjectDesc
     {
         string modelGuid;
         ECollisionProxyType proxyType = ECollisionProxyType::WorldBlock;
         bool enabled = true;
     };

public:
    explicit CollisionProxyActor(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit CollisionProxyActor(const CollisionProxyActor& rhs);
    virtual ~CollisionProxyActor() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

    virtual json    To_Json() const override;
    virtual void    From_Json(const json& data) override;

public:
    ECollisionProxyType Get_ProxyType() const { return _proxyType; }
    bool Is_Enabled() const { return _enabled; }

    const string& Get_ModelGuid() const { return _modelGuid; }
    Shared<Model> Get_Model() const { return _modelCom; }

private:
    HRESULT Apply_ModelGuid(const string& modelGuid);
    HRESULT Resolve_ModelAsset();
    HRESULT Ensure_ModelReady();
    HRESULT Ready_Components();

private:
    string _modelGuid;
    string _resolvedPath;
    ECollisionProxyType _proxyType = ECollisionProxyType::WorldBlock;
    bool _enabled = true;
    Shared<Model> _modelCom;
    Shared<Shader> _shaderCom;
    
public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
