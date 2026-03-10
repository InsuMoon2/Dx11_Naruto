# Assimp Converter 따라치기 가이드 (애니메이션 제외)

## 0. 이 문서의 범위

이 문서는 `Dx11_Naruto` 프로젝트에서 아래 목표만 달성하는 버전이다.

- `FBX -> .meshbin + .material.json` 변환
- 런타임에서 `FBX` 대신 `.meshbin` 로드
- 머티리얼 텍스처는 `.material.json`에서 읽기
- 애니메이션, 본, 가중치, 임베디드 텍스처는 이번 버전에서 제외

즉, 이번 문서는 "가장 먼저 안정적으로 붙일 수 있는 1차 컨버터" 가이드다.

---

## 1. 왜 이 버전부터 하는가

현재 프로젝트는 `Engine::Model`이 런타임에서 FBX를 직접 Assimp로 읽는다.

- 런타임 로딩이 느리다
- 에러가 게임 실행 시점에 터진다
- FBX 구조 변경에 런타임이 직접 흔들린다
- 이후 `GUID`, `.meta`, 에디터 파이프라인과 연결하기 어렵다

그래서 1차 목표는 복잡한 스키닝/애니메이션보다 먼저 아래를 분리하는 것이다.

- 오프라인: AssimpTool이 FBX를 읽고 우리 포맷으로 저장
- 런타임: Engine은 우리 포맷만 읽음

이 단계만 끝내도 얻는 이점이 크다.

- 로딩 시간 단축
- 런타임 Assimp 의존 축소
- 데이터 검증 시점이 앞당겨짐
- 추후 애니메이션 확장 위치가 명확해짐

---

## 2. 이번 버전의 설계 결론

이번 버전은 아래처럼 간다.

1. `AssimpTool`이 FBX를 읽는다.
2. 메시/인덱스/머티리얼 참조만 추출한다.
3. `baseName.meshbin`, `baseName.material.json` 두 파일을 만든다.
4. `Engine::Model`이 `.meshbin`이면 커스텀 로더를 사용한다.
5. 머티리얼 파일 경로는 `same base name` 규칙으로 찾는다.

예시:

- `SK_CHR_Gaara.fbx`
- `SK_CHR_Gaara.meshbin`
- `SK_CHR_Gaara.material.json`

즉, `ModelTable.json`은 `path`만 `.meshbin`으로 바꾸면 된다.

이번 버전에서 일부러 안 하는 것:

- 애니메이션 클립 export/import
- bone hierarchy 저장
- blend weight 저장
- embedded texture 추출
- GUID를 파일명으로 직접 쓰는 방식

---

## 3. 왜 `same base name` 규칙을 쓰는가

이게 가장 단순하고 안전하다.

- `Rock.meshbin`이면 자동으로 `Rock.material.json`을 찾는다
- `ResourceLoader`를 거의 건드리지 않아도 된다
- 디버깅이 쉽다
- 사람이 폴더를 봐도 바로 이해된다

이 방식이면 `ModelTable.json` 변경도 최소다.

기존:

```json
{
  "path": "../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.fbx"
}
```

변경:

```json
{
  "path": "../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.meshbin"
}
```

---

## 4. 추가/수정할 파일 목록

## 4-1. 새로 추가할 파일

- `AssimpTool/Public/ConverterTypes.h`
- `AssimpTool/Public/BinaryWriter.h`
- `Engine/Public/ModelBinaryLoader.h`
- `Engine/Private/ModelBinaryLoader.cpp`

## 4-2. 교체 또는 크게 수정할 파일

- `AssimpTool/Public/Converter.h`
- `AssimpTool/Private/Converter.cpp`
- `AssimpTool/Default/AssimpTool.cpp`
- `Engine/Public/Mesh.h`
- `Engine/Private/Mesh.cpp`
- `Engine/Public/ModelMaterial.h`
- `Engine/Private/ModelMaterial.cpp`
- `Engine/Public/Model.h`
- `Engine/Private/Model.cpp`

메모:

- `Mesh`는 GPU 버퍼 생성 담당이라 `aiMesh` 경로와 `meshbin` 경로를 둘 다 받을 수 있게 확장해야 한다.
- `ModelMaterial`은 기존 `aiMaterial` 초기화 외에 `.material.json` 초기화가 추가된다.
- `Model`은 실제 분기 지점이다. FBX 직접 로드와 `.meshbin` 로드를 여기서 갈라준다.

## 4-3. 데이터 파일 수정

- `Client/Bin/Resources/Data/json/ModelTable.json`

메모:

- 이번 단계에서 `Client` 코드 수정은 최소다.
- `ResourceLoader`는 `path` 문자열을 그대로 `Model::Create(...)`로 넘기므로, `ModelTable.json`의 확장자만 `.meshbin`으로 바꿔도 흐름이 유지된다.

---

## 5. 출력 포맷

이번 버전의 `meshbin`은 단순하게 간다.

파일 구조:

1. 헤더
2. mesh 개수
3. mesh 하나씩
4. 각 mesh는 이름, materialIndex, vertex 배열, index 배열

헤더 구조:

```cpp
struct FMeshFileHeader
{
    uint32 magic;          // 'MESH'
    uint32 version;        // 1
    uint32 meshCount;
    uint32 materialCount;
    uint32 reserved;
};
```

정점 구조:

```cpp
struct FMeshVertexBin
{
    float px, py, pz;
    float nx, ny, nz;
    float tx, ty, tz;
    float u, v;
};
```

이 구조는 현재 엔진의 `VTXMESH`와 의미상 동일하다.

---

## 6. AssimpTool 구현

## 6-1. 새 파일: `AssimpTool/Public/ConverterTypes.h`

아래 파일을 새로 만든다.

```cpp
#pragma once

NS_BEGIN(Assimp)

constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
constexpr uint32 MESHBIN_VERSION = 1;

enum class EConvertModelType : uint32
{
    StaticMesh = 0,
    SkeletalMesh
};

struct FMeshFileHeader
{
    uint32 magic = MESHBIN_MAGIC;
    uint32 version = MESHBIN_VERSION;
    uint32 meshCount = 0;
    uint32 materialCount = 0;
    uint32 reserved = 0;
};

struct FMeshVertexBin
{
    float px = 0.f;
    float py = 0.f;
    float pz = 0.f;

    float nx = 0.f;
    float ny = 1.f;
    float nz = 0.f;

    float tx = 1.f;
    float ty = 0.f;
    float tz = 0.f;

    float u = 0.f;
    float v = 0.f;
};

struct FExportMeshData
{
    string                 name;
    uint32                 materialIndex = 0;
    vector<FMeshVertexBin> vertices;
    vector<uint32>         indices;
};

struct FExportMaterialData
{
    string name;
    string diffusePath;
    string normalPath;
    string specularPath;
};

NS_END
```

