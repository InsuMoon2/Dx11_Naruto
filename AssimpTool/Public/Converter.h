#pragma once

#include <unordered_set>

NS_BEGIN(Assimp)

class Converter
{
public:
    explicit Converter();
    virtual ~Converter() = default;

public:
    bool    Convert(const wstring& srcPath, const wstring& dstBasePath, EConvertModelType modelType);

private:
    bool    Read_AssetFile(const wstring& filePath, EConvertModelType modelType);

    // Static mesh 정점/인덱스 데이터를 export 형식으로 빌드한다.
    bool    Build_StaticMeshData();
    // Scene node 전체를 본 계층으로 수집하고 offset matrix를 덮어쓴다.
    bool    Build_BoneHierarchy();
    // Skeletal mesh 정점/가중치/본 참조 데이터를 export 형식으로 빌드한다.
    bool    Build_SkeletalMeshData();
    // 각 애니메이션 채널을 통합 시간축으로 샘플링하고 sidecar config를 반영한다.
    bool    Build_AnimationData(const wstring& srcPath);

    bool    Build_MaterialData(const wstring& srcPath, const wstring& dstBasePath);
    bool    Write_MaterialJson(const wstring& outputPath);
    bool    Write_MeshBin(const wstring& outputPath);
    bool    Write_AnimBin(const wstring& dstBasePath);

private:
    struct FAnimationClipConfig;

    // 변환 전에 실제 모델 타입을 확정
    EConvertModelType   Resolve_ModelType(const wstring& srcPath, EConvertModelType requestedType);

    // 현재 _scene을 보고 Skeletal/Static 판별
    EConvertModelType   Detect_ModelType_FromScene() const;
    bool                Scene_HasBones() const;

    // meshbin.meta를 직접 써서 modelType을 명시적으로 남김
    bool                Write_ModelMeta(const wstring& meshPath, EConvertModelType resolvedType);
    bool                Try_ReadExistingGuid(const wstring& metaPath, string& outGuid) const;
    static string       Generate_Guid_String();


    string              Resolve_TexturePath(const aiMaterial* material, aiTextureType textureType,
                                uint32 textureIndex, const wstring& modelFilePath,
                                const wstring& dstBasePath);

    string              Resolve_ExportTextureSlot(const aiMaterial* material,
                                aiTextureType assimpType, uint32 textureIndex) const;

    static string       Normalize_TextureSlot(aiTextureType assimpType);

    // aiMaterialProperty의 문자열 payload 읽기
    static string       Read_PropertyString(const aiMaterialProperty* prop);

    static string       Infer_TextureSlot_FromString(const string& value, const string& fallbackSlot);
    filesystem::path    Build_TextureOutputRelativePath(const filesystem::path& sourceTexturePath) const;
    string              Copy_Texture_ToOutput(const filesystem::path& sourceTexturePath,
                                const wstring& dstBasePath) const;

    // 시작 경로의 상위 폴더를 따라 올라가며 targetRelativePath가 존재하는지 찾는다.
    static filesystem::path Find_Path_FromAncestors(const filesystem::path& startPath, const filesystem::path& targetRelativePath);
    // 외부 ExportAssets를 포함해 animation config를 탐색할 루트들을 수집한다.
    vector<filesystem::path> Build_AnimationConfigSearchRoots(const wstring& srcPath) const;
    // config 파일명 기준 인덱스를 1회 빌드해서 clip별 조회 비용을 줄인다.
    void                Ensure_AnimationConfigIndex(const wstring& srcPath);
    // props 파일명 기준 인덱스를 1회 빌드해서 clip별 조회 비용을 줄인다.
    void                Ensure_AnimationPropsIndex(const wstring& srcPath);
    // clip 이름 기준 sidecar .config 경로를 추론한다.
    filesystem::path    Resolve_AnimationConfigPath(const wstring& srcPath, const string& clipName);
    // clip 이름 기준 sidecar .props.txt 경로를 추론한다.
    filesystem::path    Resolve_AnimationPropsPath(const wstring& srcPath, const string& clipName);
    // .config 파일에서 rotation-only / translation 허용 본 목록을 읽어온다.
    bool                Try_LoadAnimationConfig(const filesystem::path& configPath, struct FAnimationClipConfig& outConfig) const;
    // .props.txt 파일에서 실제 트랙이 존재하는 본 인덱스 목록을 읽어온다.
    bool                Try_LoadAnimationProps(const filesystem::path& propsPath, struct FAnimationClipConfig& outConfig) const;
    // clip의 고유 샘플 시각 목록을 position/rotation/scale 전체 키에서 만든다.
    vector<float>       Build_ChannelSampleTimes(const aiNodeAnim* srcChannel) const;
    // 본 node transform에서 기본 local scale/rotation/translation을 꺼낸다.
    void                Get_DefaultLocalPose(int32 boneIndex, Vec3& outScale, Quaternion& outRotation, Vec3& outTranslation) const;
    // 현재 scene 본 구조와 props의 BoneTreeIndex 집합이 같은 스켈레톤을 가리키는지 확인한다.
    bool                Is_AnimationPropsCompatible(const FAnimationClipConfig* config) const;
    // props 기준으로 이 본 채널을 export 대상으로 유지할지 확인한다.
    bool                Should_KeepAnimationChannel(const FAnimationClipConfig* config, int32 boneIndex) const;
    // rotation-only 설정일 때 이 본이 translation 애니메이션을 써도 되는지 확인한다.
    bool                Should_UseAnimatedTranslation(const FAnimationClipConfig* config, const string& boneName) const;
    // 증가하는 시간축에 맞춰 vector key를 보간 샘플링한다.
    Vec3                Sample_VectorKeys(const aiVectorKey* keys, uint32 keyCount, float time, const Vec3& defaultValue, uint32& inOutCursor) const;
    // 증가하는 시간축에 맞춰 quaternion key를 보간 샘플링한다.
    Quaternion          Sample_RotationKeys(const aiQuatKey* keys, uint32 keyCount, float time, const Quaternion& defaultValue, uint32& inOutCursor) const;

