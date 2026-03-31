#include "pch.h"
#include "Model.h"

#include "Animation.h"
#include "Bone.h"
#include "Mesh.h"
#include "ModelMaterial.h"
#include "GameInstance.h"
#include "Model_BinaryLoader.h"
#include "Shader.h"

#include "AnimNotify_Serializer.h"  
#include "AnimNotify.h"             
#include "AnimNotifyState.h"        

Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
    _blendState.duration = _animationBlendDuration;
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _modelType(rhs._modelType)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
    , _numMaterials(rhs._numMaterials)
    , _materials(rhs._materials)
    , _currentAnimationIndex(-1)
    , _isAnimationLoop(false)
    , _modelGuid(rhs._modelGuid)
    , _animationPlayRate(1.f)
    , _animationBlendDuration(rhs._animationBlendDuration)
    , _animPhase(EAnimPhase::Start)
    , _startClip{}
    , _loopClip{}
    , _endClip{}
    , _hasAnimSequence(false)
    , _isAnimSequenceFinished(false)
    , _currentClip{}
    , _blendState{}
    , _isCurrentAnimationFinished(false)
    , _isAnimNotifyAssetLoaded(false)
    , _animNotifyAsset{}
    , _activeNotifyStates{}
    , _enableNotifies(true)
{
    _blendState.duration = _animationBlendDuration;

    _bones.reserve(rhs._bones.size());
    for (const auto& bone : rhs._bones)
    {
        if (bone)
            _bones.push_back(bone->Clone());
        else
            _bones.push_back(nullptr);
    }

    _animations.reserve(rhs._animations.size());
    for (const auto& animation : rhs._animations)
    {
        if (animation)
            _animations.push_back(animation->Clone());
        else
            _animations.push_back(nullptr);
    }

    _boneMatrices.assign(_bones.size(), Matrix::Identity);
}

HRESULT Model::Initialize_Prototype(EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    _modelType = type;
    _preLocalTransformMatrix = preLocalTransformMatrix;

    fs::path path(modelFilePath);
    string ext = path.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".meshbin")
    {
        LOG_ERROR("Model runtime only supports .meshbin: {}", modelFilePath);
        return E_FAIL;
    }

    wstring wpath = Utils::ToWString(fs::absolute(modelFilePath).string());
    string guid = GAME->Find_AssetGUID(wpath);
    if (!guid.empty())
        _modelGuid = guid;

    return Initialize_FromMeshBin(modelFilePath);
}

HRESULT Model::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

HRESULT Model::Render(uint32 meshIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    _meshes[meshIndex]->Bind_Resources();
    _meshes[meshIndex]->Render();

    return S_OK;
}

HRESULT Model::Bind_Material(Shared<Shader> shader, const char* constantName, uint32 meshIndex,
    EMaterialTextureSlot slot, uint32 textureIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    uint32 matIdx = _meshes[meshIndex]->Get_MaterialIndex();

    if (matIdx >= _materials.size())
        return E_FAIL;

    return _materials[matIdx]->Bind_Material(shader, constantName, slot, textureIndex);
}

void Model::Set_Animation(uint32 animIndex, bool isLoop)
{
    if (animIndex >= _animations.size())
        return;

    FAnimationClipSetting clip;
    clip.animationName = Get_AnimationName(animIndex);
    clip.loop = isLoop;
    clip.playRate = 1.f;

    Set_Animation(clip);
}

void Model::Set_Animation(const string& animName, bool isLoop)
{
    FAnimationClipSetting clip;
    clip.animationName = animName;
    clip.loop = isLoop;
    clip.playRate = 1.f;

    Set_Animation(clip);
}

void Model::Set_Animation(const FAnimationClipSetting& clip)
{
    Reset_AnimationSequenceState();
    Find_BeginAnimationTransition(clip);
}

void Model::Set_AnimationSequence(
    const FAnimationClipSetting& startClip,
    const FAnimationClipSetting& loopClip,
    const FAnimationClipSetting& endClip)
{
    _startClip = startClip;
    _loopClip = loopClip;
    _endClip = endClip;

    _hasAnimSequence = true;
    _isAnimSequenceFinished = false;

    if (!Try_PlayBestSequenceEntry())
    {
        Complete_AnimationSequence();
    }
}

void Model::Request_AnimEnd()
{
    if (!_hasAnimSequence || _isAnimSequenceFinished)
        return;

    if (_animPhase == EAnimPhase::End)
        return;

    if (!Has_EndAnimation())
    {
        Complete_AnimationSequence();
        return;
    }

    _animPhase = EAnimPhase::End;

    if (!Find_BeginAnimationTransition(_endClip))
    {
        Complete_AnimationSequence();
    }
}

uint32 Model::Get_AnimationCount() const
{
    return static_cast<uint32>(_animations.size());
}

const string& Model::Get_AnimationName(uint32 index) const
{
    static const string empty = "";

    if (index >= _animations.size())
        return empty;

    if (_animations[index] == nullptr)
        return empty;

    return _animations[index]->Get_Name();
}

int32 Model::Find_AnimationIndex_ByName(const string& animName)
{
    for (uint32 i = 0; i < _animations.size(); ++i)
    {
        if (_animations[i] && _animations[i]->Get_Name() == animName)
        {
            return static_cast<int32>(i);
        }
    }

    return -1;
}
// 기존 호환용으로 냅두기
bool Model::Play_Animation(float timeDelta)
{
    return Play_Animation(timeDelta, true);
}