## 6-2. 새 파일: `AssimpTool/Public/BinaryWriter.h`

```cpp
#pragma once

#include <fstream>

NS_BEGIN(Assimp)

class BinaryWriter
{
public:
    BinaryWriter() = default;
    ~BinaryWriter()
    {
        Close();
    }

public:
    bool Open(const wstring& filePath)
    {
        filesystem::path path(filePath);
        filesystem::create_directories(path.parent_path());

        _stream.open(path, ios::binary | ios::out | ios::trunc);
        return _stream.is_open();
    }

    void Close()
    {
        if (_stream.is_open())
            _stream.close();
    }

    bool IsOpen() const
    {
        return _stream.is_open();
    }

    template<typename T>
    void Write(const T& value)
    {
        _stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    void WriteBytes(const void* data, size_t size)
    {
        if (size == 0)
            return;

        _stream.write(reinterpret_cast<const char*>(data), static_cast<streamsize>(size));
    }

    void WriteString(const string& value)
    {
        uint32 length = static_cast<uint32>(value.size());
        Write(length);

        if (length > 0)
            WriteBytes(value.data(), length);
    }

private:
    ofstream _stream;
};

NS_END
```

## 6-3. 교체 파일: `AssimpTool/Public/Converter.h`

기존 파일을 아래로 교체한다.

```cpp
#pragma once

#include "ConverterTypes.h"
#include "BinaryWriter.h"

NS_BEGIN(Assimp)

class Converter
{
public:
    explicit Converter();
    virtual ~Converter() = default;

public:
    bool Convert(const wstring& srcPath, const wstring& dstBasePath, EConvertModelType modelType);

private:
    bool ReadAssetFile(const wstring& filePath, EConvertModelType modelType);
    bool BuildMeshData();
    bool BuildMaterialData(const wstring& srcPath);
    bool WriteMeshBin(const wstring& outputPath);
    bool WriteMaterialJson(const wstring& outputPath);

    string ResolveTexturePath(const aiMaterial* material, aiTextureType textureType, const wstring& modelFilePath);
    string NormalizePath(const string& value) const;
    void Clear();

private:
    shared_ptr<Importer>        _importer;
    const aiScene*              _scene = nullptr;
    vector<FExportMeshData>     _meshes;
    vector<FExportMaterialData> _materials;

public:
    static unique_ptr<Converter> Create();
};

NS_END
```

## 6-4. 교체 파일: `AssimpTool/Private/Converter.cpp`

기존 파일을 아래로 교체한다.

