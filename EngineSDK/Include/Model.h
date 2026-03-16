#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class Mesh;
class ModelMaterial;
class Shader;
class Bone;
class Animation;

class ENGINE_DLL Model : public Component
{
    GENERATED_COMPONENT(Model, Protocol::COMPONENT_TYPE_MODEL)

public:
    explicit Model(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Model(const Model& rhs);
    virtual ~Model() = default;
    
public:
    virtual HRESULT Initialize_Prototype(EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);
    virtual HRESULT Initialize(void* arg) override;
    HRESULT         Render(uint32 meshIndex);

    HRESULT         Bind_Material(Shared<Shader> shader, const char* constantName,
                                    uint32 meshIndex, EMaterialTextureSlot slot,
                                    uint32 textureIndex);

    size_t          Get_NumMeshes() const { return _meshes.size(); }

    void            Set_Animation(uint32 animIndex, bool isLoop);
    void            Set_Animation(const string& animName, bool isLoop);

    int32           Find_AnimationIndex_ByName(const string& animName);

    bool            Play_Animation(float timeDelta);
    HRESULT         Bind_BoneMatrices(Shared<Shader> shader, const char* constantName);

public:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

    size_t  Get_NumMaterials() const { return _materials.size(); }
    Shared<ModelMaterial> Get_Material(uint32 index) const;

    uint32  Get_MeshMaterialIndex(uint32 index) const;
    string  Get_MeshName(uint32 index);

private:
    // .meshbin 확장자일 때 들어오는 초기화 경로
    HRESULT Initialize_FromMeshBin(const string& modelFilePath);

    HRESULT Ready_FromBinary(const string& modelFilePath);

    // Binary loader가 읽은 raw 데이터를 VTXMESH / Mesh로 바꾸는 단계.
    HRESULT Ready_StaticMeshes(const FModelBinaryData& data);
    HRESULT Ready_SkeletalMeshes(const FModelBinaryData& data);

    HRESULT Ready_Bones(const FModelBinaryData& data);
    HRESULT Ready_Animations(const FModelBinaryData& data);


    HRESULT Ready_Materials_FromJson(const string& materialFilePath);

    void    Apply_MaterialOverrides(const json& data);
    string  Build_MaterialJsonPath(const string& modelFilePath) const;

private:
    EMeshVertexType                 _modelType = { EMeshVertexType::END };
    Matrix                          _preLocalTransformMatrix = {};

private:
    uint32                          _numMeshes = {};
    vector<Shared<Mesh>>            _meshes;

    uint32                          _numMaterials = {};
    vector<Shared<ModelMaterial>>   _materials;

    vector<Shared<Bone>>            _bones;
    vector<Shared<Animation>>       _animations;
    vector<Matrix>                  _boneMatrices;
    int32                           _currentAnimationIndex = -1;
    bool                            _isAnimationLoop = false;

    string                          _modelGuid = "";

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
