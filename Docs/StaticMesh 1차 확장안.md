# StaticMesh `COLOR_0 + TEXCOORD1` 1차 확장안
## Summary

내 의견은 1차는 **`COLOR_0 + TEXCOORD1`만 정식 지원**하고, 바닥 복원은 그 위에 **`ground_blend_v1` 프로파일**을 얹는 게 가장 좋습니다.

왜 이 구성이 좋냐면:
- 현재 [Converter.cpp](/d:/GitDesktop/Dx11_Naruto/AssimpTool/Private/Converter.cpp), [ModelBin_Types.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/ModelBin_Types.h), [Vertex_Struct.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/Vertex_Struct.h), [Shader_VtxStaticMesh.hlsl](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Shaders/Shader_VtxStaticMesh.hlsl) 전부가 `UV0 + base color 1장` 기준이라, 원본 GroundSoil 재질의 핵심인 `Base + Blend + Mask` 구조를 못 따라갑니다.
- 하지만 1차에서 `UV1`과 `vertex color`만 열어도, `BaseColor + BlendBaseColor + Mask` 조합은 충분히 구현 가능합니다.
- `TEXCOORD2/3`, `BlendNormal`, `UnevenColor`까지 한 번에 열면 범위가 너무 커집니다.

이번 1차 목표는:
- 기존 v1 static meshbin은 그대로 읽기
- 새로 변환하는 static meshbin은 v2로 쓰기
- static mesh vertex에 `texcoord1`, `color0` 추가
- `ground_blend_v1` 머티리얼 프로파일 추가
- 바닥은 `base_color + blend_base_color + mask`로 2레이어 블렌딩

## Public / Format Changes

### 1. [변경] static meshbin 버전 업
대상:
- [ConverterTypes.h](/d:/GitDesktop/Dx11_Naruto/AssimpTool/Public/ConverterTypes.h)
- [ModelBin_Types.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/ModelBin_Types.h)

결정:
- 기존 static meshbin v1 읽기 유지
- 새 static meshbin은 v2로 기록

```cpp
// [변경] static mesh 전용 버전 상수 분리
constexpr uint32 STATIC_MESHBIN_VERSION_V1 = 1;
constexpr uint32 STATIC_MESHBIN_VERSION_V2 = 2;
constexpr uint32 STATIC_MESHBIN_VERSION = STATIC_MESHBIN_VERSION_V2;
```

### 2. [추가] static vertex 포맷 확장
1차는 `UV1 + COLOR0`까지만 엽니다. `UV2/UV3`는 2차로 미룹니다.

```cpp
// [추가] v1 legacy reader용
struct FMeshVertexBinV1
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 1.f, nz = 0.f;
    float tx = 1.f, ty = 0.f, tz = 0.f;
    float u = 0.f, v = 0.f;
};

// [변경] runtime / v2 writer 공용 static vertex 포맷
struct FMeshVertexBin
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 1.f, nz = 0.f;
    float tx = 1.f, ty = 0.f, tz = 0.f;

    // [변경] 기존 UV0
    float u0 = 0.f, v0 = 0.f;

    // [추가] 1차 확장: UV1
    float u1 = 0.f, v1 = 0.f;

    // [추가] 1차 확장: COLOR_0
    float cr = 1.f, cg = 1.f, cb = 1.f, ca = 1.f;
};
```

## Implementation Changes

### 1. [변경] AssimpTool에서 `UV1`, `COLOR_0` 추출
대상:
- [Converter.cpp](/d:/GitDesktop/Dx11_Naruto/AssimpTool/Private/Converter.cpp)

`Build_StaticMeshData()` 안에서 `mTextureCoords[1]`, `mColors[0]`까지 채웁니다.