```cpp
#include "pch.h"
#include "Converter.h"

#include <fstream>

namespace
{
    string EscapeJson(const string& value)
    {
        string out;
        out.reserve(value.size());

        for (char ch : value)
        {
            switch (ch)
            {
            case '\\\\': out += "\\\\\\\\"; break;
            case '\"': out += "\\\\\""; break;
            case '\\n': out += "\\\\n"; break;
            case '\\r': out += "\\\\r"; break;
            case '\\t': out += "\\\\t"; break;
            default: out += ch; break;
            }
        }

        return out;
    }
}

Converter::Converter()
{
    _importer = make_shared<Importer>();
}

bool Converter::Convert(const wstring& srcPath, const wstring& dstBasePath, EConvertModelType modelType)
{
    Clear();

    if (!ReadAssetFile(srcPath, modelType))
        return false;

    if (!BuildMeshData())
        return false;

    if (!BuildMaterialData(srcPath))
        return false;

    const wstring meshPath = dstBasePath + L".meshbin";
    const wstring materialPath = dstBasePath + L".material.json";

    if (!WriteMeshBin(meshPath))
        return false;

    if (!WriteMaterialJson(materialPath))
        return false;

    LOG_INFO("Convert success: {}", filesystem::path(srcPath).string());
    return true;
}

bool Converter::ReadAssetFile(const wstring& filePath, EConvertModelType modelType)
{
    uint32 flags =
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals;

    if (modelType == EConvertModelType::StaticMesh)
        flags |= aiProcess_PreTransformVertices;

    const string path = filesystem::path(filePath).string();
    _scene = _importer->ReadFile(path, flags);

    if (_scene == nullptr || _scene->mRootNode == nullptr || _scene->mNumMeshes == 0)
    {
        LOG_ERROR("Failed to import: {}", path);
        return false;
    }

    return true;
}

bool Converter::BuildMeshData()
{
    if (_scene == nullptr)
        return false;

    _meshes.clear();
    _meshes.reserve(_scene->mNumMeshes);

    for (uint32 meshIndex = 0; meshIndex < _scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* srcMesh = _scene->mMeshes[meshIndex];
        if (srcMesh == nullptr)
            continue;

        FExportMeshData meshData;
        meshData.name = srcMesh->mName.length > 0 ? srcMesh->mName.C_Str() : ("Mesh_" + to_string(meshIndex));
        meshData.materialIndex = srcMesh->mMaterialIndex;
        meshData.vertices.reserve(srcMesh->mNumVertices);
        meshData.indices.reserve(srcMesh->mNumFaces * 3);

        for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
        {
            FMeshVertexBin vertex;

            vertex.px = srcMesh->mVertices[v].x;
            vertex.py = srcMesh->mVertices[v].y;
            vertex.pz = srcMesh->mVertices[v].z;

            if (srcMesh->HasNormals())
            {
                vertex.nx = srcMesh->mNormals[v].x;
                vertex.ny = srcMesh->mNormals[v].y;
                vertex.nz = srcMesh->mNormals[v].z;
            }

            if (srcMesh->HasTangentsAndBitangents())
            {
                vertex.tx = srcMesh->mTangents[v].x;
                vertex.ty = srcMesh->mTangents[v].y;
                vertex.tz = srcMesh->mTangents[v].z;
            }

            if (srcMesh->HasTextureCoords(0))
            {
                vertex.u = srcMesh->mTextureCoords[0][v].x;
                vertex.v = srcMesh->mTextureCoords[0][v].y;
            }

            meshData.vertices.push_back(vertex);
        }

        for (uint32 f = 0; f < srcMesh->mNumFaces; ++f)
        {
            const aiFace& face = srcMesh->mFaces[f];
            if (face.mNumIndices != 3)
                continue;

            meshData.indices.push_back(face.mIndices[0]);
            meshData.indices.push_back(face.mIndices[1]);
            meshData.indices.push_back(face.mIndices[2]);
        }

        _meshes.push_back(meshData);
    }

    return !_meshes.empty();
}

bool Converter::BuildMaterialData(const wstring& srcPath)
{
    if (_scene == nullptr)
        return false;

    _materials.clear();
    _materials.reserve(_scene->mNumMaterials);

    for (uint32 materialIndex = 0; materialIndex < _scene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* srcMaterial = _scene->mMaterials[materialIndex];
        if (srcMaterial == nullptr)
            continue;

        FExportMaterialData materialData;

        aiString materialName;
        if (srcMaterial->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
            materialData.name = materialName.C_Str();
        else
            materialData.name = "Material_" + to_string(materialIndex);

        materialData.diffusePath = ResolveTexturePath(srcMaterial, aiTextureType_DIFFUSE, srcPath);
        materialData.normalPath = ResolveTexturePath(srcMaterial, aiTextureType_NORMALS, srcPath);
        materialData.specularPath = ResolveTexturePath(srcMaterial, aiTextureType_SPECULAR, srcPath);

        _materials.push_back(materialData);
    }

    return true;
}

bool Converter::WriteMeshBin(const wstring& outputPath)
{
    BinaryWriter writer;
    if (!writer.Open(outputPath))
    {
        LOG_ERROR("Failed to open meshbin output");
        return false;
    }

    FMeshFileHeader header;
    header.meshCount = static_cast<uint32>(_meshes.size());
    header.materialCount = static_cast<uint32>(_materials.size());

    writer.Write(header);

    for (const FExportMeshData& meshData : _meshes)
    {
        writer.WriteString(meshData.name);
        writer.Write(meshData.materialIndex);

        const uint32 vertexCount = static_cast<uint32>(meshData.vertices.size());
        writer.Write(vertexCount);
        writer.WriteBytes(meshData.vertices.data(), sizeof(FMeshVertexBin) * meshData.vertices.size());

        const uint32 indexCount = static_cast<uint32>(meshData.indices.size());
        writer.Write(indexCount);
        writer.WriteBytes(meshData.indices.data(), sizeof(uint32) * meshData.indices.size());
    }

    return true;
}

bool Converter::WriteMaterialJson(const wstring& outputPath)
{
    filesystem::path path(outputPath);
    filesystem::create_directories(path.parent_path());

    ofstream file(path, ios::out | ios::trunc);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open material json output");
        return false;
    }

    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"materials\": [\n";

    for (size_t i = 0; i < _materials.size(); ++i)
    {
        const FExportMaterialData& material = _materials[i];

        file << "    {\n";
        file << "      \"material_name\": \"" << EscapeJson(material.name) << "\",\n";
        file << "      \"textures\": {\n";
        file << "        \"diffuse\": \"" << EscapeJson(material.diffusePath) << "\",\n";
        file << "        \"normal\": \"" << EscapeJson(material.normalPath) << "\",\n";
        file << "        \"specular\": \"" << EscapeJson(material.specularPath) << "\"\n";
        file << "      }\n";
        file << "    }";

        if (i + 1 < _materials.size())
            file << ",";

        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    return true;
}

string Converter::ResolveTexturePath(const aiMaterial* material, aiTextureType textureType, const wstring& modelFilePath)
{
    if (material == nullptr)
        return "";

    if (material->GetTextureCount(textureType) == 0)
        return "";

    aiString texturePath;
    if (material->GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
        return "";

    string raw = texturePath.C_Str();
    if (raw.empty())
        return "";

    if (_scene->GetEmbeddedTexture(raw.c_str()) != nullptr)
    {
        LOG_WARN("Embedded texture is not supported in this v1 converter: {}", raw);
        return "";
    }

    filesystem::path texPath(raw);
    filesystem::path modelDir = filesystem::path(modelFilePath).parent_path();

    if (texPath.is_absolute())
    {
        try
        {
            texPath = filesystem::relative(texPath, modelDir);
        }
        catch (...)
        {
            texPath = texPath.filename();
        }
    }

    return NormalizePath(texPath.string());
}

string Converter::NormalizePath(const string& value) const
{
    string result = value;
    replace(result.begin(), result.end(), '\\', '/');
    return result;
}

void Converter::Clear()
{
    _meshes.clear();
    _materials.clear();

    if (_importer)
        _importer->FreeScene();

    _scene = nullptr;
}

unique_ptr<Converter> Converter::Create()
{
    return make_unique<Converter>();
}
```

## 6-5. 교체 파일: `AssimpTool/Default/AssimpTool.cpp`

이번 버전은 가장 단순한 `하드코딩 smoke test`로 시작한다.
처음에는 이게 낫다.

```cpp
#include "pch.h"
#include "Converter.h"

int main()
{
    cout << "=== AssimpTool ===" << endl;

    auto converter = Converter::Create();

    const bool ok = converter->Convert(
        L"../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.fbx",
        L"../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara",
        Assimp::EConvertModelType::SkeletalMesh);

    if (!ok)
    {
        cout << "Convert Failed" << endl;
        return 1;
    }

    cout << "=== Done ===" << endl;
    return 0;
}
```

처음에는 위 경로 하나만 맞게 바꿔서 테스트하면 된다.

---

## 7. Engine 쪽 구현

## 7-1. 새 파일: `Engine/Public/ModelBinaryLoader.h`

설명 메모:

- 이 파일의 역할은 `meshbin` 파싱 전용이다.
- 여기서는 GPU 버퍼를 만들지 않고, 파일에서 읽은 raw 데이터만 `Model` 쪽으로 넘긴다.
- 즉 `ModelBinaryLoader`는 렌더링 클래스가 아니라 파일 포맷 로더라고 생각하면 된다.

```cpp
#pragma once

#include "Base.h"

NS_BEGIN(Engine)

// 디스크에 저장된 정점 레이아웃이다.
// 런타임에서는 이 값을 읽은 뒤 VTXMESH로 변환해서 Mesh를 만든다.
struct FMeshVertexRaw
{
    float px = 0.f;
    float py = 0.f;
    float pz = 0.f;

    float nx = 0.f;
    float ny = 1.f;
    float nz = 0.f;

    float tx = 1.f;
    float ty = 0.f;
    float tz = 0.f;

    float u = 0.f;
    float v = 0.f;
};

struct FMeshBinaryData
{
    string                  name;
    uint32                  materialIndex = 0;
    vector<FMeshVertexRaw>  vertices;
    vector<uint32>          indices;
};

class ENGINE_DLL ModelBinaryLoader
{
public:
    // 이 로더는 파일 파싱만 담당한다.
    // GPU 버퍼 생성은 여기서 하지 않고 Model / Mesh 단계로 넘긴다.
    static bool Load(const string& filePath, vector<FMeshBinaryData>& outMeshes, uint32& outMaterialCount);
};

NS_END
```

