#pragma once

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
    bool    Build_MeshData();
    bool    Build_MaterialData(const wstring& srcPath);
    bool    Write_MaterialJson(const wstring& outputPath);

    bool    Write_MeshBin(const wstring& outputPath);

private:
    // 변환 전에 실제 모델 타입을 확정
    EConvertModelType   Resolve_ModelType(const wstring& srcPath, EConvertModelType requestedType);

    // 현재 _scene을 보고 Skeletal/Static 판별
    EConvertModelType   Detect_ModelType_FromScene() const;
    bool                Scene_HasBones() const;

    // meshbin.meta를 직접 써서 modelType을 명시적으로 남김
    bool                Write_ModelMeta(const wstring& meshPath, EConvertModelType resolvedType);
    bool                Try_ReadExistingGuid(const wstring& metaPath, string& outGuid) const;
    static string       Generate_Guid_String();


    string  Resolve_TexturePath(const aiMaterial* material, aiTextureType textureType,
                    uint32 textureIndex, const wstring& modelFilePath);

    string  Resolve_ExportTextureSlot(const aiMaterial* material,
        aiTextureType assimpType, uint32 textureIndex) const;

    static string Normalize_TextureSlot(aiTextureType assimpType);

    // aiMaterialProperty의 문자열 payload 읽기
    static string Read_PropertyString(const aiMaterialProperty* prop);

    static string Infer_TextureSlot_FromString(const string& value, const string& fallbackSlot);

    string  Normalize_Path(const string& value) const;
    void    Clear();

private:
    string         EscapeJson(const string& value);
    static string  ToLower_Copy(string value);

private:
    shared_ptr<Assimp::Importer>    _importer;
    const aiScene*                  _scene = nullptr;

    vector<FExportMeshData>         _meshes;
    vector<FExportMaterialData>     _materials;

public:
    static unique_ptr<Converter> Create();

};

NS_END
