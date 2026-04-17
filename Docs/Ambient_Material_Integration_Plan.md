# Ambient 재질 표현 이식 가이드

## 1. 오늘 수업 핵심 정리

`D:\Jusin_158\10개월차\6일차_RenderTarget6` 기준으로 오늘 수업의 핵심은 아래 2가지입니다.

1. `Light Ambient`를 실제 Deferred Light Pass에 바인딩한다.
2. `Material Ambient`를 Light Ambient와 곱해서 최종 `Shade`에 더한다.

수업 코드의 핵심 수식은 사실상 아래 한 줄입니다.

```hlsl
Out.vShade = g_vLightDiffuse * (max(dot(normalize(g_vLightDir) * -1.f, normalize(vNormal)), 0.f) + (g_vLightAmbient * g_vMtrlAmbient));
```

즉, 오늘 수업은 "어두운 영역이 완전한 검정으로 떨어지지 않게 하고, 재질마다 다른 ambient 반응을 주는 것"이 메인입니다.

## 2. 현재 프로젝트와 수업 코드의 차이

먼저 결론부터 말하면, **지금 프로젝트는 수업 코드의 `g_vMtrlAmbient` 상수 하나만 그대로 넣으면 안 됩니다.**

이유는 현재 프로젝트가:

1. 여러 재질이 한 화면에 동시에 섞이는 `Deferred` 구조이고,
2. `StaticMeshActor -> GBuffer(Target_Diffuse, Target_Normal) -> Light Pass -> Combined`
   흐름으로 동작하며,
3. 재질별 값은 GBuffer에 저장하지 않으면 Light Pass에서 구분할 수 없기 때문입니다.

즉, 수업 코드는 "화면 전체에 같은 material ambient를 쓰는 단순 구조"이고,
현재 프로젝트는 "픽셀마다 다른 material ambient가 필요한 구조"입니다.

그래서 현재 프로젝트에서는 아래처럼 가야 합니다.

1. 머티리얼 JSON에 ambient 값을 추가한다.
2. `ModelMaterial`이 그 값을 읽는다.
3. `StaticMeshActor`가 셰이더로 넘긴다.
4. `Shader_VtxStaticMesh.hlsl`가 GBuffer에 `material ambient / AO`를 기록한다.
5. `Shader_Deferred.hlsl`가 그 값을 읽어 `Light Ambient`와 곱한다.

이게 현재 레포에 맞는 정석 이식입니다.

## 3. 현재 레포 기준 진단

현재 프로젝트는 ambient 관련 뼈대는 이미 일부 있습니다.

1. `FLightDesc`에 `ambient`가 이미 있습니다.
2. `Engine_Shader_Defines.hlsli`에 `g_LightAmbient`, `g_MtrlAmbient`가 이미 있습니다.
3. `EMaterialTextureSlot::AmbientOcclusion` 슬롯도 이미 있습니다.
4. 하지만 실제 렌더링 결과로 이어지는 연결이 끊겨 있습니다.

특히 실제 병목은 아래입니다.

1. `Engine/Private/Light.cpp`에서 `g_LightAmbient`를 바인딩하지 않습니다.
2. `Client/Bin/Shaders/Shader_Deferred.hlsl`가 ambient 항을 계산하지 않습니다.
3. `Shader_VtxStaticMesh.hlsl`가 재질 ambient/AO를 GBuffer에 저장하지 않습니다.
4. `ModelMaterial`은 ambient factor를 아직 읽지 않습니다.
5. 현재 `KonohaVillage02`의 `.matinst.json`에는 ambient 전용 값이 없습니다.

## 4. 추천 이식 방향

이번 이식은 아래 방향으로 잡는 것을 추천합니다.

1. **1차 목표**
   KonohaVillage02 정적 환경 메시의 ambient 재질 표현부터 붙입니다.
2. **2차 목표**
   필요하면 `Shader_VtxAnimMesh.hlsl`까지 확장해서 캐릭터/무기에도 같은 구조를 붙입니다.

이번 요청 기준으로는 1차 목표가 메인입니다.

## 5. A to Z 수정 플랜

## [변경] 배치 변환 파이프라인 기준으로 다시 보는 핵심 전제

지금 KonohaVillage02 머티리얼은 사용자가 직접 `matinst.json`을 수작업으로 유지하는 구조가 아닙니다.

실제 원천 흐름은 아래입니다.