## 7-2. 새 파일: `Engine/Private/ModelBinaryLoader.cpp`

설명 메모:

- 로더 구현부는 `헤더 검증`, `문자열 읽기`, `바이트 배열 읽기`처럼 역할을 나눠 두는 편이 이후 디버깅이 쉽다.
- 특히 여기서 `magic`, `version` 검사를 해 두면 런타임에서 포맷이 어긋났을 때 원인 추적이 빠르다.

```cpp
#include "pch.h"
#include "ModelBinaryLoader.h"

#include <fstream>

namespace
{
    // meshbin 포맷 검증용 상수.
    constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
    constexpr uint32 MESHBIN_VERSION = 1;

    struct FMeshFileHeader
    {
        uint32 magic = 0;
        uint32 version = 0;
        uint32 meshCount = 0;
        uint32 materialCount = 0;
        uint32 reserved = 0;
    };

    template<typename T>
    bool ReadValue(ifstream& file, T& outValue)
    {
        file.read(reinterpret_cast<char*>(&outValue), sizeof(T));
        return file.good();
    }

    bool ReadBytes(ifstream& file, void* dst, size_t size)
    {
        if (size == 0)
            return true;

        file.read(reinterpret_cast<char*>(dst), static_cast<streamsize>(size));
        return file.good();
    }

    bool ReadString(ifstream& file, string& outValue)
    {
        uint32 length = 0;
        if (!ReadValue(file, length))
            return false;

        outValue.clear();
        outValue.resize(length);

        if (length == 0)
            return true;

        return ReadBytes(file, outValue.data(), length);
    }
}

bool ModelBinaryLoader::Load(const string& filePath, vector<FMeshBinaryData>& outMeshes, uint32& outMaterialCount)
{
    // 여기서는 meshbin -> CPU 메모리 데이터까지만 만든다.
    // 실제 렌더링용 Mesh 객체 생성은 Model 쪽에서 처리한다.
    outMeshes.clear();
    outMaterialCount = 0;

    ifstream file(filePath, ios::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open meshbin: {}", filePath);
        return false;
    }

    FMeshFileHeader header;
    if (!ReadValue(file, header))
    {
        LOG_ERROR("Failed to read meshbin header: {}", filePath);
        return false;
    }

    if (header.magic != MESHBIN_MAGIC)
    {
        LOG_ERROR("Invalid meshbin magic: {}", filePath);
        return false;
    }

    if (header.version != MESHBIN_VERSION)
    {
        LOG_ERROR("Unsupported meshbin version: {}", filePath);
        return false;
    }

    outMaterialCount = header.materialCount;
    outMeshes.reserve(header.meshCount);

    for (uint32 meshIndex = 0; meshIndex < header.meshCount; ++meshIndex)
    {
        FMeshBinaryData meshData;

        if (!ReadString(file, meshData.name))
            return false;

        if (!ReadValue(file, meshData.materialIndex))
            return false;

        uint32 vertexCount = 0;
        if (!ReadValue(file, vertexCount))
            return false;

        meshData.vertices.resize(vertexCount);
        if (!ReadBytes(file, meshData.vertices.data(), sizeof(FMeshVertexRaw) * meshData.vertices.size()))
            return false;

        uint32 indexCount = 0;
        if (!ReadValue(file, indexCount))
            return false;

        meshData.indices.resize(indexCount);
        if (!ReadBytes(file, meshData.indices.data(), sizeof(uint32) * meshData.indices.size()))
            return false;

        outMeshes.push_back(meshData);
    }

    return true;
}
```

## 7-3. 교체 파일: `Engine/Public/Mesh.h`

기존 파일을 아래로 교체한다.

설명 메모:

- 기존 `Mesh`는 `aiMesh*`만 받아서 생성한다.
- `.meshbin`을 붙이면 `Assimp` 없이도 같은 `Mesh` 객체를 만들 수 있어야 하므로, `vertices / indices`를 직접 받는 오버로드가 필요하다.
- 이 변경의 목적은 "렌더링 경로 통합"이다. 데이터 입력만 달라지고, 그 뒤의 렌더링은 같은 `Mesh`를 쓰게 만든다.

```cpp
#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class Mesh final : public VIBuffer
{
    GENERATED_COMPONENT(Mesh, Protocol::COMPONENT_TYPE_MESH)

public:
    explicit Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Mesh(const Mesh& rhs);
    virtual ~Mesh();

public:
    // 기존 FBX 런타임 경로.
    HRESULT Initialize_Prototype(const aiMesh* aiMesh, const Matrix& preTransformMatrix);
    // meshbin 런타임 경로.
    HRESULT Initialize_Prototype(const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices);
    HRESULT Initialize(void* arg) override;

    const uint32  Get_MaterialIndex() const { return _materialIndex; }
    const string& Get_MeshName() const { return _meshName; }

private:
    HRESULT Create_Buffers(const vector<VTXMESH>& vertices, const vector<uint32>& indices);

private:
    uint32  _materialIndex = 0;
    string  _meshName;

public:
    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const aiMesh* aiMesh, const Matrix& preTransformMatrix);
    // Binary loader가 읽은 정점/인덱스를 같은 Mesh 타입으로 감싼다.
    static Shared<Mesh> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const string& meshName, uint32 materialIndex,
        const vector<VTXMESH>& vertices, const vector<uint32>& indices);

    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
```

## 7-4. 교체 파일: `Engine/Private/Mesh.cpp`