bool Model::Play_Animation(float timeDelta, bool executeNotifies)
{
    executeNotifies = executeNotifies && _enableNotifies;

    if (!_currentClip.Is_Valid())
    {
        Stop_AllNotifyStates(executeNotifies);

        if (!_lastAppliedPose.empty())
        {
            Apply_LocalPoses_ToBones(_lastAppliedPose);
        }

        Update_BoneMatrices_FromBones();

        return false;
    }

    for (auto& bone : _bones)
    {
        if (bone)
            bone->Reset_ToNodeTransform();
    }

    bool currentFinished = false;

    // 이번 프레임 Notify 판정용 track 위치는 내부 샘플링 단위인 tick으로 유지한다.
    const float previousTrackTicks = _currentClip.trackPosition;

    if (_currentClip.animIndex >= 0 &&
        _currentClip.animIndex < static_cast<int32>(_animations.size()) &&
        _animations[_currentClip.animIndex] != nullptr)
    {
        currentFinished = _animations[_currentClip.animIndex]->Advance_TrackPosition(
            timeDelta * _currentClip.playRate,
            _currentClip.loop,
            _currentClip.trackPosition);

        Sample_ClipPose(_currentClip, _currentSamplePose);
    }

    const bool wrapped = (_currentClip.loop && _currentClip.trackPosition < previousTrackTicks);

    if (_blendState.active)
    {
        _isCurrentAnimationFinished = false;

        // 블렌드 target clip도 내부적으로는 tick 단위로 진행한다.
        const float previousNextTrackTicks = _blendState.next.trackPosition;

        if (_blendState.next.animIndex >= 0 &&
            _blendState.next.animIndex < static_cast<int32>(_animations.size()) &&
            _animations[_blendState.next.animIndex] != nullptr)
        {
            _animations[_blendState.next.animIndex]->Advance_TrackPosition(
                timeDelta * _blendState.next.playRate,
                _blendState.next.loop,
                _blendState.next.trackPosition);

            Sample_ClipPose(_blendState.next, _nextSamplePose);
        }

        const bool nextWrapped = (_blendState.next.loop && _blendState.next.trackPosition < previousNextTrackTicks);

        _blendState.elapsed += timeDelta;

        const float blendRatio = (_blendState.duration <= FLT_EPSILON)
            ? 1.f
            : Utils::Min(_blendState.elapsed / _blendState.duration, 1.f);

        Blend_LocalPoses(_blendState.fromPose, _nextSamplePose, blendRatio, _blendedPose);
        Apply_LocalPoses_ToBones(_blendedPose);
        Update_BoneMatrices_FromBones();

        _lastAppliedPose = _blendedPose;

        if (Ensure_AnimNotifyAssetLoaded())
        {
            // next 클립의 애니메이션 이름으로 노티파이 클립 검색
            string nextAnimName;
            if (_blendState.next.animIndex >= 0 &&
                _blendState.next.animIndex < static_cast<int32>(_animations.size()) &&
                _animations[_blendState.next.animIndex])
            {
                nextAnimName = _animations[_blendState.next.animIndex]->Get_Name();
            }

            if (const auto* nextClipData = AnimNotify_Serializer::Find_Clip(_animNotifyAsset, nextAnimName))
            {
                const float prevNextTimeSec =
                    Convert_TrackTicks_ToSeconds(_blendState.next, previousNextTrackTicks);
                const float currentNextTimeSec =
                    Convert_TrackTicks_ToSeconds(_blendState.next, _blendState.next.trackPosition);

                FAnimNotifyContext ctx;
                ctx.owner = Get_Owner().get();
                ctx.model = this;
                ctx.modelGuid = _modelGuid;
                ctx.clipName = nextAnimName;
                ctx.previousTimeSec = prevNextTimeSec;
                ctx.currentTimeSec = currentNextTimeSec;
                ctx.deltaTime = timeDelta;
                ctx.isLooping = _blendState.next.loop;
                ctx.wrapped = nextWrapped;
                ctx.isPreview = !executeNotifies;

                Update_AnimNotifies(*nextClipData, ctx, executeNotifies);
                Update_AnimNotifyStates(*nextClipData, ctx, executeNotifies);
            }
        }

        if (blendRatio >= 1.f)
        {
            string nextClipName;
            if (_blendState.next.animIndex >= 0 &&
                _blendState.next.animIndex < static_cast<int32>(_animations.size()) &&
                _animations[_blendState.next.animIndex])
            {
                nextClipName = _animations[_blendState.next.animIndex]->Get_Name();
            }

            Stop_NotifyStates_ExceptClip(nextClipName, executeNotifies);

            _currentClip = _blendState.next;
            Clear_BlendState();
            Sync_LegacyAnimationState();
        }

        return false;
    }

    Apply_LocalPoses_ToBones(_currentSamplePose);
    Update_BoneMatrices_FromBones();

    _lastAppliedPose = _currentSamplePose;
    Sync_LegacyAnimationState();

    const int32 beforeAnimIndex = _currentClip.animIndex;

    if (Ensure_AnimNotifyAssetLoaded())
    {
        if (const auto* clipData = Find_CurrentNotifyClip())
        {
            const float previousTimeSec =
                Convert_TrackTicks_ToSeconds(_currentClip, previousTrackTicks);
            const float currentTimeSec =
                Convert_TrackTicks_ToSeconds(_currentClip, _currentClip.trackPosition);

            FAnimNotifyContext context;
            context.owner = Get_Owner().get();
            context.model = this;
            context.modelGuid = _modelGuid;
            context.clipName = Get_CurrentAnimationName();
            context.previousTimeSec = previousTimeSec;
            context.currentTimeSec = currentTimeSec;
            context.deltaTime = timeDelta;
            context.isLooping = _currentClip.loop;
            context.wrapped = wrapped;
            context.isPreview = !executeNotifies;

            Update_AnimNotifies(*clipData, context, executeNotifies);
            Update_AnimNotifyStates(*clipData, context, executeNotifies);
        }
    }

    // 콤보 공격 도중에 상태 변경이 일어났다면, 바로 탈출
    if (beforeAnimIndex != _currentClip.animIndex || _blendState.active)
    {
        return false;
    }

    if (!_hasAnimSequence)
    {
        _isCurrentAnimationFinished = currentFinished;
        return currentFinished;
    }

    if (!currentFinished)
        return false;

    if (_animPhase == EAnimPhase::Start)
    {
        if (!Try_AdvanceSequenceAfterCurrentFinished())
            return _isAnimSequenceFinished;

        return false;
    }

    if (_animPhase == EAnimPhase::End)
    {
        _isCurrentAnimationFinished = true;

        Stop_AllNotifyStates(executeNotifies);

        Complete_AnimationSequence();
        return true;
    }

    return false;
}

