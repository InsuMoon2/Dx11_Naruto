#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class Mesh;
class ModelMaterial;
class Shader;

class ENGINE_DLL Model : public Component
{
    GENERATED_COMPONENT(Model, Protocol::COMPONENT_TYPE_MODEL)

public:
    explicit Model(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Model(const Model& rhs);
    virtual ~Model() = default;
    
public:
    virtual HRESULT Initialize_Prototype(EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);
    virtual HRESULT Initialize(void* arg) override;
    HRESULT         Render(uint32 meshIndex);

    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName,
                            uint32 meshIndex, aiTextureType materialType,
                            uint32 textureIndex);

    size_t  Get_NumMeshes() const { return _meshes.size(); }

private:
    const aiScene*                  _aiScene = { nullptr };
    Assimp::Importer                _importer = { };

    EModelType                      _modelType = { EModelType::END };
    Matrix                          _preLocalTransformMatrix = {};

private:
    uint32                          _numMeshes = {};
    vector<Shared<Mesh>>            _meshes;

    uint32                          _numMaterials = {};
    vector<Shared<ModelMaterial>>   _materials;

    string _modelGuid = "";

private:
    HRESULT Ready_Meshes();
    HRESULT Ready_Materials(const string& modelFilePath);

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