```cpp
#include "pch.h"
#include "Mesh.h"

Mesh::Mesh(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : VIBuffer(device, context)
{
}

Mesh::Mesh(const Mesh& rhs)
    : VIBuffer(rhs)
    , _materialIndex(rhs._materialIndex)
    , _meshName(rhs._meshName)
{
}

Mesh::~Mesh()
{
}

HRESULT Mesh::Initialize_Prototype(const aiMesh* aiMesh, const Matrix& preTransformMatrix)
{
    vector<VTXMESH> vertices;
    vector<uint32> indices;

    vertices.resize(aiMesh->mNumVertices);
    indices.reserve(aiMesh->mNumFaces * 3);

    for (size_t i = 0; i < aiMesh->mNumVertices; ++i)
    {
        VTXMESH vertex{};

        memcpy(&vertex.position, &aiMesh->mVertices[i], sizeof(Vec3));

        if (aiMesh->HasNormals())
            memcpy(&vertex.normal, &aiMesh->mNormals[i], sizeof(Vec3));
        else
            vertex.normal = Vec3(0.f, 1.f, 0.f);

        if (aiMesh->HasTangentsAndBitangents())
            memcpy(&vertex.tangent, &aiMesh->mTangents[i], sizeof(Vec3));
        else
            vertex.tangent = Vec3(1.f, 0.f, 0.f);

        if (aiMesh->HasTextureCoords(0))
            memcpy(&vertex.texcoord, &aiMesh->mTextureCoords[0][i], sizeof(Vec2));
        else
            vertex.texcoord = Vec2(0.f, 0.f);

        Vec3 pos = vertex.position;
        Vec3 nor = vertex.normal;
        Vec3 tan = vertex.tangent;

        vertex.position = Vec3::Transform(pos, preTransformMatrix);
        vertex.normal = Vec3::TransformNormal(nor, preTransformMatrix);
        vertex.tangent = Vec3::TransformNormal(tan, preTransformMatrix);

        vertices[i] = vertex;
    }

    for (size_t i = 0; i < aiMesh->mNumFaces; ++i)
    {
        aiFace face = aiMesh->mFaces[i];
        if (face.mNumIndices != 3)
            continue;

        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }

    return Initialize_Prototype(aiMesh->mName.C_Str(), aiMesh->mMaterialIndex, vertices, indices);
}

HRESULT Mesh::Initialize_Prototype(const string& meshName, uint32 materialIndex,
    const vector<VTXMESH>& vertices, const vector<uint32>& indices)
{
    _meshName = meshName;
    _materialIndex = materialIndex;

    return Create_Buffers(vertices, indices);
}

HRESULT Mesh::Create_Buffers(const vector<VTXMESH>& vertices, const vector<uint32>& indices)
{
    _numVertexBuffers = 1;
    _numVertices = static_cast<uint32>(vertices.size());
    _vertexStride = sizeof(VTXMESH);
    _numIndices = static_cast<uint32>(indices.size());
    _indexStride = sizeof(uint32);
    _primitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    D3D11_BUFFER_DESC vertexBufferDesc{};
    vertexBufferDesc.ByteWidth = _vertexStride * _numVertices;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.StructureByteStride = _vertexStride;

    D3D11_SUBRESOURCE_DATA vertexInitialData{};
    vertexInitialData.pSysMem = vertices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &vertexBufferDesc, &vertexInitialData, _vertexBuffer.GetAddressOf()), E_FAIL);

    D3D11_BUFFER_DESC indexBufferDesc{};
    indexBufferDesc.ByteWidth = _indexStride * _numIndices;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.StructureByteStride = _indexStride;

    D3D11_SUBRESOURCE_DATA indexInitialData{};
    indexInitialData.pSysMem = indices.data();

    CHECK_FAILED(_device->CreateBuffer(
        &indexBufferDesc, &indexInitialData, _indexBuffer.GetAddressOf()), E_FAIL);

    return S_OK;
}

HRESULT Mesh::Initialize(void* arg)
{
    return VIBuffer::Initialize(arg);
}

Shared<Mesh> Mesh::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const aiMesh* aiMesh, const Matrix& preTransformMatrix)
{
    auto instance = make_shared<Mesh>(device, context);

    if (FAILED(instance->Initialize_Prototype(aiMesh, preTransformMatrix)))
    {
        MSG_BOX("Failed to Create : Mesh");
        instance->Free();
        return nullptr;
    }

    return instance;
}

Shared<Mesh> Mesh::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const string& meshName, uint32 materialIndex,
    const vector<VTXMESH>& vertices, const vector<uint32>& indices)
{
    auto instance = make_shared<Mesh>(device, context);

    if (FAILED(instance->Initialize_Prototype(meshName, materialIndex, vertices, indices)))
    {
        MSG_BOX("Failed to Create : Mesh");
        instance->Free();
        return nullptr;
    }

    return instance;
}

Shared<Component> Mesh::Clone(void* arg)
{
    auto clone = make_shared<Mesh>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Mesh");
        clone->Free();
        return nullptr;
    }

    return clone;
}

void Mesh::Free()
{
    VIBuffer::Free();
}
```

## 7-5. 교체 파일: `Engine/Public/ModelMaterial.h`

설명 메모:

- 여기서는 머티리얼 초기화 경로가 둘로 나뉜다.
- `Initialize(...)`는 기존 FBX 런타임 로드용이고, `Initialize_FromJson(...)`는 컨버터가 뽑은 `.material.json` 로드용이다.
- 즉 `ModelMaterial`은 "Assimp 머티리얼 래퍼"에서 "엔진 머티리얼 로더"로 책임이 조금 넓어진다.

```cpp
#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL ModelMaterial : public Base
{
    GENERATED_BODY(ModelMaterial)

public:
    explicit ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~ModelMaterial() = default;

public:
    // 기존 FBX 직접 로드 경로.
    HRESULT Initialize(const aiMaterial* aiMaterial, const string& modelFilePath);
    // 컨버터가 만든 .material.json 로드 경로.
    HRESULT Initialize_FromJson(const json& data, const string& materialFilePath);
    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName, aiTextureType type, uint32 textureIndex);

    string  Get_MaterialName() const { return _materialName; }
    uint32  Get_TextureCount(aiTextureType type) const;
    string  Get_TextureGuid(aiTextureType type, uint32 index) const;
    HRESULT Override_Texture(aiTextureType type, uint32 index, const string& guid);

public:
    json    To_Json() const;
    void    From_Json(const json& data);

private:
    HRESULT Load_Texture_File(aiTextureType type, const string& texturePath, const string& baseFilePath);

private:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

    vector<ComPtr<ShaderResourceView>> _textures[AI_TEXTURE_TYPE_MAX];
    vector<string>                     _textureGuids[AI_TEXTURE_TYPE_MAX];

    string _materialName;

public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const aiMaterial* aiMaterial, const string& modelFilePath);
    static Shared<ModelMaterial> CreateFromJson(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const json& data, const string& materialFilePath);

    void Free() override;
};

NS_END
```

## 7-6. 교체 파일: `Engine/Private/ModelMaterial.cpp`

설명 메모:

- `.material.json`에서는 텍스처 경로만 받아서 로드하므로, 파일 경로 해석을 한 곳으로 모으는 `Load_Texture_File(...)` 같은 함수가 중요하다.
- relative path를 material json 파일 기준으로 해석해야 same base name 규칙이 자연스럽게 동작한다.