HRESULT Model::Bind_BoneMatrices(Shared<Shader> shader, const char* constantName)
{
    //if (_masterPoseModel != nullptr && !_boneRetargetIndices.empty())
    //{
    //    // 리타겟팅 된 마스터 뼈 배열 가져오기
    //    const vector<Matrix>& masterMatrices = _masterPoseModel->_boneMatrices;

    //    vector<Matrix> retargetMatrices(_boneMatrices.size(), Matrix::Identity);

    //    for (size_t i = 0; i < _boneMatrices.size(); ++i)
    //    {
    //        int32 masterIndex = _boneRetargetIndices[i];


    //        int32 size = static_cast<int32>(masterMatrices.size());
    //        if (masterIndex != -1 && masterIndex < size)
    //        {
    //            retargetMatrices[i] = masterMatrices[masterIndex];
    //        }
    //        else
    //        {
    //            retargetMatrices[i] = _boneMatrices[i];
    //        }
    //    }
    //    return shader->Bind_RawValue(constantName, retargetMatrices.data(), sizeof(Matrix) * retargetMatrices.size());
    //}

    return shader->Bind_RawValue(constantName, _boneMatrices.data(), sizeof(Matrix) * _boneMatrices.size());
}

HRESULT Model::Ready_Materials_FromJson(const string& materialFilePath)
{
    ifstream file(materialFilePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open material json: {}", materialFilePath);
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (!root.contains("materials") || !root["materials"].is_array())
    {
        LOG_ERROR("Invalid material json: {}", materialFilePath);
        return E_FAIL;
    }

    _materials.clear();

    for (const auto& item : root["materials"])
    {
        Shared<ModelMaterial> material = nullptr;

        string matInstGuid = item.value("material_instance_guid", "");

        if (!matInstGuid.empty())
        {
            wstring matInstPath = GAME->Resolve_AssetPath(matInstGuid);
            if (!matInstPath.empty())
            {
                material = make_shared<ModelMaterial>(_device, _context);

                CHECK_FAILED(material->Initialize_FromMaterialInstance(
                    Utils::ToString(matInstPath)), E_FAIL);

                material->From_Json(item);
            }

            else
            {
                material = ModelMaterial::Create(_device, _context, item, materialFilePath);
            }
        }

        if (material == nullptr)
        {
            material = ModelMaterial::Create(
                _device, _context, item, materialFilePath);
        }

        CHECK_NULL(material, E_FAIL);
        _materials.push_back(material);
    }

    _numMaterials = static_cast<uint32>(_materials.size());

    return S_OK;
}

void Model::Apply_MaterialOverrides(const json& data)
{
    if (!data.contains("materials"))
        return;

    auto& matArray = data["materials"];
    for (size_t i = 0; i < matArray.size() && i < _materials.size(); ++i)
    {
        if (_materials[i] && !matArray[i].empty())
            _materials[i]->From_Json(matArray[i]);
    }

}

string Model::Build_MaterialJsonPath(const string& modelFilePath) const
{
    fs::path path(modelFilePath);
    path.replace_extension(".material.json");

    return path.string();
}

void Model::Apply_AnimationClip(uint32 animIndex, bool isLoop, float playRate)
{
    if (animIndex >= _animations.size() || !_animations[animIndex])
        return;

    _currentAnimationIndex = static_cast<int32>(animIndex);
    _isAnimationLoop = isLoop;
    _animationPlayRate = Utils::Max(playRate, 0.01f);
    _animations[animIndex]->Reset();

    // 클립 전환 시 본 로컬 포즈를 기본 자세로 다시 맞춘다.
    for (auto& bone : _bones)
    {
        if (bone)
            bone->Reset_ToNodeTransform();
    }
}

bool Model::Find_AnimationIndex(const string& animName, uint32& outIndex) const
{
    const int32 index = const_cast<Model*>(this)->Find_AnimationIndex_ByName(animName);
    if (index < 0)
        return false;

    outIndex = static_cast<uint32>(index);
    return true;
}

bool Model::Has_AnimationName(const string& animName) const
{
    if (animName.empty())
        return false;

    for (const auto& animation : _animations)
    {
        if (!animation)
            continue;

        if (animation->Get_Name() == animName)
            return true;
    }

    return false;
}

void Model::Add_Animation_Unique(vector<Shared<Animation>>& targetAnimations, Shared<Animation> animation) const
{
    if (!animation)
        return;

    const string& animName = animation->Get_Name();
    if (animName.empty())
        return;

    for (const auto& existing : targetAnimations)
    {
        if (!existing)
            continue;

        if (existing->Get_Name() == animName)
            return;
    }

    targetAnimations.push_back(animation);
}

void Model::Reset_AnimationSequenceState()
{
    Stop_AllNotifyStates(false);

    _hasAnimSequence = false;
    _isAnimSequenceFinished = false;
    _animPhase = EAnimPhase::Start;
    _startClip = {};
    _loopClip = {};
    _endClip = {};
}

void Model::Complete_AnimationSequence()
{
    Stop_AllNotifyStates(false);

    _hasAnimSequence = false;
    _isAnimSequenceFinished = true;

    _startClip = {};
    _loopClip = {};
    _endClip = {};
}

bool Model::Find_BeginAnimationTransition(const FAnimationClipSetting& clip)
{
    if (clip.animationName.empty())
        return false;

    uint32 animIndex = 0;
    if (!Find_AnimationIndex(clip.animationName, animIndex))
        return false;

    FPlayingClipState targetClip{};
    targetClip.animIndex = static_cast<int32>(animIndex);
    targetClip.loop = clip.loop;
    targetClip.playRate = Utils::Max(clip.playRate, 0.01f);
    targetClip.trackPosition = 0.f;

    _isAnimSequenceFinished = false;

    // 아직 현재 재생중인 애니메이션이 없으면 바로 시작
    if (!_currentClip.Is_Valid())
    {
        Begin_ImmediateClip(targetClip);
        return true;
    }

    // 다음 애니메이션과 cross fade 시작
    Capture_CurrentPose(_blendState.fromPose);
    Sample_ClipPose(targetClip, _blendState.toPose);

    _blendState.active = true;
    _blendState.duration = _animationBlendDuration;
    _blendState.elapsed = 0.f;
    _blendState.next = targetClip;

    _nextSamplePose = _blendState.toPose;
    return true;
}

void Model::Begin_ImmediateClip(const FPlayingClipState& clipState)
{
    _currentClip = clipState;
    _isCurrentAnimationFinished = false;
    Clear_BlendState();
    Sync_LegacyAnimationState();
}

void Model::Clear_BlendState()
{
    _blendState.active = false;
    _blendState.elapsed = 0.f;
    _blendState.duration = _animationBlendDuration;
    _blendState.next = {};

    _blendState.fromPose.clear();
    _blendState.toPose.clear();
}

void Model::Capture_CurrentPose(vector<FAnimationLocalPose>& outPose) const
{
    outPose.clear();
    outPose.resize(_bones.size());

    // 이미 직전 프레임 최종 적용 포즈가 있으면, 그걸 우선해서 사용
    if (_lastAppliedPose.size() == _bones.size())
    {
        outPose = _lastAppliedPose;
        return;
    }

    if (_currentClip.Is_Valid())
    {
        Sample_ClipPose(_currentClip, outPose);
    }
}

bool Model::Sample_ClipPose(const FPlayingClipState& clipState, vector<FAnimationLocalPose>& outPose) const
{
    outPose.clear();
    outPose.resize(_bones.size());

    if (!clipState.Is_Valid())
        return false;

    if (clipState.animIndex < 0 || clipState.animIndex >= static_cast<int32>(_animations.size()))
        return false;

    const auto& animation = _animations[clipState.animIndex];
    if (!animation)
        return false;

    animation->Sample_LocalPoses(clipState.trackPosition, outPose);
    return true;
}

void Model::Apply_LocalPoses_ToBones(const vector<FAnimationLocalPose>& poses)
{
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        if (!_bones[i])
            continue;

        if (i >= poses.size() || !poses[i].valid)
        {
            _bones[i]->Reset_ToNodeTransform();
            continue;
        }

        Matrix local = Matrix::CreateScale(poses[i].scale)
            * Matrix::CreateFromQuaternion(poses[i].rotation)
            * Matrix::CreateTranslation(poses[i].translation);

        _bones[i]->Set_LocalTransform(local);
    }
}