1. `Client/Bin/Resources/Data/MapTools/RunBuildKonohaVillage02AndLevel_fixed.bat`
2. `Client/Bin/Resources/Data/MapTools/BuildKonohaVillage02Pipeline.py`
3. `Client/Bin/Resources/Data/MapTools/ResolveFModelMaterials.py`
4. 최종 산출물
   `Client/Bin/Resources/Materials/FModel/KonohaVillage02/*.matinst.json`

즉, 제가 처음 적었던 아래 문장은 "결과물 구조 설명"으로는 맞아도, "실제 작업 기준"으로는 틀렸습니다.

```json
"ambient_factor": [1.0, 1.0, 1.0, 1.0],
"ambient_occlusion_strength": 1.0
```

왜 틀렸는지 요약하면:

1. 지금 `*.matinst.json`은 배치를 다시 돌리면 재생성됩니다.
2. 그래서 여기에 수동 필드를 박는 방식은 다음 변환 때 덮어써집니다.
3. 현재 파이프라인의 원천 데이터는 `D:\NarutoExports\...\Materials\*.props.txt`입니다.
4. 따라서 ambient 관련 값도 `ResolveFModelMaterials.py`가 생성하도록 넣어야 유지됩니다.

## [대체] 1단계. 머티리얼 JSON 수동 추가가 아니라 생성 파이프라인 확장

### 기존 제안의 문제

기존 문서에서는 `MI_ENV_KNVLLG02_MetalRoof_BlueMarine.matinst.json` 같은 결과 파일에
`ambient_factor`, `ambient_occlusion_strength`를 직접 추가하는 방향으로 설명했습니다.

하지만 현재 작업 방식에서는 이 파일이 아래 배치 실행 때 다시 생성됩니다.

```bat
D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\MapTools\RunBuildKonohaVillage02AndLevel_fixed.bat
```

그래서 실제로는 이 방향이 맞습니다.

1. 결과물인 `*.matinst.json`을 손대는 게 아니라,
2. `ResolveFModelMaterials.py`가 `*.props.txt`를 읽어 `*.matinst.json`을 만들 때
3. ambient 관련 필드를 함께 생성하게 바꿔야 합니다.

### 현재 원본 `props.txt`에서 확인된 사실

제가 실제 원본 파일을 확인해보니:

1. `MI_ENV_KNVLLG02_MetalRoof_BlueMarine.props.txt`
2. `M_ENV_KNVLLG02_Mass_Snow.props.txt`
3. `MI_ENV_KNVLLG02_Balloon_A.props.txt`

이 셋 모두에서 현재 resolver가 읽고 있는 대표 값은:

1. `AA_BaseMixColor`
2. `AA_ShadowColor`
3. `AA_NormalMapBoost`
4. `AA_MaskScale`
5. `AC_Mask_Threshold`
6. 텍스처 슬롯들
7. `TextureStreamingData`

정도입니다.

중요한 점은, **지금 확인한 원본 `props.txt` 안에는 `Ambient` 전용 파라미터가 별도로 보이지 않았습니다.**

즉, ambient를 넣는 방법은 둘 중 하나입니다.

1. 원본 `props.txt`/FModel export 안에 실제 ambient 성격 파라미터가 더 있는지 찾아서 resolver가 복원한다.
2. 원본에 그런 값이 없으면 `ResolveFModelMaterials.py` 안에서 "머티리얼별 규칙" 또는 "프로파일별 기본값"으로 생성한다.

현재 확인 결과만 기준으로는 2번 가능성이 더 높습니다.

## [변경] 1단계. 머티리얼 JSON 스키마 확장

### 수정 대상

- `Client/Bin/Resources/Materials/FModel/KonohaVillage02/*.matinst.json`

### 왜 필요한가

수업의 `g_vMtrlAmbient`에 대응하는 값을 현재 프로젝트에서는 머티리얼 인스턴스 JSON이 들고 있어야 합니다.

### 추천 필드

```json
"ambient_factor": [1.0, 1.0, 1.0, 1.0],
"ambient_occlusion_strength": 1.0
```

### 예시 1. `MI_ENV_KNVLLG02_MetalRoof_BlueMarine.matinst.json`