```cpp
#include "pch.h"
#include "ModelMaterial.h"

#include "GameInstance.h"
#include "Shader.h"

ModelMaterial::ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT ModelMaterial::Initialize(const aiMaterial* aiMaterial, const string& modelFilePath)
{
    aiString matName;
    if (aiMaterial->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS)
        _materialName = matName.C_Str();

    for (uint32 typeIndex = 0; typeIndex < AI_TEXTURE_TYPE_MAX; ++typeIndex)
    {
        aiTextureType type = static_cast<aiTextureType>(typeIndex);
        uint32 numTextures = aiMaterial->GetTextureCount(type);

        for (uint32 i = 0; i < numTextures; ++i)
        {
            aiString texturePath;
            if (aiMaterial->GetTexture(type, i, &texturePath) != AI_SUCCESS)
                continue;

            Load_Texture_File(type, texturePath.C_Str(), modelFilePath);
        }
    }

    return S_OK;
}

HRESULT ModelMaterial::Initialize_FromJson(const json& data, const string& materialFilePath)
{
    // material json은 텍스처 경로와 이름만 넘겨준다고 가정한다.
    _materialName = data.value("material_name", data.value("name", string("Material")));

    if (!data.contains("textures"))
        return S_OK;

    const json& textures = data["textures"];

    CHECK_FAILED(Load_Texture_File(aiTextureType_DIFFUSE, textures.value("diffuse", ""), materialFilePath), E_FAIL);
    CHECK_FAILED(Load_Texture_File(aiTextureType_NORMALS, textures.value("normal", ""), materialFilePath), E_FAIL);
    CHECK_FAILED(Load_Texture_File(aiTextureType_SPECULAR, textures.value("specular", ""), materialFilePath), E_FAIL);

    return S_OK;
}

HRESULT ModelMaterial::Load_Texture_File(aiTextureType type, const string& texturePath, const string& baseFilePath)
{
    if (texturePath.empty())
        return S_OK;

    filesystem::path finalPath(texturePath);
    if (finalPath.is_relative())
    {
        // same base name 규칙을 쓰므로 material json 파일 위치 기준으로 상대경로를 푼다.
        filesystem::path baseDir = filesystem::path(baseFilePath).parent_path();
        finalPath = baseDir / finalPath;
    }

    wstring wPath = filesystem::absolute(finalPath).wstring();
    wstring ext = filesystem::path(wPath).extension().wstring();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    ComPtr<ShaderResourceView> srv;
    HRESULT hr = E_FAIL;

    if (ext == L".dds")
        hr = DirectX::CreateDDSTextureFromFile(_device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());
    else
        hr = DirectX::CreateWICTextureFromFile(_device.Get(), wPath.c_str(), nullptr, srv.GetAddressOf());

    if (FAILED(hr))
    {
        LOG_WARN("ModelMaterial: Failed to load texture - {}", filesystem::path(wPath).string());
        return S_OK;
    }

    const uint32 typeIndex = static_cast<uint32>(type);
    _textures[typeIndex].push_back(srv);
    _textureGuids[typeIndex].push_back(GAME->Find_AssetGUID(wPath));

    return S_OK;
}

HRESULT ModelMaterial::Bind_Material(Shared<Shader> shader, const char* constantName, aiTextureType type, uint32 textureIndex)
{
    const uint32 typeIndex = static_cast<uint32>(type);

    if (typeIndex >= AI_TEXTURE_TYPE_MAX)
        return E_FAIL;

    if (textureIndex >= _textures[typeIndex].size())
        return E_FAIL;

    return shader->Bind_SRV(constantName, _textures[typeIndex][textureIndex]);
}

uint32 ModelMaterial::Get_TextureCount(aiTextureType type) const
{
    const uint32 idx = static_cast<uint32>(type);
    if (idx >= AI_TEXTURE_TYPE_MAX)
        return 0;

    return static_cast<uint32>(_textures[idx].size());
}

string ModelMaterial::Get_TextureGuid(aiTextureType type, uint32 index) const
{
    const uint32 idx = static_cast<uint32>(type);

    if (idx >= AI_TEXTURE_TYPE_MAX || index >= _textureGuids[idx].size())
        return "";

    return _textureGuids[idx][index];
}
HRESULT ModelMaterial::Override_Texture(aiTextureType type, uint32 index, const string& guid)
{
    const uint32 idx = static_cast<uint32>(type);
    if (idx >= AI_TEXTURE_TYPE_MAX)
        return E_FAIL;

    wstring path = GAME->Resolve_AssetPath(guid);
    if (path.empty())
        return E_FAIL;

    ComPtr<ShaderResourceView> srv;
    wstring ext = filesystem::path(path).extension().wstring();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    HRESULT hr = E_FAIL;
    if (ext == L".dds")
        hr = DirectX::CreateDDSTextureFromFile(_device.Get(), path.c_str(), nullptr, srv.GetAddressOf());
    else
        hr = DirectX::CreateWICTextureFromFile(_device.Get(), path.c_str(), nullptr, srv.GetAddressOf());

    CHECK_FAILED(hr, E_FAIL);

    if (index < _textures[idx].size())
    {
        _textures[idx][index] = srv;
        _textureGuids[idx][index] = guid;
    }
    else
    {
        _textures[idx].push_back(srv);
        _textureGuids[idx].push_back(guid);
    }

    return S_OK;
}

json ModelMaterial::To_Json() const
{
    json j;
    j["material_name"] = _materialName;

    json texOverrides = json::object();

    for (uint32 typeIndex = 0; typeIndex < AI_TEXTURE_TYPE_MAX; ++typeIndex)
    {
        for (uint32 i = 0; i < _textureGuids[typeIndex].size(); ++i)
        {
            if (_textureGuids[typeIndex][i].empty())
                continue;

            string key = to_string(typeIndex) + "_" + to_string(i);
            texOverrides[key] = _textureGuids[typeIndex][i];
        }
    }

    if (!texOverrides.empty())
        j["texture_overrides"] = texOverrides;

    return j;
}

void ModelMaterial::From_Json(const json& data)
{
    if (!data.contains("texture_overrides"))
        return;

    const auto& overrides = data["texture_overrides"];
    for (auto& [key, val] : overrides.items())
    {
        size_t sep = key.find('_');
        if (sep == string::npos)
            continue;

        uint32 typeIdx = static_cast<uint32>(stoi(key.substr(0, sep)));
        uint32 texIdx = static_cast<uint32>(stoi(key.substr(sep + 1)));
        string guid = val.get<string>();

        if (typeIdx >= AI_TEXTURE_TYPE_MAX || guid.empty())
            continue;

        if (texIdx < _textureGuids[typeIdx].size() && _textureGuids[typeIdx][texIdx] == guid)
            continue;

        Override_Texture(static_cast<aiTextureType>(typeIdx), texIdx, guid);
    }
}

Shared<ModelMaterial> ModelMaterial::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const aiMaterial* aiMaterial, const string& modelFilePath)
{
    auto instance = make_shared<ModelMaterial>(device, context);

    if (FAILED(instance->Initialize(aiMaterial, modelFilePath)))
    {
        MSG_BOX("Failed to Create : ModelMaterial");
        return nullptr;
    }

    return instance;
}

Shared<ModelMaterial> ModelMaterial::CreateFromJson(ComPtr<Device> device, ComPtr<DeviceContext> context,
    const json& data, const string& materialFilePath)
{
    auto instance = make_shared<ModelMaterial>(device, context);

    if (FAILED(instance->Initialize_FromJson(data, materialFilePath)))
    {
        MSG_BOX("Failed to Create : ModelMaterial");
        return nullptr;
    }

    return instance;
}

void ModelMaterial::Free()
{
    for (auto& texArray : _textures)
        texArray.clear();

    Base::Free();
}
```

