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

public:
    // 단일 애니메이션 재생
    void    Set_Animation(uint32 animIndex, bool isLoop);
    void    Set_Animation(const string& animName, bool isLoop);
    void    Set_Animation(const FAnimationClipSetting& clip);


    void Set_AnimationSequence(
        const FAnimationClipSetting& startClip,
        const FAnimationClipSetting& loopClip,
        const FAnimationClipSetting& endClip);

    void            Request_AnimEnd();
    EAnimPhase      Get_AnimPhase() const { return _animPhase; }

    uint32          Get_AnimationCount() const;
    const string&   Get_AnimationName(uint32 index) const;
    int32           Find_AnimationIndex_ByName(const string& animName);

    bool            Play_Animation(float timeDelta);
    HRESULT         Bind_BoneMatrices(Shared<Shader> shader, const char* constantName);

    bool            Has_Animations() const { return !_animations.empty(); }
    bool            Has_ActiveAnimation() const
    {
        return _currentAnimationIndex >= 0 &&
            _currentAnimationIndex < static_cast<int32>(_animations.size());
    }

public:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

    size_t  Get_NumMaterials() const { return _materials.size(); }
    Shared<ModelMaterial> Get_Material(uint32 index) const;

    uint32  Get_MeshMaterialIndex(uint32 index) const;
    string  Get_MeshName(uint32 index);

    void    Set_AnimationPlayRate(float playRate);
    float   Get_AnimationPlayRate() const { return _animationPlayRate; }

    bool    Is_AnimationSequenceFinished() const { return _isAnimSequenceFinished; }

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
    void    Apply_AnimationClip(uint32 animIndex, bool isLoop, float playRate);
    bool    Find_AnimationIndex(const string& animName, uint32& outIndex) const;

    // 단일 클립 재생으로 돌아갈 때 시퀀스 상태를 정리한다.
    void    Reset_AnimationSequenceState();

    // End 클립이 없거나 End까지 끝났을 때 공통 종료 처리
    void    Complete_AnimationSequence();

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

private: /* 애니메이션 재생 관련 */
    float                           _animationPlayRate = 1.f;

    EAnimPhase                      _animPhase = EAnimPhase::Start;

    FAnimationClipSetting           _startClip;
    FAnimationClipSetting           _loopClip;
    FAnimationClipSetting           _endClip;

    bool                            _hasAnimSequence = false;
    bool                            _isAnimSequenceFinished = false;

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