```json
{
  "version": 1,
  "name": "MI_ENV_KNVLLG02_MetalRoof_BlueMarine",
  "profile": "generic_pbr",
  "parent": "M_GenericPBR",
  "base_color_factor": [
    0.28835,
    0.455566,
    0.79,
    1.0
  ],
  "ambient_factor": [
    0.40,
    0.55,
    0.85,
    1.0
  ],
  "ambient_occlusion_strength": 1.0,
  "shadow_color": [
    1.0,
    1.0,
    1.0,
    1.0
  ],
  "normal_strength": 2.0,
  "blend_normal_strength": 1.0,
  "mask_scale": 1.0,
  "mask_threshold": 2.0,
  "uneven_color_scale": 1.0,
  "blend_mode": 0,
  "textures": [
    {
      "slot": "base_color",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG_MetalRoof_BC.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "blend_base_color",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG02_SnowBase02_BC.png",
      "uv_channel": 1,
      "sampling_scale": 5.0
    },
    {
      "slot": "normal",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG_MetalRoof_N.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "blend_normal",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG02_Ground_Snow_N.png",
      "uv_channel": 1,
      "sampling_scale": 5.0
    },
    {
      "slot": "ambient_occlusion",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG_MetalRoof_AO.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "uneven_color",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG_Noize_06_UC.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "mask",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG02_Mask_Snow01_M.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    }
  ],
  "source": {
    "fmodel_mi_json": "MI_ENV_KNVLLG02_MetalRoof_BlueMarine.props.txt"
  }
}
```

### 예시 2. `M_ENV_KNVLLG02_Mass_Snow.matinst.json`

```json
{
  "version": 1,
  "name": "M_ENV_KNVLLG02_Mass_Snow",
  "profile": "generic_pbr",
  "parent": "M_GenericPBR",
  "base_color_factor": [
    1.0,
    1.0,
    1.0,
    1.0
  ],
  "ambient_factor": [
    0.85,
    0.90,
    0.95,
    1.0
  ],
  "ambient_occlusion_strength": 0.75,
  "shadow_color": [
    1.0,
    1.0,
    1.0,
    1.0
  ],
  "normal_strength": 1.0,
  "blend_normal_strength": 1.0,
  "mask_scale": 1.0,
  "mask_threshold": 1.0,
  "uneven_color_scale": 1.5,
  "blend_mode": 0,
  "textures": [
    {
      "slot": "base_color",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG02_SnowBase02_BC.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "ambient_occlusion",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG02_SnowBase02_AO.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    },
    {
      "slot": "uneven_color",
      "index": 0,
      "path": "../../../Textures/FModel/KonohaVillage02/T_ENV_KNVLLG_PaperBase_01_BC.png",
      "uv_channel": 0,
      "sampling_scale": 1.0
    }
  ],
  "source": {
    "fmodel_mi_json": "M_ENV_KNVLLG02_Mass_Snow.props.txt"
  }
}
```

## [변경] 2단계. `ModelMaterial`이 ambient 데이터를 읽도록 확장

### 수정 대상

- `Engine/Public/ModelMaterial.h`
- `Engine/Private/ModelMaterial.cpp`

### 수정 이유

머티리얼 JSON에 넣은 `ambient_factor`, `ambient_occlusion_strength`를 런타임에서 꺼내서 셰이더로 보낼 수 있어야 합니다.

### `Engine/Public/ModelMaterial.h`

### [변경] 이 단계의 전제 수정

이 단계 자체는 맞습니다.

다만 입력 원천은:

1. 사용자가 `matinst.json`을 손으로 쓰는 방식이 아니라,
2. `ResolveFModelMaterials.py`가 생성한 `matinst.json`

이어야 합니다.

즉, `ModelMaterial` 확장은 유지하되, 그 앞단에서 `ResolveFModelMaterials.py`가
`ambient_factor`, `ambient_occlusion_strength`를 생성하게 만드는 것이 먼저입니다.

