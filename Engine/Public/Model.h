#pragma once

#include "Component.h"
#include "AnimNotify_Types.h"

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
    void            Set_Animation(uint32 animIndex, bool isLoop);
    void            Set_Animation(const string& animName, bool isLoop);
    void            Set_Animation(const FAnimationClipSetting& clip);

    void            Set_AnimationSequence(
                            const FAnimationClipSetting& startClip,
                            const FAnimationClipSetting& loopClip,
                            const FAnimationClipSetting& endClip);

    void            Request_AnimEnd();
    EAnimPhase      Get_AnimPhase() const { return _animPhase; }

    uint32          Get_AnimationCount() const;
    const string&   Get_AnimationName(uint32 index) const;
    int32           Find_AnimationIndex_ByName(const string& animName);

    bool            Play_Animation(float timeDelta);
    bool            Play_Animation(float timeDelta, bool executeNotifies);

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
    EMeshVertexType Get_ModelType() const { return _modelType; }

    void    Set_AnimationPlayRate(float playRate);
    float   Get_AnimationPlayRate() const { return _animationPlayRate; }

    // 블렌딩용
    void    Set_AnimationBlendDuration(float seconds);
    float   Get_AnimationBlendDuration() { return _animationBlendDuration; }

    bool    Is_CurrentAnimationFinished() const { return _isCurrentAnimationFinished; }
    bool    Is_AnimationSequenceFinished() const { return _isAnimSequenceFinished; }

    float   Get_CurrentTrackPositionTicks() const { return _blendState.active ? _blendState.next.trackPosition : _currentClip.trackPosition; }
    float   Get_CurrentTrackPositionSec() const;

    float   Get_CurrentAnimationDurationTicks() const;
    float   Get_CurrentAnimationDurationSec() const;

    // 재생중인 애니메이션 이름 반환
    const string& Get_CurrentAnimationName() const;
    const string& Get_ModelGuid() const { return _modelGuid; }

    int32   Get_BoneIndex_ByName(const string& boneName) const;
    void    Set_MasterPoseModel(Shared<Model> masterModel);

    // 동적 애니메이션 추가
    void    Add_Animation(Shared<Animation> animation);
    // 애니메이션 여러개 추가
    void    Set_Animations(const vector<Shared<Animation>>& animations);

    // 소켓 가져오기
    const Matrix* Get_SocketBoneMatrixPtr(const string& boneName) const;

    const vector<Matrix>& Get_BoneMatrices() const { return _boneMatrices; }

public: /* 지형타기 */
    void    Set_KeepCPUData(bool keep) { _keepCPUData = keep; }
    bool    Get_KeepCPUData() const    { return _keepCPUData; }

    bool    Raycast(const Ray& ray, float& outDist, Vec3& outHitPoint);

    bool    Raycast(
        const Ray& ray,
        float& outDist,
        Vec3& outHitPoint,
        Vec3& outNormal) const;

    const vector<Shared<Mesh>>& Get_Meshes() const { return _meshes; }

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

    HRESULT Reload_ModelFromGuid(const string& guid, EMeshVertexType modelType);

private:
    void    Apply_AnimationClip(uint32 animIndex, bool isLoop, float playRate);
    bool    Find_AnimationIndex(const string& animName, uint32& outIndex) const;
    bool    Has_AnimationName(const string& animName) const;
    void    Add_Animation_Unique(vector<Shared<Animation>>& targetAnimations, Shared<Animation> animation) const;

    
    void    Reset_AnimationSequenceState();
    void    Complete_AnimationSequence();