```cpp
for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
{
    FMeshVertexBin vertex{};

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

    // [유지] UV0
    if (srcMesh->HasTextureCoords(0))
    {
        vertex.u0 = srcMesh->mTextureCoords[0][v].x;
        vertex.v0 = srcMesh->mTextureCoords[0][v].y;
    }

    // [추가] UV1. 없으면 UV0로 fallback
    if (srcMesh->HasTextureCoords(1))
    {
        vertex.u1 = srcMesh->mTextureCoords[1][v].x;
        vertex.v1 = srcMesh->mTextureCoords[1][v].y;
    }
    else
    {
        vertex.u1 = vertex.u0;
        vertex.v1 = vertex.v0;
    }

    // [추가] COLOR_0. 없으면 white
    if (srcMesh->HasVertexColors(0))
    {
        vertex.cr = srcMesh->mColors[0][v].r;
        vertex.cg = srcMesh->mColors[0][v].g;
        vertex.cb = srcMesh->mColors[0][v].b;
        vertex.ca = srcMesh->mColors[0][v].a;
    }

    meshData.staticVertices.push_back(vertex);
}
```

### 2. [변경] Engine loader는 v1/v2 둘 다 읽기
대상:
- [Model_BinaryLoader.cpp](/d:/GitDesktop/Dx11_Naruto/Engine/Private/Model_BinaryLoader.cpp)

핵심:
- `version == 1`이면 `FMeshVertexRawV1` 읽고 새 포맷으로 승격
- `version == 2`이면 새 포맷 그대로 읽기

```cpp
// [추가] v1 -> runtime vertex 승격
static FMeshVertexRaw Upgrade_StaticVertex_V1(const FMeshVertexRawV1& legacy)
{
    FMeshVertexRaw out{};
    out.px = legacy.px; out.py = legacy.py; out.pz = legacy.pz;
    out.nx = legacy.nx; out.ny = legacy.ny; out.nz = legacy.nz;
    out.tx = legacy.tx; out.ty = legacy.ty; out.tz = legacy.tz;

    out.u0 = legacy.u;
    out.v0 = legacy.v;

    // [추가] old mesh는 UV1을 UV0로 맞춘다.
    out.u1 = legacy.u;
    out.v1 = legacy.v;

    // [추가] old mesh는 vertex color가 없으므로 white
    out.cr = 1.f; out.cg = 1.f; out.cb = 1.f; out.ca = 1.f;
    return out;
}
```

```cpp
// [변경] static mesh version 분기
if (version == STATIC_MESHBIN_VERSION_V1)
    return Read_V1_Static(file, filePath, header, outData);

if (version == STATIC_MESHBIN_VERSION_V2)
    return Read_V2_Static(file, filePath, header, outData);
```

### 3. [변경] runtime vertex/input layout 확장
대상:
- [Vertex_Struct.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/Vertex_Struct.h)
- [Model.cpp](/d:/GitDesktop/Dx11_Naruto/Engine/Private/Model.cpp)

`VTXMESH`를 아래처럼 바꿉니다.

```cpp
typedef struct FVertexMesh
{
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;

    // [변경] UV0
    Vec2 texcoord0;

    // [추가] UV1
    Vec2 texcoord1;

    // [추가] COLOR_0
    Vec4 color;

    static const uint32 numElements = { 6 };

    static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT,    0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
} VTXMESH;
```

`Model::Ready_StaticMeshes()`도 같이 바꿉니다.

```cpp
VTXMESH vertex{};
vertex.position = Vec3(raw.px, raw.py, raw.pz);
vertex.normal   = Vec3(raw.nx, raw.ny, raw.nz);
vertex.tangent  = Vec3(raw.tx, raw.ty, raw.tz);

// [변경] UV0 / [추가] UV1 / [추가] COLOR_0
vertex.texcoord0 = Vec2(raw.u0, raw.v0);
vertex.texcoord1 = Vec2(raw.u1, raw.v1);
vertex.color     = Vec4(raw.cr, raw.cg, raw.cb, raw.ca);
```

### 4. [변경] 머티리얼 슬롯 / 프로파일 확장
대상:
- [Engine_Enum.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/Engine_Enum.h)
- [ModelMaterial.h](/d:/GitDesktop/Dx11_Naruto/Engine/Public/ModelMaterial.h)
- [ModelMaterial.cpp](/d:/GitDesktop/Dx11_Naruto/Engine/Private/ModelMaterial.cpp)
- [ResolveFModelMaterials.py](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Data/MapTools/ResolveFModelMaterials.py)

1차 슬롯은 여기까지로 고정합니다.