```cpp
#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL ModelMaterial : public Base
{
    GENERATED_BODY(ModelMaterial)

private:
    struct FMaterialTextureMeta
    {
        // [추가] 이 텍스처가 어느 UV 채널을 써야 하는지 나타내는 인덱스다.
        uint32 uvChannel = 0;
        // [추가] 원본 머티리얼의 sampling scale을 복원한 값이다.
        float samplingScale = 1.f;
    };

public:
    explicit ModelMaterial(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~ModelMaterial() = default;

public:
    HRESULT Initialize_FromJson(const json& data, const string& materialFilePath);
    HRESULT Initialize_FromMaterialInstance(const string& matInstanceFilePath);
    HRESULT Bind_Material(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

    string  Get_MaterialName() const { return _materialName; }
    uint32  Get_TextureCount(EMaterialTextureSlot slot) const;
    string  Get_TextureGuid(EMaterialTextureSlot slot, uint32 index) const;
    // [추가] 셰이더가 texture별로 올바른 UV 세트를 고를 수 있도록 UV 채널 메타를 제공한다.
    uint32  Get_TextureUVChannel(EMaterialTextureSlot slot, uint32 index) const;
    // [추가] 셰이더가 원본 타일링을 복원할 수 있도록 texture별 sampling scale을 제공한다.
    float   Get_TextureSamplingScale(EMaterialTextureSlot slot, uint32 index) const;

    HRESULT Override_Texture(EMaterialTextureSlot slot, uint32 index, const string& guid);

public:
    const Vec4&     Get_BaseColorFactor() const { return _baseColorFactor; }
    // [추가] Ambient 재질 색상 곱이다. Deferred Light Pass에서 Light Ambient와 곱해진다.
    const Vec4&     Get_AmbientFactor() const { return _ambientFactor; }
    // [추가] AO 텍스처의 적용 강도다. 0이면 무시, 1이면 원본 AO를 그대로 쓴다.
    float           Get_AmbientOcclusionStrength() const { return _ambientOcclusionStrength; }
    float           Get_NormalStrength() const { return _normalStrength; }
    const string&   Get_MaterialProfile() const { return _materialProfile; }

    static Vec4 Read_Vec4_Array(const json& data, const char* key, const Vec4& defaultValue);

public: /* 머티리얼 종류 추가 */
    float           Get_BlendNormalStrength() const { return _blendNormalStrength; }
    float           Get_MaskScale() const { return _maskScale; }
    float           Get_MaskThreshold() const { return _maskThreshold; }
    float           Get_UnevenColorScale() const { return _unevenColorScale; }
    Vec4            Get_ShadowColor() const { return _shadowColor; }

public:
    json    To_Json() const;
    void    From_Json(const json& data);

private:
    static EMaterialTextureSlot SlotString_To_Enum(const string& slot);
    static string SlotEnum_To_String(EMaterialTextureSlot slot);

    // [추가] slot/index 조합에 대응하는 texture 메타 배열 크기를 미리 맞춘다.
    void    Ensure_TextureMetaStorage(EMaterialTextureSlot slot, uint32 index);
    // [추가] JSON에서 읽은 UV 채널과 sampling scale을 slot/index 기준으로 저장한다.
    void    Set_TextureMeta(EMaterialTextureSlot slot, uint32 index, uint32 uvChannel, float samplingScale);
    HRESULT Load_Texture_File(EMaterialTextureSlot slot, uint32 index, const string& texturePath, const string& baseFilePath);
    HRESULT Bind_Texture_Internal(Shared<Shader> shader, const char* constantName, EMaterialTextureSlot slot, uint32 textureIndex);

private:
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

    vector<ComPtr<ShaderResourceView>> _textures[MATERIAL_TEXTURE_SLOT_COUNT];
    vector<string>                     _textureGuids[MATERIAL_TEXTURE_SLOT_COUNT];
    // [추가] 각 텍스처 슬롯/인덱스가 사용하는 UV 채널과 타일링 메타를 저장한다.
    vector<FMaterialTextureMeta>       _textureMetas[MATERIAL_TEXTURE_SLOT_COUNT];

    string                             _materialName;
    string                             _materialInstanceGuid;

private:
    // Mateiral Instnace
    Vec4    _baseColorFactor = Vec4(1.f, 1.f, 1.f, 1.f);
    // [추가] Material Ambient 전용 곱셈 색상이다.
    Vec4    _ambientFactor = Vec4(1.f, 1.f, 1.f, 1.f);
    // [추가] AO 텍스처가 있을 때 ambient를 얼마나 눌러줄지 결정하는 강도다.
    float   _ambientOcclusionStrength = 1.f;
    Vec4    _shadowColor = Vec4(1.f, 1.f, 1.f, 1.f);
    float   _normalStrength = 1.f;
    string  _materialProfile;
    int32   _blendMode = 0;

    float   _blendNormalStrength = 1.f;
    float   _maskScale = 1.f;
    float   _maskThreshold = 1.f;
    float   _unevenColorScale = 1.f;

public:
    static Shared<ModelMaterial> Create(ComPtr<Device> device, ComPtr<DeviceContext> context,
        const json& data, const string& materialFilePath);

    void Free() override;
};

NS_END
```

### `Engine/Private/ModelMaterial.cpp`

`Initialize_FromJson()` 내부에 아래 읽기 코드를 추가하면 됩니다.