## 7-7. 교체 파일: `Engine/Public/Model.h`

설명 메모:

- 이 파일이 실제 핵심 분기점이다.
- `Model`은 파일 확장자를 보고 `FBX -> Assimp 로드`와 `.meshbin -> Binary 로드`를 선택한다.
- 그래서 binary 경로를 붙일 때 `Client`보다 `Model` 수정이 더 중요하다.

```cpp
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
    HRESULT Render(uint32 meshIndex);

    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName,
        uint32 meshIndex, aiTextureType materialType, uint32 textureIndex);

    size_t Get_NumMeshes() const { return _meshes.size(); }

public:
    json To_Json() const override;
    void From_Json(const json& data) override;

    size_t Get_NumMaterials() const { return _materials.size(); }
    Shared<ModelMaterial> Get_Material(uint32 index) const;

    uint32 Get_MeshMaterialIndex(uint32 index) const;
    string Get_MeshName(uint32 index);

private:
    // .meshbin 확장자일 때 들어오는 초기화 경로.
    HRESULT Initialize_FromMeshBin(const string& modelFilePath);
    HRESULT Ready_Meshes();
    // Binary loader가 읽은 raw 데이터를 VTXMESH / Mesh로 바꾸는 단계.
    HRESULT Ready_Meshes_FromBinary(const string& modelFilePath);
    HRESULT Ready_Materials(const string& modelFilePath);
    // same base name 규칙으로 .material.json 을 읽는 단계.
    HRESULT Ready_Materials_FromJson(const string& materialFilePath);

    void Apply_MaterialOverride(const json& data);
    string Build_MaterialJsonPath(const string& modelFilePath) const;

private:
    const aiScene*                  _aiScene = { nullptr };
    Importer                        _importer = { };

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
```

## 7-8. 교체 파일: `Engine/Private/Model.cpp`

설명 메모:

- `Initialize_Prototype(...)`에서 확장자 분기를 먼저 처리하면 기존 FBX 경로와 새 `.meshbin` 경로를 함께 유지할 수 있다.
- `Ready_Meshes()`는 Assimp scene 전용, `Ready_Meshes_FromBinary()`는 meshbin 전용으로 나누는 편이 읽기 쉽다.
- `Ready_Materials_FromJson()`도 같은 이유다. aiMaterial 경로와 json 경로를 섞지 말고 분리해 두는 게 유지보수에 유리하다.

```cpp
#include "pch.h"
#include "Model.h"

#include <fstream>

#include "Mesh.h"
#include "ModelBinaryLoader.h"
#include "ModelMaterial.h"
#include "GameInstance.h"

Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _aiScene(rhs._aiScene)
    , _modelType(rhs._modelType)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
    , _numMaterials(rhs._numMaterials)
    , _materials(rhs._materials)
    , _modelGuid(rhs._modelGuid)
{
}

HRESULT Model::Initialize_Prototype(EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    _modelType = type;
    _preLocalTransformMatrix = preLocalTransformMatrix;

    // 확장자 기준으로 FBX 직접 로드와 meshbin 로드를 분기한다.
    filesystem::path path(modelFilePath);
    string ext = path.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".meshbin")
        return Initialize_FromMeshBin(modelFilePath);

    uint32 flag = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;

    if (_modelType == EModelType::StaticMesh)
        flag |= aiProcess_PreTransformVertices;

    _aiScene = _importer.ReadFile(modelFilePath, flag);
    CHECK_NULL(_aiScene, E_FAIL);

    CHECK_FAILED(Ready_Meshes(), E_FAIL);
    CHECK_FAILED(Ready_Materials(modelFilePath), E_FAIL);

    return S_OK;
}

HRESULT Model::Initialize_FromMeshBin(const string& modelFilePath)
{
    // 메시 본체는 meshbin, 머티리얼은 same base name의 .material.json 에서 읽는다.
    CHECK_FAILED(Ready_Meshes_FromBinary(modelFilePath), E_FAIL);
    CHECK_FAILED(Ready_Materials_FromJson(Build_MaterialJsonPath(modelFilePath)), E_FAIL);
    return S_OK;
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
    aiTextureType materialType, uint32 textureIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    const uint32 materialIndex = _meshes[meshIndex]->Get_MaterialIndex();
    if (materialIndex >= _materials.size())
        return E_FAIL;

    return _materials[materialIndex]->Bind_Material(shader, constantName, materialType, textureIndex);
}
HRESULT Model::Ready_Meshes()
{
    _numMeshes = _aiScene->mNumMeshes;

    for (size_t i = 0; i < _numMeshes; ++i)
    {
        Shared<Mesh> mesh = Mesh::Create(_device, _context, _aiScene->mMeshes[i], _preLocalTransformMatrix);
        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_Meshes_FromBinary(const string& modelFilePath)
{
    // binary loader가 읽은 raw 정점을 엔진의 VTXMESH로 변환한 뒤
    // 최종적으로는 기존과 동일한 Mesh 객체를 만든다.
    vector<FMeshBinaryData> meshList;
    uint32 materialCount = 0;

    if (!ModelBinaryLoader::Load(modelFilePath, meshList, materialCount))
        return E_FAIL;

    _meshes.clear();
    _numMeshes = static_cast<uint32>(meshList.size());
    _numMaterials = materialCount;

    for (const FMeshBinaryData& srcMesh : meshList)
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

HRESULT Model::Ready_Materials(const string& modelFilePath)
{
    _numMaterials = _aiScene->mNumMaterials;

    for (size_t i = 0; i < _numMaterials; ++i)
    {
        Shared<ModelMaterial> material = ModelMaterial::Create(
            _device, _context, _aiScene->mMaterials[i], modelFilePath);
        CHECK_NULL(material, E_FAIL);
        _materials.push_back(material);
    }

    return S_OK;
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

    if (!root.contains("materials"))
    {
        LOG_ERROR("Invalid material json: {}", materialFilePath);
        return E_FAIL;
    }

    _materials.clear();

    for (const auto& item : root["materials"])
    {
        Shared<ModelMaterial> material = ModelMaterial::CreateFromJson(
            _device, _context, item, materialFilePath);
        CHECK_NULL(material, E_FAIL);
        _materials.push_back(material);
    }

    _numMaterials = static_cast<uint32>(_materials.size());
    return S_OK;
}

string Model::Build_MaterialJsonPath(const string& modelFilePath) const
{
    filesystem::path path(modelFilePath);
    path.replace_extension(".material.json");
    return path.string();
}

void Model::Apply_MaterialOverride(const json& data)
{
    if (!data.contains("materials"))
        return;

    const auto& matArray = data["materials"];
    for (size_t i = 0; i < matArray.size() && i < _materials.size(); ++i)
    {
        if (_materials[i] && !matArray[i].empty())
            _materials[i]->From_Json(matArray[i]);
    }
}

json Model::To_Json() const
{
    json j = Component::To_Json();

    if (!_modelGuid.empty())
        j["model_guid"] = _modelGuid;

    j["model_type"] = (_modelType == EModelType::SkeletalMesh) ? "SkeletalMesh" : "StaticMesh";

    if (!_materials.empty())
    {
        json matArray = json::array();
        for (const auto& mat : _materials)
        {
            if (mat)
                matArray.push_back(mat->To_Json());
            else
                matArray.push_back(json::object());
        }
        j["materials"] = matArray;
    }

    return j;
}

void Model::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("model_guid"))
        return;

    string newGuid = data["model_guid"].get<string>();
    string newTypeStr = data.value("model_type", "SkeletalMesh");
    EModelType newType = (newTypeStr == "SkeletalMesh") ? EModelType::SkeletalMesh : EModelType::StaticMesh;

    if (newGuid == _modelGuid && newType == _modelType)
    {
        Apply_MaterialOverride(data);
        return;
    }

    _modelGuid = newGuid;
    _modelType = newType;

    wstring path = GAME->Resolve_AssetPath(_modelGuid);
    if (path.empty())
    {
        LOG_WARN("Model GUID not found: {}", _modelGuid);
        return;
    }

    _meshes.clear();
    _materials.clear();

    if (_aiScene)
    {
        _importer.FreeScene();
        _aiScene = nullptr;
    }

    Matrix preTransform = Matrix::CreateScale(0.01f);
    if (_modelType == EModelType::SkeletalMesh)
    {
        preTransform = preTransform
            * Matrix::CreateRotationX(XMConvertToRadians(90.f))
            * Matrix::CreateRotationY(XMConvertToRadians(180.f));
    }

    Initialize_Prototype(_modelType, Utils::ToString(path), preTransform);
    Apply_MaterialOverride(data);
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

Shared<Model> Model::Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
    EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
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

    _meshes.clear();
    _materials.clear();

    _importer.FreeScene();
}
```