void Model::Update_BoneMatrices_FromBones()
{
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        if (!_bones[i])
            continue;

        if (_masterPoseModel != nullptr && !_boneRetargetIndices.empty())
        {
            int32 masterIndex = _boneRetargetIndices[i];

            if (masterIndex != -1 && masterIndex < static_cast<int32>(_masterPoseModel->_bones.size()))
            {
                if (auto masterBone = _masterPoseModel->_bones[masterIndex])
                {
                    _bones[i]->Set_CombinedTransform(masterBone->Get_CombinedTransform());
                    //_boneMatrices[i] = _bones[i]->Get_SkinningMatrix();
                    _boneMatrices[i] = _masterPoseModel->_boneMatrices[masterIndex];
                    continue;
                }
            }
        }

        const int32 parentIndex = _bones[i]->Get_ParentIndex();
        const Matrix* parentMatrix = (parentIndex >= 0)
            ? &_bones[parentIndex]->Get_CombinedTransform()
            : nullptr;

        _bones[i]->Update_Combined(parentMatrix, _preLocalTransformMatrix);
        _boneMatrices[i] = _bones[i]->Get_SkinningMatrix();
    }
}

void Model::Sync_LegacyAnimationState()
{
    _currentAnimationIndex = _currentClip.animIndex;
    _isAnimationLoop = _currentClip.loop;
}

void Model::Blend_LocalPoses(const vector<FAnimationLocalPose>& fromPose, const vector<FAnimationLocalPose>& toPose,
    float blendRatio, vector<FAnimationLocalPose>& outPose)
{
    const size_t poseCount = Utils::Max(fromPose.size(), toPose.size());

    outPose.clear();
    outPose.resize(poseCount);

    for (size_t i = 0; i < poseCount; i++)
    {
        const bool hasFrom = i < fromPose.size() && fromPose[i].valid;
        const bool hasTo = i < toPose.size() && toPose[i].valid;

        if (hasFrom && hasTo)
        {
            outPose[i].scale = Vec3::Lerp(fromPose[i].scale, toPose[i].scale, blendRatio);
            outPose[i].rotation = Quat::Slerp(fromPose[i].rotation, toPose[i].rotation, blendRatio);
            outPose[i].translation = Vec3::Lerp(fromPose[i].translation, toPose[i].translation, blendRatio);
            outPose[i].valid = true;
            continue;
        }

        if (hasTo)
        {
            outPose[i] = toPose[i];
            outPose[i].valid = true;
            continue;
        }

        if (hasFrom)
        {
            outPose[i] = fromPose[i];
            outPose[i].valid = true;
            continue;
        }

        outPose[i] = {};
    }
}

bool Model::Is_CurrentNotifyClipStillActive(const string& expectedClipName) const
{
    if (expectedClipName.empty())
        return false;

    return Get_CurrentAnimationName() == expectedClipName;
}

bool Model::Has_StartAnimation() const
{
    return !_startClip.animationName.empty();
}

bool Model::Has_LoopAnimation() const
{
    return !_loopClip.animationName.empty();
}

bool Model::Has_EndAnimation() const
{
    return !_endClip.animationName.empty();
}

bool Model::Try_PlayBestSequenceEntry()
{
    if (Has_StartAnimation())
    {
        _animPhase = EAnimPhase::Start;
        return Find_BeginAnimationTransition(_startClip);
    }

    if (Has_LoopAnimation())
    {
        _animPhase = EAnimPhase::Loop;
        return Find_BeginAnimationTransition(_loopClip);
    }

    if (Has_EndAnimation())
    {
        _animPhase = EAnimPhase::End;
        return Find_BeginAnimationTransition(_endClip);
    }

    return false;
}

bool Model::Try_AdvanceSequenceAfterCurrentFinished()
{
    if (_animPhase == EAnimPhase::Start)
    {
        if (Has_LoopAnimation())
        {
            _animPhase = EAnimPhase::Loop;
            return Find_BeginAnimationTransition(_loopClip);
        }

        if (Has_EndAnimation())
        {
            _animPhase = EAnimPhase::End;
            return Find_BeginAnimationTransition(_endClip);
        }

        Complete_AnimationSequence();
        return false;
    }

    if (_animPhase == EAnimPhase::End)
    {
        Complete_AnimationSequence();
        return false;
    }

    return false;
}

bool Model::Ensure_AnimNotifyAssetLoaded()
{
    if (_isAnimNotifyAssetLoaded)
        return true;

    _animNotifyAsset = {};
    _animNotifyAsset.modelGuid = _modelGuid;
    _isAnimNotifyAssetLoaded = true;

    if (_modelGuid.empty())
        return false;

    const fs::path filePath = AnimNotify_Serializer::Get_ModelNotifyFilePath(_modelGuid);
    if (!fs::exists(filePath))
        return false;

    return AnimNotify_Serializer::Load_FromFile(filePath.wstring(), _animNotifyAsset);
}