```cpp
HRESULT ModelMaterial::Initialize_FromJson(const json& data, const string& materialFilePath)
{
    {
        _baseColorFactor = Read_Vec4_Array(data, "base_color_factor", Vec4(1.f, 1.f, 1.f, 1.f));
        // [추가] Deferred Light Pass에서 사용할 재질 ambient 색상이다.
        _ambientFactor = Read_Vec4_Array(data, "ambient_factor", Vec4(1.f, 1.f, 1.f, 1.f));
        // [추가] ambient occlusion texture가 있을 때 적용 강도를 조절한다.
        _ambientOcclusionStrength = data.value("ambient_occlusion_strength", 1.f);
        _shadowColor = Read_Vec4_Array(data, "shadow_color", Vec4(1.f, 1.f, 1.f, 1.f));

        _normalStrength = data.value("normal_strength", 1.f);

        _blendNormalStrength = data.value("blend_normal_strength", 1.f);

        _maskScale = data.value("mask_scale", 1.f);
        _maskThreshold = data.value("mask_threshold", 1.f);
        _unevenColorScale = data.value("uneven_color_scale", 1.f);

        _materialProfile = data.value("profile", string(""));
        _blendMode = data.value("blend_mode", 0);
    }

    _materialName = data.value("material_name", data.value("name", string("Material")));

    if (!data.contains("textures") || !data["textures"].is_array())
        return S_OK;

    for (const auto& textureItem : data["textures"])
    {
        if (!textureItem.is_object())
            continue;

        string slotStr = textureItem.value("slot", "");
        uint32 index = textureItem.value("index", 0u);
        string path = textureItem.value("path", "");
        // [추가] texture item이 참조하는 UV 채널 인덱스다.
        uint32 uvChannel = textureItem.value("uv_channel", 0u);
        // [추가] texture item의 원본 sampling scale이다.
        float samplingScale = textureItem.value("sampling_scale", 1.f);

        if (path.empty())
            continue;

        EMaterialTextureSlot slot = SlotString_To_Enum(slotStr);
        if (slot == EMaterialTextureSlot::END)
            continue;

        Set_TextureMeta(slot, index, uvChannel, samplingScale);
        CHECK_FAILED(Load_Texture_File(slot, index, path, materialFilePath), E_FAIL);
    }

    return S_OK;
}
```

## [변경] 3단계. `StaticMeshActor`가 ambient / AO를 셰이더로 보내도록 확장

### 수정 대상

- `Client/Private/StaticMeshActor.cpp`

### 수정 이유

지금 `StaticMeshActor`는 base color / mask / blend 정보만 셰이더에 넘깁니다.
ambient와 AO도 같이 넘겨야 `Shader_VtxStaticMesh.hlsl`가 GBuffer에 기록할 수 있습니다.

### 핵심 추가 코드

`Render()` 내부 mesh loop에 아래 항목을 추가합니다.

```cpp
Vec4 materialAmbient = Vec4(1.f, 1.f, 1.f, 1.f);
float ambientOcclusionStrength = 1.f;

int hasAOTexture = 0;
int aoUVChannel = 0;
float aoUVScale = 1.f;

if (material)
{
    baseColorFactor = material->Get_BaseColorFactor();
    materialAmbient = material->Get_AmbientFactor();
    ambientOcclusionStrength = material->Get_AmbientOcclusionStrength();
    shadowColor = material->Get_ShadowColor();

    hasDiffuseTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::BaseColor) > 0) ? 1 : 0;

    hasBlendDiffuseTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::BlendBaseColor) > 0) ? 1 : 0;

    hasMaskTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::Mask) > 0) ? 1 : 0;

    hasBlendNormalTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::BlendNormal) > 0) ? 1 : 0;

    hasUnevenColorTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::UnevenColor) > 0) ? 1 : 0;

    hasAOTexture =
        (material->Get_TextureCount(EMaterialTextureSlot::AmbientOcclusion) > 0) ? 1 : 0;

    maskScale = material->Get_MaskScale();
    maskThreshold = material->Get_MaskThreshold();
    blendNormalStrength = material->Get_BlendNormalStrength();
    unevenColorScale = material->Get_UnevenColorScale();

    baseColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BaseColor, 0));
    blendDiffuseUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BlendBaseColor, 0));
    maskUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::Mask, 0));
    blendNormalUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::BlendNormal, 0));
    unevenColorUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::UnevenColor, 0));
    aoUVChannel = static_cast<int>(material->Get_TextureUVChannel(EMaterialTextureSlot::AmbientOcclusion, 0));

    baseColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BaseColor, 0);
    blendDiffuseUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BlendBaseColor, 0);
    maskUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::Mask, 0);
    blendNormalUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::BlendNormal, 0);
    unevenColorUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::UnevenColor, 0);
    aoUVScale = material->Get_TextureSamplingScale(EMaterialTextureSlot::AmbientOcclusion, 0);
}

CHECK_FAILED(_shaderCom->Bind_RawValue("g_MtrlAmbient", &materialAmbient, sizeof(Vec4)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_AmbientOcclusionStrength", &ambientOcclusionStrength, sizeof(float)), E_FAIL);

CHECK_FAILED(_shaderCom->Bind_RawValue("g_HasAOTexture", &hasAOTexture, sizeof(int)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_AOUVChannel", &aoUVChannel, sizeof(int)), E_FAIL);
CHECK_FAILED(_shaderCom->Bind_RawValue("g_AOUVScale", &aoUVScale, sizeof(float)), E_FAIL);
```

