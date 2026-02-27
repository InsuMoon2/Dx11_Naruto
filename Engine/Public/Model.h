#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class Mesh;

class ENGINE_DLL Model : public Component
{
    //GENERATED_COMPONENT(Model, Protocol::COMPONENT_TYPE_MODEL)

public:
    explicit Model(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Model(const Model& rhs);
    virtual ~Model() = default;
    
public:
    virtual HRESULT Initialize_Prototype(const string& modelFilePath);
    virtual HRESULT Initialize(void* arg) override;
    HRESULT Render();

private:
    const aiScene*          _aiScene = { nullptr };
    Assimp::Importer        _importer = { };

private:
    uint32                  _numMeshes = {};
    vector<Shared<Mesh>>    _meshes;

private:
    HRESULT Ready_Meshes();

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const string& modelFilePath);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

    uint32 Get_ComponentID() const override { return Protocol::COMPONENT_TYPE_MODEL; }
};

NS_END