    string              Normalize_Path(const string& value) const;
    void                Clear();

public:/* 애니메이션 */
    void    Collect_Bones_DFS(aiNode* node, int32 parentIndex, uint32 depth);
    int32   Find_BoneIndex_ByName(const string& name) const;

    void    Add_BoneInfluence(FMeshVertexAnimBin& vertex, uint32 boneIndex, float weight);
    void    Normalize_BoneWeights(FMeshVertexAnimBin& vertex);

    Matrix  Convert_AssimpMatrix(const aiMatrix4x4& m) const;
    FMatrixBin To_MatrixBin(const Matrix& mat) const;

private:
    string         EscapeJson(const string& value);
    // sidecar config 파싱을 위해 줄 양끝 공백을 제거한다.
    static string  Trim_Copy(const string& value);
    static string  ToLower_Copy(string value);

private:
    struct FAnimationClipConfig
    {
        // 이 clip이 sidecar .config에서 읽혀졌는지 확인하는 플래그다.
        bool                    loaded = false;
        // true면 .props.txt에서 실제 트랙이 존재하는 본 인덱스를 읽어온 상태다.
        bool                    propsLoaded = false;
        // true면 허용 목록에 없는 본은 node transform translation을 유지한다.
        bool                    animRotationOnly = false;
        // translation 애니메이션을 허용하는 본 이름 목록이다.
        unordered_set<string>   translationBoneNames;
        // 이 clip에서 실제 애니메이션 트랙이 존재하는 본 인덱스 목록이다.
        unordered_set<int32>    animatedBoneIndices;
        // props가 참조한 BoneTreeIndex 최대값이다.
        int32                   maxAnimatedBoneIndex = -1;
    };

    shared_ptr<Assimp::Importer>    _importer;
    const aiScene*                  _scene = nullptr;

    EConvertModelType               _resolvedModelType = EConvertModelType::END;

    vector<FExportMeshData>         _meshes;
    vector<FExportMaterialData>     _materials;

    vector<FExportBoneData>         _bones;
    vector<FExportAnimationClip>    _animations;

    unordered_map<string, uint32>   _boneNameToIndex;
    // true면 현재 _animationConfigIndexKey 기준 인덱스를 이미 한 번 구축했다는 뜻이다.
    bool                            _animationConfigIndexBuilt = false;
    // 마지막 srcPath 기준으로 animation config 인덱스를 다시 빌드해야 하는지 판단한다.
    wstring                         _animationConfigIndexKey;
    // clip 이름 소문자 기준으로 발견된 config 파일 경로를 캐시한다.
    unordered_map<string, filesystem::path> _animationConfigIndex;
    // true면 현재 _animationPropsIndexKey 기준 인덱스를 이미 한 번 구축했다는 뜻이다.
    bool                            _animationPropsIndexBuilt = false;
    // 마지막 srcPath 기준으로 animation props 인덱스를 다시 빌드해야 하는지 판단한다.
    wstring                         _animationPropsIndexKey;
    // clip 이름 소문자 기준으로 발견된 props 파일 경로를 캐시한다.
    unordered_map<string, filesystem::path> _animationPropsIndex;

public:
    static unique_ptr<Converter> Create();

};

NS_END