AO 텍스처 바인딩도 추가합니다.

```cpp
if (hasAOTexture != 0)
{
    CHECK_FAILED(
        _modelCom->Bind_Material(_shaderCom, "g_AmbientOcclusionTexture",
            static_cast<uint32>(i), EMaterialTextureSlot::AmbientOcclusion, 0), E_FAIL);
}
else
{
    CHECK_FAILED(_shaderCom->Bind_SRV("g_AmbientOcclusionTexture", nullptr), E_FAIL);
}
```

## [변경] 4단계. Static Mesh GBuffer에 Ambient/AO 저장

### 수정 대상

- `Client/Bin/Shaders/Shader_VtxStaticMesh.hlsl`

### 수정 이유

현재 GBuffer는 `Diffuse`, `Normal`만 저장합니다.
Material Ambient는 Light Pass 때 픽셀 단위로 필요하므로 GBuffer에 같이 저장해야 합니다.

### 추천 코드

아래처럼 `Target_Material`에 ambient rgb + ao scalar를 저장하는 형태가 가장 안정적입니다.

```hlsl
#include "Engine_Shader_Defines.hlsli"

float4 g_BaseColorFactor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_ShadowColor = float4(1.f, 1.f, 1.f, 1.f);
float4 g_MtrlAmbient = float4(1.f, 1.f, 1.f, 1.f);

int g_HasDiffuseTexture = 1;
int g_HasBlendDiffuseTexture = 0;
int g_HasMaskTexture = 0;
int g_HasBlendNormalTexture = 0;
int g_HasUnevenColorTexture = 0;
int g_HasAOTexture = 0;

int g_BaseColorUVChannel = 0;
int g_BlendDiffuseUVChannel = 0;
int g_MaskUVChannel = 0;
int g_BlendNormalUVChannel = 0;
int g_UnevenColorUVChannel = 0;
int g_AOUVChannel = 0;

float g_BaseColorUVScale = 1.f;
float g_BlendDiffuseUVScale = 1.f;
float g_MaskUVScale = 1.f;
float g_BlendNormalUVScale = 1.f;
float g_UnevenColorUVScale = 1.f;
float g_AOUVScale = 1.f;

float g_MaskScale = 1.f;
float g_MaskThreshold = 1.f;
float g_BlendNormalStrength = 1.f;
float g_UnevenColorScale = 1.f;
float g_AmbientOcclusionStrength = 1.f;

float4 g_OutlineColor = float4(0.1f, 1.f, 0.1f, 1.f);
float g_OutlineThickness = 0.0035f;
int g_IsOutlineEnabled = 0;

Texture2D g_BlendDiffuseTexture;
Texture2D g_BlendNormalTexture;
Texture2D g_UnevenColorTexture;
Texture2D g_AmbientOcclusionTexture;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vWorldPos : TEXCOORD2;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;

    float4x4 matWV, matWVP;

    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vTexcoord1 = In.vTexcoord1;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), g_WorldMatrix);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float2 vTexcoord : TEXCOORD0;
    float2 vTexcoord1 : TEXCOORD1;
    float4 vWorldPos : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal  : SV_TARGET1;
    vector vMaterial : SV_TARGET2;
};

float2 SelectMaterialUV(float2 uv0, float2 uv1, int uvChannel, float uvScale)
{
    float2 selectedUV = (uvChannel == 1) ? uv1 : uv0;
    return selectedUV * uvScale;
}

vector ComputeLayeredBaseColor(float2 uv0, float2 uv1);
float ComputeAmbientOcclusion(float2 uv0, float2 uv1);

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;

    vector mtrlDiffuse = ComputeLayeredBaseColor(In.vTexcoord, In.vTexcoord1);
    float aoValue = ComputeAmbientOcclusion(In.vTexcoord, In.vTexcoord1);
    float3 ambientColor = saturate(mtrlDiffuse.rgb * g_MtrlAmbient.rgb);

    Out.vDiffuse = vector(mtrlDiffuse.rgb, 1.f);
    Out.vNormal = vector(normalize(In.vNormal.xyz) * 0.5f + 0.5f, 0.f);
    Out.vMaterial = vector(ambientColor, aoValue);

    return Out;
}
```

