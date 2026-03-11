#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Shader;
class Model;

NS_END

NS_BEGIN(Client)

class StaticMeshActor : public GameObject
{
    GENERATED_BODY(StaticMeshActor)

public:
    struct FStaticMeshDesc : public FGameObjectDesc
    {
        string modelGuid;      // Asset_Manager GUID (경로 대신 GUID로 관리)
    };

public:
    explicit StaticMeshActor(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit StaticMeshActor(const StaticMeshActor& rhs);
    virtual ~StaticMeshActor();

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;
    virtual HRESULT Bind_ShaderResources() override;

    virtual json    To_Json() const override;
    virtual void    From_Json(const json& data) override;

public:
    const string& Get_ModelGuid() const { return _modelGuid; }

private:
    HRESULT Apply_ModelGuid(const string& modelGuid);
    HRESULT Resolve_ModelAsset();
    HRESULT Ensure_ModelReady();

    HRESULT Ready_Components();

private:
    string                  _modelGuid;       // Asset GUID
    string                  _resolvedPath;    // resolve한 실제 경로
    Shared<Shader>          _shaderCom;
    Shared<Model>           _modelCom;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