void Model::Invalidate_AnimNotifyAsset()
{
    // 모델 GUID 변경이나 재로딩할 때 Notify 비우기
    _isAnimNotifyAssetLoaded = false;
    _animNotifyAsset = {};
    _activeNotifyStates.clear();
}

const FAnimNotifyClipData* Model::Find_CurrentNotifyClip() const
{
    if (_modelGuid.empty())
        return nullptr;

    return AnimNotify_Serializer::Find_Clip(_animNotifyAsset, Get_CurrentAnimationName());
}

void Model::Stop_AllNotifyStates(bool executeEndCheck)
{
    // 애니메이션 종료, 애니메이션 전환, 시퀀스 종료 시에는 재생중이더라도 정리가 필요
    // ex) Hit 되거나, 중간에 순간이동하면서 애니메이션이 갑자기 끊길 때
    if (_activeNotifyStates.empty())
        return;

    if (executeEndCheck)
    {
        for (auto& active : _activeNotifyStates)
        {
            FAnimNotifyContext context;
            context.owner = Get_Owner().get();
            context.model = this;
            context.modelGuid = _modelGuid;
            context.clipName = active.clipName;        
            context.currentTimeSec = active.lastTimeSec;

            if (active.notifyState)
                active.notifyState->On_End(context);
        }
    }

    _activeNotifyStates.clear();
}

void Model::Update_AnimNotifies(const FAnimNotifyClipData& clipData, const FAnimNotifyContext& context,
    bool executeNotifies)
{
    // 프리뷰모드에서는 생략하게
    if (!executeNotifies)
        return;

    for (const auto& entry : clipData.notifies)
    {
        if (!entry.notify)
            continue;

        if (!Is_NotifyTimeInRange(entry.timeSec,
            context.previousTimeSec, context.currentTimeSec, context.wrapped))
            continue;

        entry.notify->Execute(context);
    }
}

void Model::Update_AnimNotifyStates(const FAnimNotifyClipData& clipData, const FAnimNotifyContext& context,
    bool executeNotifies)
{
    int size = static_cast<int32>(clipData.notifyStates.size());

    const string expectedClipName = clipData.clipName;

    for (int32 stateIndex = 0; stateIndex < size; ++stateIndex)
    {
        const auto& entry = clipData.notifyStates[stateIndex];

        const float durationSec = max(entry.durationSec, 0.f);
        if (durationSec <= 0.f)
            continue;

        const float startSec = entry.startSec;
        const float endSec = startSec + durationSec;

        const bool crossedStart = Is_NotifyTimeInRange(
            startSec,
            context.previousTimeSec, context.currentTimeSec, context.wrapped);

        const bool isActive = Is_NotifyStateActiveTime(context.currentTimeSec, startSec, durationSec);

        int32 activeIndex = Find_ActiveNotifyStateIndex(_activeNotifyStates, clipData.clipName, stateIndex);

        // 시작지점을 통과했고, 아직 active가 아니면 시작
        if (activeIndex < 0 && crossedStart)
        {
            FActiveAnimNotifyState activeState;
            activeState.clipName = expectedClipName;
            activeState.stateIndex = stateIndex;
            activeState.startSec = startSec;
            activeState.endSec = endSec;
            activeState.lastTimeSec = context.currentTimeSec;
            activeState.notifyState = entry.notifyState;

            _activeNotifyStates.push_back(activeState);
            activeIndex = static_cast<int32>(_activeNotifyStates.size()) - 1;

            if (executeNotifies)
                entry.notifyState->On_Begin(context);

            if (!Is_CurrentNotifyClipStillActive(expectedClipName))
                return;

            activeIndex = Find_ActiveNotifyStateIndex(_activeNotifyStates, expectedClipName, stateIndex);
            if (activeIndex < 0 || activeIndex >= static_cast<int32>(_activeNotifyStates.size()))
                continue;
        }

        if (activeIndex < 0)
            continue;

        _activeNotifyStates[activeIndex].lastTimeSec = context.currentTimeSec;

        if (isActive)
        {
            if (executeNotifies && _activeNotifyStates[activeIndex].notifyState)
                _activeNotifyStates[activeIndex].notifyState->On_Tick(context);

            if (!Is_CurrentNotifyClipStillActive(expectedClipName))
                return;

            continue;
        }

        // State 범위를 벗어나면 End 후 active 제거
        if (executeNotifies && _activeNotifyStates[activeIndex].notifyState)
            _activeNotifyStates[activeIndex].notifyState->On_End(context);

        if (!Is_CurrentNotifyClipStillActive(expectedClipName))
            return;

        activeIndex = Find_ActiveNotifyStateIndex(_activeNotifyStates, expectedClipName, stateIndex);
        if (activeIndex < 0 || activeIndex >= static_cast<int32>(_activeNotifyStates.size()))
            continue;

        _activeNotifyStates.erase(_activeNotifyStates.begin() + activeIndex);
    }

}

bool Model::Is_NotifyTimeInRange(float targetTime, float previouseTime, float currentTime, bool wrapped)
{
    if (!wrapped)
        return targetTime >= previouseTime && targetTime <= currentTime;

    return targetTime >= previouseTime || targetTime <= currentTime;
}

bool Model::Is_NotifyStateActiveTime(float currentTime, float startTime, float duration)
{
    const float clampedDuration = max(duration, 0.f);
    const float endTime = startTime + clampedDuration;

    return currentTime >= startTime && currentTime < endTime;
}

int32 Model::Find_ActiveNotifyStateIndex(const vector<FActiveAnimNotifyState>& activeStates, const string& clipName, int32 stateIndex)
{
    for (int32 i = 0; i < static_cast<int32>(activeStates.size()); ++i)
    {
        if (activeStates[i].clipName == clipName &&
            activeStates[i].stateIndex == stateIndex)
        {
            return i;
        }
          
    }

    return -1;
}

void Model::Stop_NotifyStates_ExceptClip(const string& keepClipName, bool executeEndCheck)
{
    vector<FActiveAnimNotifyState> remained;
    remained.reserve(_activeNotifyStates.size());

    for (auto& active : _activeNotifyStates)
    {
        if (active.clipName == keepClipName)
        {
            remained.push_back(active);
            continue;
        }

        if (executeEndCheck && active.notifyState)
        {
            FAnimNotifyContext context;
            context.owner = Get_Owner().get();
            context.model = this;
            context.modelGuid = _modelGuid;
            context.clipName = active.clipName;
            context.currentTimeSec = active.lastTimeSec;   
            active.notifyState->On_End(context);
        }
    }

    _activeNotifyStates = std::move(remained);
}

