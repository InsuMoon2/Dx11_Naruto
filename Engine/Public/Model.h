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

    HRESULT         Bind_Material(Shared<Shader> shader, const char* constantName,
                                    uint32 meshIndex, EMaterialTextureSlot slot,
                                    uint32 textureIndex);

    size_t          Get_NumMeshes() const { return _meshes.size(); }

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

    size_t  Get_NumMaterials() const { return _materials.size(); }
    Shared<ModelMaterial> Get_Material(uint32 index) const;

    uint32  Get_MeshMaterialIndex(uint32 index) const;
    string  Get_MeshName(uint32 index);

private:
    // .meshbin 확장자일 때 들어오는 초기화 경로
    HRESULT Initialize_FromMeshBin(const string& modelFilePath);

    // Binary loader가 읽은 raw 데이터를 VTXMESH / Mesh로 바꾸는 단계.
    HRESULT Ready_Meshes_FromBinary(const string& modelFilePath);
    HRESULT Ready_Materials_FromJson(const string& materialFilePath);

    void    Apply_MaterialOverrides(const json& data);
    string  Build_MaterialJsonPath(const string& modelFilePath) const;

private:
    EModelType                      _modelType = { EModelType::END };
    Matrix                          _preLocalTransformMatrix = {};

private:
    uint32                          _numMeshes = {};
    vector<Shared<Mesh>>            _meshes;

    uint32                          _numMaterials = {};
    vector<Shared<ModelMaterial>>   _materials;

    string                          _modelGuid = "";

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