private: /* blend */

    // clip 전환 진입점
    bool    Find_BeginAnimationTransition(const FAnimationClipSetting& clip);

    // 첫 clip이거나 블렌드가 필요 없을 떄
    void    Begin_ImmediateClip(const FPlayingClipState& clipState);
    void    Clear_BlendState();

    // 현재 적용 중 포즈 캡처
    void    Capture_CurrentPose(vector<FAnimationLocalPose>& outPose) const;

    // 특정 clip state 기준으로 포즈 샘플링
    bool    Sample_ClipPose(const FPlayingClipState& clipState, vector<FAnimationLocalPose>& outPose) const;

    // 샘플링된 local pose를 실제 bone local transform에 반영
    void    Apply_LocalPoses_ToBones(const vector<FAnimationLocalPose>& poses);

    // bone local이 반영된 뒤 combined/skiing matrix 갱신
    void    Update_BoneMatrices_FromBones();

    void    Sync_LegacyAnimationState();

    // 두 포즈를 SRT 기준으로 블렌딩 진행
    static void Blend_LocalPoses(
        const vector<FAnimationLocalPose>& fromPose,
        const vector<FAnimationLocalPose>& toPose,
        float blendRatio,
        vector<FAnimationLocalPose>& outPose);

    bool    Is_CurrentNotifyClipStillActive(const string& expectedClipName) const;

private:
    bool    Has_StartAnimation() const;
    bool    Has_LoopAnimation() const;
    bool    Has_EndAnimation() const;

    bool    Try_PlayBestSequenceEntry();
    bool    Try_AdvanceSequenceAfterCurrentFinished();

public: /* 노티파이 */
    bool    Ensure_AnimNotifyAssetLoaded();
    void    Invalidate_AnimNotifyAsset();
    const FAnimNotifyClipData* Find_CurrentNotifyClip() const;

    void    Set_EnableNotifies(bool enable) { _enableNotifies = enable; }
    bool    Is_EnableNotifies() const { return _enableNotifies; }

    // 노티파이 스테이트 정리
    void    Stop_AllNotifyStates(bool executeEndCheck);

    void    Stop_NotifyStates_ExceptClip(
        const string& keepClipName,
        bool executeEndCheck);

    // Notify 체크
    void    Update_AnimNotifies(
        const FAnimNotifyClipData& clipData,
        const FAnimNotifyContext& context,
        bool executeNotifies);

    // Notify State 체크
    void    Update_AnimNotifyStates(
        const FAnimNotifyClipData& clipData,
        const FAnimNotifyContext& context,
        bool executeNotifies);

    // 구간 통과 판정
    static bool Is_NotifyTimeInRange(float targetTime, float previouseTime, float currentTime, bool wrapped);
    // 특정 시점이 NotifyState 구간 안인지 판정
    static bool Is_NotifyStateActiveTime(float currentTime, float startTime, float duration);
    // 실행중인 NotifyState가 구간 안인지 판정
    static int32 Find_ActiveNotifyStateIndex(
        const vector<FActiveAnimNotifyState>& activeStates,
        const string& clipName,
        int32 stateIndex);

    float   Get_AnimationLengthSec(uint32 animIndex) const;
    float   Get_AnimationTicksPerSecond(uint32 animIndex) const;

    void    Set_CurrentTrackPositionTicks(float trackPosition);
    void    Sample_CurrentPose();

private:
    const FPlayingClipState& Get_VisibleClipState() const;
    float   Get_ClipDurationTicks(const FPlayingClipState& clipState) const;
    float   Convert_TrackTicks_ToSeconds(
        const FPlayingClipState& clipState,
        float trackTicks) const;

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
    bool                            _isCurrentAnimationFinished = false;

    Shared<Model>                   _masterPoseModel;
    vector<int32>                   _boneRetargetIndices;

private: /* 블렌딩 */

    float                           _animationBlendDuration = 0.15f;

    // 현재/다음 애니메이션 상태
    FPlayingClipState               _currentClip;
    FAnimationBlendState            _blendState;

    // 프레임별 샘플 pose 버퍼
    vector<FAnimationLocalPose>     _currentSamplePose;
    vector<FAnimationLocalPose>     _nextSamplePose;
    vector<FAnimationLocalPose>     _blendedPose;

    // 마지막 적용 pose를 유지하면 시퀀스 종료 후 다음 전환 부드럽게
    vector<FAnimationLocalPose>     _lastAppliedPose;

private: /* 노티파이 */
    bool                            _isAnimNotifyAssetLoaded = false;
    FAnimNotifyAsset                _animNotifyAsset;
    vector<FActiveAnimNotifyState>  _activeNotifyStates;

    bool                            _enableNotifies = true;

private: /* 지형타기 */
    bool _keepCPUData = false;

public:
    static Shared<Model> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
           EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix, bool keepCPUData = false);

    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