float Model::Get_AnimationLengthSec(uint32 animIndex) const
{
    if (animIndex >= _animations.size())
        return 0.f;

    const auto& animation = _animations[animIndex];
    if (!animation)
        return 0.f;

    const float ticksPerSecond = animation->Get_TicksPerSecond();
    if (ticksPerSecond <= FLT_EPSILON)
        return 0.f;

    return animation->Get_Duration() / ticksPerSecond;
}

float Model::Get_AnimationTicksPerSecond(uint32 animIndex) const
{
    if (animIndex >= _animations.size())
        return 0.f;

    const auto& animation = _animations[animIndex];
    if (!animation)
        return 0.f;

    return animation->Get_TicksPerSecond();
}

const FPlayingClipState& Model::Get_VisibleClipState() const
{
    return _blendState.active ? _blendState.next : _currentClip;
}

float Model::Get_ClipDurationTicks(const FPlayingClipState& clipState) const
{
    if (!clipState.Is_Valid())
        return 0.f;

    if (clipState.animIndex < 0 ||
        clipState.animIndex >= static_cast<int32>(_animations.size()))
    {
        return 0.f;
    }

    const auto& animation = _animations[clipState.animIndex];
    if (!animation)
        return 0.f;

    return animation->Get_Duration();
}

void Model::Set_CurrentTrackPositionTicks(float trackPosition)
{
    if (!_currentClip.Is_Valid())
        return;

    if (_currentClip.animIndex < 0 || _currentClip.animIndex >= static_cast<int32>(_animations.size()))
        return;

    const auto& animation = _animations[_currentClip.animIndex];
    if (!animation)
        return;

    const float duration = animation->Get_Duration();
    if (duration <= FLT_EPSILON)
    {
        _currentClip.trackPosition = 0.f;
        return;
    }

    _currentClip.trackPosition = ::clamp(trackPosition, 0.f, duration);
}

void Model::Sample_CurrentPose()
{
    if (!_currentClip.Is_Valid())
        return;

    if (_currentClip.animIndex < 0 || _currentClip.animIndex >= static_cast<int32>(_animations.size()))
        return;

    const auto& animation = _animations[_currentClip.animIndex];
    if (!animation)
        return;

    if (_lastAppliedPose.size() != _bones.size())
        _lastAppliedPose.resize(_bones.size());

    animation->Sample_LocalPoses(_currentClip.trackPosition, _lastAppliedPose);

    for (size_t i = 0; i < _bones.size(); ++i)
    {
        auto& bone = _bones[i];
        if (!bone)
            continue;

        const auto& pose = _lastAppliedPose[i];

        Matrix local =
            Matrix::CreateScale(pose.scale) *
            Matrix::CreateFromQuaternion(pose.rotation) *
            Matrix::CreateTranslation(pose.translation);

        bone->Set_LocalTransform(local);
    }

    for (size_t i = 0; i < _bones.size(); ++i)
    {
        auto& bone = _bones[i];
        if (!bone)
            continue;

        const int32 parentIndex = bone->Get_ParentIndex();

        if (parentIndex >= 0 && parentIndex < static_cast<int32>(_bones.size()) && _bones[parentIndex])
        {
            const Matrix parentCombined = _bones[parentIndex]->Get_CombinedTransform();
            bone->Update_Combined(&parentCombined, Matrix::Identity);
        }
        else
        {
            bone->Update_Combined(nullptr, Matrix::Identity);
        }
    }
}

float Model::Convert_TrackTicks_ToSeconds(const FPlayingClipState& clipState, float trackTicks) const
{
    if (clipState.animIndex < 0 ||
        clipState.animIndex >= static_cast<int32>(_animations.size()))
    {
        return 0.f;
    }

    const auto& animation = _animations[clipState.animIndex];
    if (!animation)
        return 0.f;

    const float ticksPerSecond = animation->Get_TicksPerSecond();
    if (ticksPerSecond <= FLT_EPSILON)
        return 0.f;

    return trackTicks / ticksPerSecond;
}

void Model::Set_AnimationPlayRate(float playRate)
{
    _animationPlayRate = Utils::Max(playRate, 0.01f);

    if (_currentClip.Is_Valid())
    {
        _currentClip.playRate = _animationPlayRate;
        Sync_LegacyAnimationState();
    }

    if (_blendState.active && _blendState.next.Is_Valid())
    {
        _blendState.next.playRate = _animationPlayRate;
    }
}

json Model::To_Json() const
{
    json j = Component::To_Json();

    if (!_modelGuid.empty())
    {
        j["model_guid"] = _modelGuid;
    }

    j["model_type"] = (_modelType == EMeshVertexType::SkeletalMesh) ? "SkeletalMesh" : "StaticMesh";

    if (!_materials.empty())
    {
        json matArray = json::array();
        for (auto& mat : _materials)
        {
            if (mat)
            {
                if (mat)
                    matArray.push_back(mat->To_Json());
                else
                    matArray.push_back(json::object());
            }
        }
        j["materials"] = matArray;
    }

    // 장착된 애니메이션 이름 목록 저장
    if (!_animations.empty())
    {
        json animArray = json::array();

        for (auto& anim : _animations)
        {
            animArray.push_back(anim->Get_Name());
        }
        j["animations"] = animArray;
    }

    return j;
}