## [변경] 5단계. Renderer에 Material GBuffer 추가

### 수정 대상

- `Engine/Private/Renderer.cpp`

### 수정 이유

Light Pass에서 재질 ambient를 읽으려면 `Target_Material`을 MRT에 추가해야 합니다.

### `Ready_RenderTarget()` 수정 코드

```cpp
CHECK_FAILED(GAME->Add_RenderTarget(L"Target_Material",
    width, height, DXGI_FORMAT_R16G16B16A16_UNORM, Color(1.f, 1.f, 1.f, 1.f)), E_FAIL);

CHECK_FAILED(GAME->Add_MRT(L"MRT_GameObjects", L"Target_Material"), E_FAIL);
```

### `Render_Lights()` 수정 코드

```cpp
if (canRender &&
    FAILED(GAME->Bind_RT_ShaderResource(_deferredShader, "g_MaterialTexture", L"Target_Material")))
{
    canRender = false;
}
```

## [변경] 6단계. 실제 Light Ambient를 셰이더에 바인딩

### 수정 대상

- `Engine/Private/Light.cpp`

### 수정 이유

현재 프로젝트는 `FLightDesc.ambient` 값을 만들지만 실제 셰이더에 보내지 않습니다.
이 단계가 빠지면 수업 내용의 핵심이 반영되지 않습니다.

### 추천 코드

```cpp
if (FAILED(shader->Bind_RawValue("g_LightDiffuse", &_lightDesc.diffuse, sizeof _lightDesc.diffuse)))
    return E_FAIL;
if (FAILED(shader->Bind_RawValue("g_LightAmbient", &_lightDesc.ambient, sizeof _lightDesc.ambient)))
    return E_FAIL;
if (FAILED(shader->Bind_RawValue("g_LightSpecular", &_lightDesc.specular, sizeof _lightDesc.specular)))
    return E_FAIL;
```

## [변경] 7단계. Deferred Light Pass에서 Ambient 계산 적용

### 수정 대상

- `Client/Bin/Shaders/Shader_Deferred.hlsl`

### 수정 이유

여기가 오늘 수업 내용을 현재 프로젝트에 맞게 최종 적용하는 핵심입니다.

### 추천 코드

```hlsl
Texture2D g_NormalTexture;
Texture2D g_MaterialTexture;

PS_OUT_LIGHT PS_MAIN_DIRECTIONAL(PS_IN In)
{
    PS_OUT_LIGHT Out;

    vector vNormalDesc = g_NormalTexture.Sample(DefaultSampler, In.vTexcoord);
    vector vMaterialDesc = g_MaterialTexture.Sample(DefaultSampler, In.vTexcoord);

    vector vNormal = vector(vNormalDesc.xyz * 2.f - 1.f, 0.f);
    float3 materialAmbient = saturate(vMaterialDesc.rgb);
    float aoValue = saturate(vMaterialDesc.a);

    float ndotl = max(dot(normalize(g_LightDir.xyz) * -1.f, normalize(vNormal.xyz)), 0.f);

    float3 diffuseTerm = g_LightDiffuse.rgb * ndotl;
    float3 ambientTerm = g_LightAmbient.rgb * materialAmbient * aoValue;

    Out.vShade = vector(diffuseTerm + ambientTerm, 1.f);

    return Out;
}
```

## [변경] 8단계. Konoha 레벨의 ambient 세기 조정

### 수정 대상

- `Client/Private/Level_Konoha.cpp`
- 필요 시 `Client/Private/Level_Gameplay.cpp`

### 수정 이유

지금 Konoha 레벨은 ambient가 `1,1,1,1`이라 너무 강합니다.
ambient 재질 표현을 붙이면 오히려 전체가 평평하게 밝아질 수 있습니다.

### 추천 값