```cpp
enum class EMaterialTextureSlot : uint32
{
    BaseColor = 0,
    Normal,
    Specular,
    Emissive,
    AmbientOcclusion,
    Metalness,
    Roughness,

    // [추가] 1차 ground blend용
    BlendBaseColor,
    Mask,

    END
};
```

`ModelMaterial`에는 tint 하나 추가합니다.

```cpp
// [추가] ground blend 두 번째 레이어 tint
Vec4 _blendColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
const Vec4& Get_BlendColorFactor() const { return _blendColorFactor; }
```

slot string 변환도 추가:

```cpp
if (slot == "blend_base_color") return EMaterialTextureSlot::BlendBaseColor;
if (slot == "mask")             return EMaterialTextureSlot::Mask;
```

resolver 쪽은 `GroundSoil`을 `ground_blend_v1`로 분기합니다.

```python
def detect_profile(material_name: str, profile_config: dict) -> str:
    lower = material_name.lower()

    # [추가] 1차 확장: ground blend 프로파일
    if "groundsoil" in lower:
        return "ground_blend_v1"
```

```python
# [추가] GroundSoil 1차 프로파일
def resolve_ground_blend_v1(material_name: str, mi_data: dict, texture_index: dict, owner_json_path: Path,
                            copy_root, dry_run: bool):
    textures = get_textures(mi_data)
    colors = get_colors(mi_data)
    scalars = get_scalars(mi_data)
    params = get_parameters(mi_data)

    base_color_ref = find_first_key(textures, ["AA_Override_BaseColorMap", "AA_BaseColorMap"])
    blend_base_color_ref = find_first_key(textures, ["AB_BlendBaseColorMap"])
    mask_ref = find_first_key(textures, ["EA_MaskMap"])

    out = {
        "version": 1,
        "name": material_name,
        "profile": "ground_blend_v1",
        "parent": "M_ENV_GroundBlend_V1",
        "base_color_factor": get_color4(colors, "AA_BaseMixColor", [1.0, 1.0, 1.0, 1.0]),
        "blend_color_factor": get_color4(colors, "AA_BlendMixColor", [1.0, 1.0, 1.0, 1.0]),
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": int(params.get("BlendMode", 0)),
        "textures": [],
    }

    base_color_path = make_texture_path(resolve_source_texture_path(base_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    blend_base_color_path = make_texture_path(resolve_source_texture_path(blend_base_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    mask_path = make_texture_path(resolve_source_texture_path(mask_ref, texture_index), owner_json_path, copy_root, dry_run)

    if base_color_path:
        out["textures"].append({"slot": "base_color", "index": 0, "path": base_color_path})
    if blend_base_color_path:
        out["textures"].append({"slot": "blend_base_color", "index": 0, "path": blend_base_color_path})
    if mask_path:
        out["textures"].append({"slot": "mask", "index": 0, "path": mask_path})

    return out
```

### 5. [변경] static mesh shader / actor에 ground blend 분기 추가
대상:
- [Shader_VtxStaticMesh.hlsl](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Shaders/Shader_VtxStaticMesh.hlsl)
- [StaticMeshActor.cpp](/d:/GitDesktop/Dx11_Naruto/Client/Private/StaticMeshActor.cpp)

셰이더 입력:

```hlsl
struct VS_IN
{
    float3 vPosition  : POSITION;
    float3 vNormal    : NORMAL;
    float3 vTangent   : TANGENT;
    float2 vTexcoord0 : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vColor     : COLOR0;
};

struct VS_OUT
{
    float4 vPosition  : SV_POSITION;
    float4 vNormal    : NORMAL;
    float2 vTexcoord0 : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vColor     : COLOR0;
    float4 vWorldPos  : TEXCOORD2;
};
```

셰이더 상수:

```hlsl
Texture2D g_DiffuseTexture;
Texture2D g_BlendDiffuseTexture;
Texture2D g_MaskTexture;

float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_BlendColorFactor = float4(1.f, 1.f, 1.f, 1.f);

int g_HasDiffuseTexture = 1;
int g_HasBlendDiffuseTexture = 0;
int g_HasMaskTexture = 0;
int g_UseGroundBlend = 0;
```

픽셀 셰이더 핵심:

```hlsl
vector baseColor = g_BaseColorFactor;
if (g_HasDiffuseTexture != 0)
    baseColor *= g_DiffuseTexture.Sample(DefaultSampler, In.vTexcoord0);

vector finalColor = baseColor;

// [추가] 1차 ground blend: UV0로 base/blend, UV1로 mask, COLOR0.r로 보정
if (g_UseGroundBlend != 0 && g_HasBlendDiffuseTexture != 0)
{
    float blendFactor = 1.f;

    if (g_HasMaskTexture != 0)
        blendFactor = g_MaskTexture.Sample(DefaultSampler, In.vTexcoord1).r;

    blendFactor *= saturate(In.vColor.r);

    vector blendColor = g_BlendColorFactor;
    blendColor *= g_BlendDiffuseTexture.Sample(DefaultSampler, In.vTexcoord0);

    finalColor = lerp(baseColor, blendColor, saturate(blendFactor));
}

Out.vColor = g_LightDiffuse * finalColor * shade + specularColor;
```

`StaticMeshActor` 바인딩:

```cpp
const bool useGroundBlend =
    (material && material->Get_MaterialProfile() == "ground_blend_v1");

int hasBlendDiffuseTexture = 0;
int hasMaskTexture = 0;
Vec4 blendColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);

if (material)
{
    blendColorFactor = material->Get_BlendColorFactor();
    hasBlendDiffuseTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::BlendBaseColor) > 0) ? 1 : 0;
    hasMaskTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;
}

int useGroundBlendInt = useGroundBlend ? 1 : 0;

CHECK_FAILED(_shaderCom->Bind_RawValue("g_UseGroundBlend", &useGroundBlendInt, sizeof(int)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasBlendDiffuseTexture", &hasBlendDiffuseTexture, sizeof(int)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasMaskTexture", &hasMaskTexture, sizeof(int)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_BlendColorFactor", &blendColorFactor, sizeof(Vec4)), E_FAIL);

if (hasBlendDiffuseTexture != 0)
{
    CHECK_FAILED(
        _modelCom->Bind_Material(_shaderCom, "g_BlendDiffuseTexture",
            static_cast<uint32>(i), EMaterialTextureSlot::BlendBaseColor, 0), E_FAIL);
}

if (hasMaskTexture != 0)
{
    CHECK_FAILED(
        _modelCom->Bind_Material(_shaderCom, "g_MaskTexture",
            static_cast<uint32>(i), EMaterialTextureSlot::Mask, 0), E_FAIL);
}
```

## Test Plan

1. Assimp 변환 확인
- `ArenaGround_Aa/Ab`를 다시 `meshbin`으로 만들고, v2 static meshbin이 생성돼야 한다.
- old Konoha meshbin도 여전히 로드돼야 한다.

2. matinst 확인
- [MI_ENV_TCHEXA_GroundSoil_A.matinst.json](/d:/GitDesktop/Dx11_Naruto/Client/Bin/Resources/Materials/FModel/ExamStadium/MI_ENV_TCHEXA_GroundSoil_A.matinst.json)에
  - `profile = "ground_blend_v1"`
  - `base_color`
  - `blend_base_color`
  - `mask`
  가 들어가야 한다.

3. 런타임 확인
- 바닥이 단일 흙색이 아니라 grass/dirt 패턴으로 깨져 보여야 한다.
- mask가 틀리면 UV1 대신 UV0로 샘플링하는 실험만 추가 확인한다.

4. 회귀 확인
- 기존 static mesh actor들(건물, 벽, 나무)은 기존처럼 base color 1장만으로 정상 렌더돼야 한다.
- old meshbin(v1)은 white vertex color + uv1=uv0 fallback으로 깨지지 않아야 한다.

## Assumptions

- 1차는 `COLOR_0 + TEXCOORD1`만 지원하고, `TEXCOORD2/3`는 2차로 미룬다.
- 1차 바닥 복원의 핵심은 `BaseColor + BlendBaseColor + Mask`이며, `BlendNormal`, `UnevenColor`는 다음 단계로 미룬다.
- `EA_MaskMap`는 `UV1`, `vertex color.r`은 보조 blend factor로 쓰는 것으로 고정한다.
- `10일차_Brush` 프로젝트는 시스템 포팅 대상이 아니라, 마스크/브러시 개념 참고 자료로만 사용한다.