void Model::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("model_guid"))
        return;

    const string newGuid = data["model_guid"].get<string>();
    const string newTypeStr = data.value("model_type", "SkeletalMesh");

    const EMeshVertexType newType =
        (newTypeStr == "SkeletalMesh") ? EMeshVertexType::SkeletalMesh : EMeshVertexType::StaticMesh;

    // 같은 GUID/같은 타입이면 실제 리로드는 하지 않고 머티리얼 오버라이드만 다시 적용
    if (newGuid == _modelGuid && newType == _modelType)
    {
        Apply_MaterialOverrides(data);

        if (data.contains("animations"))
        {
            vector<Shared<Animation>> uniqueAnimations;
            uniqueAnimations.reserve(data["animations"].size());

            for (const auto& animName : data["animations"])
            {
                auto anim = GAME->Get_Animation(animName.get<string>());
                Add_Animation_Unique(uniqueAnimations, anim);
            }

            _animations = std::move(uniqueAnimations);
        }

        if (!_animations.empty() && !_currentClip.Is_Valid())
        {
            Set_Animation(0, true);
            Play_Animation(0.f);
        }

        return;
    }

    if (newGuid.empty())
    {
        _modelGuid.clear();
        _modelType = newType;

        Invalidate_AnimNotifyAsset();

        // 기존 모델 데이터를 완전히 비워서 inspector와 runtime 상태가 함께 초기화
        _meshes.clear();
        _materials.clear();
        _bones.clear();
        _animations.clear();
        _boneMatrices.clear();

        _numMeshes = 0;
        _numMaterials = 0;

        _currentAnimationIndex = -1;
        _isAnimationLoop = false;
        _animationPlayRate = 1.f;

        _currentClip = {};
        _blendState = {};

        _hasAnimSequence = false;
        _isAnimSequenceFinished = false;
        _isCurrentAnimationFinished = false;
        _animPhase = EAnimPhase::Start;

        _currentSamplePose.clear();
        _nextSamplePose.clear();
        _blendedPose.clear();
        _lastAppliedPose.clear();

        return;
    }

    if (FAILED(Reload_ModelFromGuid(newGuid, newType)))
    {
        LOG_WARN("Model::From_Json - failed to reload model from guid: {}", newGuid);
        return;
    }

    // 머리티얼 다시 세팅
    Apply_MaterialOverrides(data);

    // 매니저 캐싱에서 애니메이션 가져와서 세팅
    if (data.contains("animations"))
    {
        vector<Shared<Animation>> uniqueAnimations;
        uniqueAnimations.reserve(data["animations"].size());

        for (const auto& animName : data["animations"])
        {
            auto anim = GAME->Get_Animation(animName.get<string>());
            Add_Animation_Unique(uniqueAnimations, anim);
        }

        _animations = std::move(uniqueAnimations);

        if (!_animations.empty() && !_currentClip.Is_Valid())
        {
            Set_Animation(0, true);
            Play_Animation(0.f);
        }
    }
}

HRESULT Model::Reload_ModelFromGuid(const string& guid, EMeshVertexType modelType)
{
    const wstring resolvedPath = GAME->Resolve_AssetPath(guid);
    if (resolvedPath.empty())
    {
        LOG_WARN("Model GUID not found: {}", guid);
        return E_FAIL;
    }

    const string modelFilePath = Utils::ToString(resolvedPath);

    _modelGuid = guid;
    _modelType = modelType;

    Invalidate_AnimNotifyAsset();

    _meshes.clear();
    _materials.clear();
    _bones.clear();
    _animations.clear();
    _boneMatrices.clear();

    _numMeshes = 0;
    _numMaterials = 0;

    _currentAnimationIndex = -1;
    _isAnimationLoop = false;
    _animationPlayRate = 1.f;

    _currentClip = {};
    _blendState = {};

    _hasAnimSequence = false;
    _isAnimSequenceFinished = false;
    _isCurrentAnimationFinished = false;
    _animPhase = EAnimPhase::Start;

    _currentSamplePose.clear();
    _nextSamplePose.clear();
    _blendedPose.clear();
    _lastAppliedPose.clear();

    // 실제 mesh/material/animation 데이터를 다시 읽기
    CHECK_FAILED(Initialize_FromMeshBin(modelFilePath), E_FAIL);

    Sync_LegacyAnimationState();

    return S_OK;
}

Shared<ModelMaterial> Model::Get_Material(uint32 index) const
{
    if (index >= _materials.size())
        return nullptr;

    return _materials[index];
}

uint32 Model::Get_MeshMaterialIndex(uint32 index) const
{
    if (index >= _meshes.size())
        return 0;

    return _meshes[index]->Get_MaterialIndex();
}

string Model::Get_MeshName(uint32 index)
{
    if (index >= _meshes.size())
        return "";

    return _meshes[index]->Get_MeshName();
}

void Model::Set_AnimationBlendDuration(float seconds)
{
    _animationBlendDuration = Utils::Max(seconds, 0.01f);

    if (_blendState.active)
    {
        _blendState.duration = _animationBlendDuration;
    }
}

float Model::Get_CurrentTrackPositionSec() const
{
    const FPlayingClipState& visibleClip = Get_VisibleClipState();
    return Convert_TrackTicks_ToSeconds(visibleClip, visibleClip.trackPosition);
}

float Model::Get_CurrentAnimationDurationTicks() const
{
    return Get_ClipDurationTicks(Get_VisibleClipState());
}

float Model::Get_CurrentAnimationDurationSec() const
{
    return Convert_TrackTicks_ToSeconds(Get_VisibleClipState(), Get_CurrentAnimationDurationTicks());
}

const string& Model::Get_CurrentAnimationName() const
{
    static const string empty = "";

    const FPlayingClipState& targetClip = _blendState.active ? _blendState.next : _currentClip;

    if (!targetClip.Is_Valid())
        return empty;

    if (targetClip.animIndex < 0 || targetClip.animIndex >= static_cast<int32>(_animations.size()))
        return empty;

    return Get_AnimationName(static_cast<uint32>(targetClip.animIndex));
}

int32 Model::Get_BoneIndex_ByName(const string& boneName) const
{
    for (size_t i = 0; i < _bones.size(); ++i)
    {
        if (_bones[i]->Get_Name() == boneName)
        {
            return static_cast<int32>(i);
        }
    }

    return -1;
}

void Model::Set_MasterPoseModel(Shared<Model> masterModel)
{
    _masterPoseModel = masterModel;
    _boneRetargetIndices.clear();

    _boneRetargetIndices.resize(_bones.size(), -1);

    for (size_t i = 0; i < _bones.size(); ++i)
    {
        string retargetBoneName = _bones[i]->Get_Name();

        int32 masterIndex = masterModel->Get_BoneIndex_ByName(retargetBoneName);
        _boneRetargetIndices[i] = masterIndex;
    }
}

void Model::Add_Animation(Shared<Animation> animation)
{
    if (!animation)
        return;

    const string& animName = animation->Get_Name();
    if (animName.empty())
        return;

    // 같은 이름의 AnimBin은 기존 것을 유지하고 새 중복 추가는 무시한다.
    if (Has_AnimationName(animName))
        return;

    _animations.push_back(animation);
}

