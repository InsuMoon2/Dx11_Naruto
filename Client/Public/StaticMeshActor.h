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
    // Renderer가 static mesh actor를 정렬 배칭 대상으로 분류할 때 호출한다.
    virtual bool    Is_RenderBatchSortable() const override;
    // 같은 static mesh shader 계열을 묶기 위한 1차 정렬 키를 반환한다.
    virtual uint64  Get_RenderBatchPrimaryKey() const override;
    // 같은 model/material 계열을 연속으로 그리기 위한 2차 정렬 키를 반환한다.
    virtual uint64  Get_RenderBatchSecondaryKey() const override;

public:
    const string& Get_ModelGuid() const { return _modelGuid; }
    Shared<Model> Get_Model() const { return _modelCom; }
    const string& Get_ResolvedPath() const { return _resolvedPath; }

public:
    virtual json    To_Json() const override;
    virtual void    From_Json(const json& data) override;

public:
    void Set_OutlineEnabled(bool enabled) { _isOutlineEnabled = enabled; }
    bool Is_OutlineEnabled() const { return _isOutlineEnabled; }

    void Set_OutlineColor(const Vec4& color) { _outlineColor = color; }
    const Vec4& Get_OutlineColor() const { return _outlineColor; }

    void Set_OutlineThickness(float thickness) { _outlineThickness = thickness; }
    float Get_OutlineThickness() const { return _outlineThickness; }

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
    bool                    _isOutlineEnabled = true;
    Vec4                    _outlineColor = Vec4(0.04f, 0.05f, 0.08f, 1.f);
    float                   _outlineThickness = 0.0035f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