---

## 8. `ModelTable.json` 변경 예시

설명 메모:

- 이 단계에서 클라이언트 쪽 실질 변경은 대부분 여기서 끝난다.
- `ResourceLoader`는 이미 `path` 문자열을 그대로 전달하고 있으므로, 데이터 테이블에서 `.fbx`를 `.meshbin`으로 바꾸는 것이 핵심이다.
- 처음에는 한 개 모델만 바꿔서 FBX 로드 결과와 meshbin 로드 결과를 비교하는 것이 가장 안전하다.

처음에는 한 개만 바꾸는 걸 권장한다.

예시:

```json
{
    "Model": [
        {
            "id": "COMPONENT_TYPE_MODEL_PLAYER",
            "level": "Static",
            "path": "../../Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.meshbin",
            "count": 1,
            "modelType": "SkeletalMesh"
        },
        {
            "id": "COMPONENT_TYPE_MODEL_MONSTER",
            "level": "Static",
            "path": "../../Client/Bin/Resources/Models/Boruto/Boruto.fbx",
            "count": 1,
            "modelType": "SkeletalMesh"
        }
    ]
}
```

처음에는 `PLAYER` 하나만 `.meshbin`으로 바꾸고, `MONSTER`는 그대로 둔다.
이렇게 해야 비교가 쉽다.

---

## 9. 실제 실행 순서

1. 위 코드대로 새 파일 추가
2. 기존 파일 교체
3. Visual Studio 프로젝트에 새 파일 등록
4. `AssimpTool` 빌드
5. `AssimpTool.cpp` 안 경로를 원하는 FBX로 수정
6. `AssimpTool` 실행
7. `.meshbin`, `.material.json` 생성 확인
8. `ModelTable.json`에서 path를 `.meshbin`으로 교체
9. 게임 실행

출력 파일이 생겨야 하는 위치 예시:

- `Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.meshbin`
- `Client/Bin/Resources/Models/Gaara/SK_CHR_Gaara.material.json`

---

## 10. 확인 포인트

AssimpTool 실행 후:

- `.meshbin`이 생성되는지
- `.material.json`이 생성되는지
- `.material.json` 안 텍스처 경로가 비어있지 않은지

게임 실행 후:

- 기존 FBX와 같은 메시 개수로 보이는지
- diffuse texture가 붙는지
- static mesh는 기존과 같은 방향/스케일인지
- skeletal mesh도 현재 프로젝트에서 보이던 것과 비슷하게 보이는지

---

## 11. 이 버전의 한계

이 문서는 일부러 범위를 좁혔다.

- 애니메이션 없음
- 스킨 weight 없음
- bone 없음
- embedded texture 없음
- material color 상수값 저장 없음

하지만 이 버전만으로도 충분히 가치가 있다.

- `FBX 직접 로드 -> 커스텀 포맷 로드` 전환의 골격이 완성된다
- 추후 `clipbin`, `bone`, `weight`를 붙일 위치가 생긴다
- `GUID` 기반 확장도 이 위에서 하는 게 훨씬 쉽다

---

## 12. 추천 진행 순서

가장 좋은 순서는 아래다.

1. `StaticMesh` 하나로 먼저 성공
2. `SkeletalMesh` 하나를 동일 방식으로 성공
3. `.meshbin` 경로 기반 로딩 안정화
4. 그 다음에 `GUID 연동`
5. 마지막에 `Animation/Clip`

현재 프로젝트에서는 이 순서가 가장 리스크가 낮다.