void Model::Set_Animations(const vector<Shared<Animation>>& animations)
{
    vector<Shared<Animation>> uniqueAnimations;
    uniqueAnimations.reserve(animations.size());

    for (const auto& animation : animations)
    {
        Add_Animation_Unique(uniqueAnimations, animation);
    }

    _animations = std::move(uniqueAnimations);
}

const Matrix* Model::Get_SocketBoneMatrixPtr(const string& boneName) const
{
    int32 index = Get_BoneIndex_ByName(boneName);

    if (index < 0 || index >= static_cast<int32>(_bones.size()))
        return nullptr;

    return &(_bones[index]->Get_CombinedTransform());
}

HRESULT Model::Initialize_FromMeshBin(const string& modelFilePath)
{
    CHECK_FAILED(Ready_FromBinary(modelFilePath), E_FAIL);
    CHECK_FAILED(Ready_Materials_FromJson(Build_MaterialJsonPath(modelFilePath)), E_FAIL);

    return S_OK;
}

HRESULT Model::Ready_FromBinary(const string& modelFilePath)
{
    FModelBinaryData data{};
    if (!Model_BinaryLoader::Load(modelFilePath, data))
        return E_FAIL;

    LOG_INFO("Ready_FromBinary path = {}", modelFilePath);
    LOG_INFO("modelType = {}", static_cast<int>(data.modelType));
    LOG_INFO("meshes = {}", data.meshes.size());
    LOG_INFO("bones = {}", data.bones.size());
    LOG_INFO("animations = {}", data.animations.size());

    _numMaterials = data.materialCount;

    if (data.modelType == EMeshVertexType::StaticMesh)
    {
        return Ready_StaticMeshes(data);
    }

    CHECK_FAILED(Ready_SkeletalMeshes(data), E_FAIL);
    CHECK_FAILED(Ready_Bones(data), E_FAIL);
    CHECK_FAILED(Ready_Animations(data), E_FAIL);

    _boneMatrices.resize(_bones.size(), Matrix::Identity);

    return S_OK;
}

HRESULT Model::Ready_StaticMeshes(const FModelBinaryData& data)
{
    _meshes.clear();
    _numMeshes = static_cast<uint32>(data.meshes.size());

    for (const FMeshBinaryData& srcMesh : data.meshes)
    {
        vector<VTXMESH> vertices;
        vertices.reserve(srcMesh.vertices.size());

        for (const FMeshVertexRaw& raw : srcMesh.vertices)
        {
            VTXMESH vertex{};
            vertex.position = Vec3(raw.px, raw.py, raw.pz);
            vertex.normal = Vec3(raw.nx, raw.ny, raw.nz);
            vertex.tangent = Vec3(raw.tx, raw.ty, raw.tz);
            vertex.texcoord = Vec2(raw.u, raw.v);

            Vec3 pos = vertex.position;
            Vec3 nor = vertex.normal;
            Vec3 tan = vertex.tangent;

            vertex.position = Vec3::Transform(pos, _preLocalTransformMatrix);
            vertex.normal = Vec3::TransformNormal(nor, _preLocalTransformMatrix);
            vertex.tangent = Vec3::TransformNormal(tan, _preLocalTransformMatrix);

            vertices.push_back(vertex);
        }

        Shared<Mesh> mesh = Mesh::Create(_device, _context,
            srcMesh.name, srcMesh.materialIndex, vertices, srcMesh.indices);
        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_SkeletalMeshes(const FModelBinaryData& data)
{
    _meshes.clear();
    _numMeshes = static_cast<uint32>(data.meshes.size());

    LOG_INFO("Ready_SkeletalMeshes meshCount = {}", data.meshes.size());

    for (const auto& srcMesh : data.meshes)
    {
        vector<VTXANIM> vertices;
        vertices.reserve(srcMesh.animVertices.size());

        for (const auto& raw : srcMesh.animVertices)
        {
            VTXANIM vertex{};
            vertex.position = Vec3(raw.px, raw.py, raw.pz);
            vertex.normal = Vec3(raw.nx, raw.ny, raw.nz);
            vertex.tangent = Vec3(raw.tx, raw.ty, raw.tz);
            vertex.texcoord = Vec2(raw.u, raw.v);

            vertex.blendIndex = XMUINT4(
                raw.blendIndex[0],
                raw.blendIndex[1],
                raw.blendIndex[2],
                raw.blendIndex[3]);

            vertex.blendWeight = Vec4(
                raw.blendWeight[0],
                raw.blendWeight[1],
                raw.blendWeight[2],
                raw.blendWeight[3]);

            //vertex.position = Vec3::Transform(vertex.position, _preLocalTransformMatrix);
            //vertex.normal = Vec3::TransformNormal(vertex.normal, _preLocalTransformMatrix);
            //vertex.tangent = Vec3::TransformNormal(vertex.tangent, _preLocalTransformMatrix);

            vertices.push_back(vertex);
        }

        Shared<Mesh> mesh = Mesh::Create(_device, _context,
            srcMesh.name, srcMesh.materialIndex, vertices, srcMesh.indices);

        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_Bones(const FModelBinaryData& data)
{
    _bones.clear();
    _bones.reserve(data.bones.size());

    LOG_INFO("Ready_Bones count = {}", data.bones.size());

    for (const auto& boneRaw : data.bones)
    {
        LOG_INFO("bone = {}, parent = {}", boneRaw.name, boneRaw.parentIndex);

        Shared<Bone> bone = Bone::Create(boneRaw);
        CHECK_NULL(bone, E_FAIL);
        _bones.push_back(bone);
    }

    return S_OK;
}

HRESULT Model::Ready_Animations(const FModelBinaryData& data)
{
    _animations.clear();
    _animations.reserve(data.animations.size());

    for (const auto& animRaw : data.animations)
    {
        Shared<Animation> animation = Animation::Create(animRaw);
        CHECK_NULL(animation, E_FAIL);
        _animations.push_back(animation);
    }

    return S_OK;
}

Shared<Model> Model::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    auto instance = make_shared<Model>(device, context);

    if (FAILED(instance->Initialize_Prototype(type, modelFilePath, preLocalTransformMatrix)))
    {
        MSG_BOX("Failed to Create : Model");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<Component> Model::Clone(void* arg)
{
    auto clone = make_shared<Model>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Model");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Model::Free()
{
    Component::Free();

    _boneMatrices.clear();
    _animations.clear();
    _bones.clear();
    _materials.clear();
    _meshes.clear();
}