```cpp
HRESULT Level_Konoha::Ready_Lights()
{
    FLightDesc lightDesc{};

    lightDesc.type = ELightType::Directional;
    lightDesc.direction = Vec4(1.f, -1.f, 1.f, 0.f);
    lightDesc.diffuse = Vec4(1.f, 1.f, 1.f, 1.f);
    // [변경] Material Ambient가 살아나도록 레벨 ambient는 과하지 않게 잡는다.
    lightDesc.ambient = Vec4(0.30f, 0.32f, 0.36f, 1.f);
    lightDesc.specular = Vec4(1.f, 1.f, 1.f, 1.f);

    CHECK_FAILED(GAME->Add_Light(lightDesc), E_FAIL);

    return S_OK;
}
```

## [추가] 9단계. Skeletal Mesh까지 확장하고 싶을 때

### 수정 대상

- `Client/Bin/Shaders/Shader_VtxAnimMesh.hlsl`
- `Client/Private/Monster.cpp`
- `Client/Private/Player_CustomPart.cpp`
- `Client/Private/Weapon.cpp`

### 현재 상태

현재 이 경로는 `g_DiffuseTexture`만 바인딩합니다.
즉, 캐릭터/무기는 환경 정적 메시만큼 정교한 ambient material을 아직 못 씁니다.

### 추천 방침

이번 작업 1차에서는 **정적 환경 메시만 먼저 적용**하세요.

그 이유는:

1. 오늘 수업 주제가 맵/재질 ambient 표현에 더 가깝고,
2. 현재 열려 있는 파일도 `KonohaVillage02` 환경 재질 기준이며,
3. 캐릭터 쪽은 normal/specular/skin shading 방향까지 같이 건드리게 될 가능성이 커서 범위가 커집니다.

즉, **1차 완료 후 2차로 분리**하는 게 맞습니다.

## 6. 실제 적용 순서

아래 순서대로 가면 충돌이 적습니다.

1. `.matinst.json`에 `ambient_factor`, `ambient_occlusion_strength`를 추가합니다.
2. `ModelMaterial.h/.cpp`에 ambient 데이터 파싱/게터를 추가합니다.
3. `StaticMeshActor.cpp`가 ambient/AO를 셰이더로 넘기게 합니다.
4. `Renderer.cpp`에 `Target_Material` MRT를 추가합니다.
5. `Shader_VtxStaticMesh.hlsl`가 ambient/AO를 GBuffer에 저장하게 합니다.
6. `Light.cpp`에서 `g_LightAmbient`를 바인딩합니다.
7. `Shader_Deferred.hlsl`에서 ambient 계산을 넣습니다.
8. `Level_Konoha.cpp` ambient 값을 재조정합니다.
9. 디버그 RT로 `Target_Material`을 확인합니다.
10. 마지막으로 머티리얼별 `ambient_factor`를 튜닝합니다.

## 7. 디버깅 체크 포인트

이 작업은 아래 순서로 확인하면 빠릅니다.

1. `Target_Material` 디버그 화면에서 RGB가 머티리얼마다 다르게 찍히는지 확인
2. `Target_Material.a`가 AO 텍스처가 있는 곳에서 1보다 낮게 들어오는지 확인
3. `lightDesc.ambient` 값을 `0.0`으로 두었을 때 ambient 효과가 완전히 사라지는지 확인
4. `ambient_factor`를 파랑/빨강으로 바꿨을 때 그림자 쪽 색감이 바뀌는지 확인
5. `ambient_occlusion_strength = 0.0`일 때 AO 영향이 사라지는지 확인

## 8. 가장 중요한 주의점

이번 이식에서 제일 중요한 건 아래 한 줄입니다.

**수업 코드의 `g_vMtrlAmbient` 상수 하나를 현재 프로젝트의 Deferred Pass에 바로 복붙하면 안 됩니다.**

왜냐하면 현재 프로젝트는 화면 전체가 아니라 **픽셀마다 다른 재질 ambient 값**이 필요하기 때문입니다.

그래서 현재 레포에서는:

1. `상수 1개 추가`가 아니라,
2. `GBuffer 1장 추가`

가 사실상 정답입니다.

## 9. 추천 마감 형태

이번 작업을 실전적으로 마감하려면 아래 상태를 목표로 잡으면 됩니다.

1. `KonohaVillage02` 환경 메시만 ambient material 적용 완료
2. 대표 재질 3개만 먼저 튜닝
   - `MetalRoof`
   - `Mass_Snow`
   - `Balloon`
3. `Target_Material` 디버그 확인 가능
4. Konoha 레벨 ambient 세기 재조정 완료

이렇게 끝내면 다음 단계에서:

1. normal map 강화
2. specular 분리
3. skeletal mesh ambient 확장

으로 자연스럽게 넘어갈 수 있습니다.
