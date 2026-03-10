# FModel LobbyMap JSON -> `.level.json` 변환 가이드

## 목표

이 문서는 FModel이 추출한 언리얼 배치 데이터를 현재 `Dx11_Naruto` 프로젝트에서 사용할 수 있는 형태로 옮기기 위한 가이드다.

대상 케이스:

- 원본 배치 파일:
  `C:\Users\moon\Desktop\FModel\Output\Exports\NARUTO\Content\Maps\LobbyMaps\KonohaVillage_BORUTO\LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.json`
- 원본 추출 에셋:
  `C:\Users\moon\Desktop\Temp_Naruto\Assets\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO`

이 작업의 목표는 `.umap`을 직접 읽는 것이 아니다.
실제 목표는 아래 4단계다.

1. 원본 메시를 이 프로젝트 런타임 포맷으로 변환한다.
2. 변환된 런타임 메시를 이 프로젝트의 에셋 시스템에 등록한다.
3. FModel 배치 JSON을 이 프로젝트의 `.level.json`으로 변환한다.
4. 기존 레벨 로더가 `StaticMeshActor`를 정상적으로 스폰하도록 연결한다.

이 문서는 현재 코드베이스 기준으로 작성했다.

---

## 현재 프로젝트의 제약 사항

### 1. 런타임 모델 로더는 `.gltf`를 직접 받지 않는다

`Model::Initialize_Prototype(...)`는 현재 `.meshbin`만 허용한다.

관련 코드:

- `Engine/Private/Model.cpp`
- `Client/Private/StaticMeshActor.cpp`

핵심 포인트:

- `Model runtime only supports .meshbin`
- `StaticMeshActor`는 `model_guid -> 실제 파일 경로` 방식으로 동작한다

즉:

- `gltf`만 복사해서는 안 된다.
- 반드시 아래 두 파일이 필요하다.
  - `*.meshbin`
  - `*.material.json`

### 2. 에셋 조회는 GUID 기반이다

`StaticMeshActor`는 파일 경로를 직접 저장하지 않는다.
`model_guid`를 저장한 뒤 `Asset_Manager`를 통해 실제 경로를 찾는다.

관련 코드:

- `Engine/Private/GameInstance.cpp`
- `Engine/Private/Asset_Manager.cpp`
- `Client/Private/StaticMeshActor.cpp`

### 3. 현재 레벨 로더는 `StaticMeshActor`를 올바르게 초기화하지 못한다

`StaticMeshActor::Initialize(void* arg)`는 `modelGuid`가 들어 있는 `FStaticMeshDesc`를 요구한다.
하지만 현재 `Level::Load_LevelFromJson(...)`는 `nullptr`로 clone한다.

관련 코드:

- `Engine/Private/Level.cpp`
- `Editor/Private/Level_Serializer.cpp`
- `Client/Private/StaticMeshActor.cpp`

이 부분이 현재 런타임 기준 가장 큰 장애물이다.

### 4. 에셋 타입 판별에 `.meshbin`이 아직 없다

`Asset_Manager::Detect_AssetType(...)`는 현재 아래 확장자만 `"model"`로 취급한다.

- `.fbx`
- `.obj`
- `.psk`
- `.gltf`

`.meshbin`은 아직 포함되지 않는다.

GUID 조회 자체가 완전히 막히는 문제는 아니지만, 이 프로젝트 기준으로는 `"model"`로 정상 분류되도록 고치는 편이 맞다.

---

## 현재까지 확인된 내용

`LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.json` 기준:

- `StaticMeshActor` 개수: `84`
- 고유 `StaticMesh` 참조 수: `17`

그리고 이 17개 메시가 모두 아래 경로에 실제로 존재하는 것을 확인했다.

`C:\Users\moon\Desktop\Temp_Naruto\Assets\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Meshes`

필요한 메시 basename 목록:

```text
SM_ENV_LKNVLD_CommonBuilding_A
SM_ENV_LKNVLD_CommonBuilding_A_Tall
SM_ENV_LKNVLD_CommonBuilding_A5
SM_ENV_LKNVLD_CommonBuilding_B
SM_ENV_LKNVLD_CommonBuilding_C
SM_ENV_LKNVLD_CommonBuilding_D
SM_ENV_LKNVLD_CommonBuilding_F
SM_ENV_LKNVLD_CommonBuilding_TowerA
SM_ENV_LKNVLD_CommonBuilding_TowerB
SM_ENV_LKNVLD_FaceRock_A
SM_ENV_LKNVLD_FaceRock_BG_01
SM_ENV_LKNVLD_FaceRock_BG_02
SM_ENV_LKNVLD_FaceRock_BG_A_03
SM_ENV_LKNVLD_FaceRock_BG_A_04
SM_ENV_LKNVLD_FaceRock_BG_B_03
SM_ENV_LKNVLD_FaceRock_BG_B_04
SM_ENV_LKNVLD_FarTree_A
```

즉, 이 맵은 실제로 작업 가능한 상태다.

---

## 권장 작업 순서

순서는 아래대로 가는 것이 가장 안전하다.

1. 추출된 원본 에셋 폴더를 `Client/Bin/Resources` 아래로 복사한다.
2. 필요한 `.gltf`를 일괄 변환해서 `.meshbin + .material.json`을 만든다.
3. `.meshbin`이 `"model"`로 잡히도록 에셋 타입 판별을 보강한다.
4. 한 번 실행해서 `Asset_Manager`가 GUID를 생성하게 만든다.
5. FModel JSON을 `.level.json`으로 변환한다.
6. `OBJECT_TYPE_STATIC_MESH`가 `FStaticMeshDesc`를 받도록 레벨 로더를 수정한다.
7. 생성된 레벨을 실제로 로드한다.

5번과 6번 순서를 바꾸면 `.level.json`은 있어도 실제로는 메시가 뜨지 않는 상태가 나온다.

---

## 권장 디렉터리 구조

런타임에서 `C:\Users\moon\Desktop\Temp_Naruto\...`를 직접 참조하지 않는 것이 맞다.
실제 실행용 에셋은 전부 프로젝트 리소스 루트 아래에 두는 편이 좋다.

- 리소스 루트:
  `../../Client/Bin/Resources`

권장 목적지:

```text
Client/Bin/Resources/Imported/Naruto/LobbyMapAssets/LM_KonohaVillage_BORUTO/
├─ MeshesSrc/
├─ MeshesRuntime/
├─ Materials/
└─ Textures/
```

의미:

- `MeshesSrc/`: 원본 `.gltf` + `.bin`
- `MeshesRuntime/`: 변환된 `.meshbin` + `.material.json`
- `Materials/`, `Textures/`: 원본 사이드카 파일

이 구조로 두면 나중에 재변환할 때도 흐름이 명확하다.

---

## 1단계. 원본 에셋 복사

복사 대상 폴더:

- `Meshes`
- `Materials`
- `Textures`

권장 PowerShell:

```powershell
$src = "C:\Users\moon\Desktop\Temp_Naruto\Assets\Game\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO"
$dst = "C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Imported\Naruto\LobbyMapAssets\LM_KonohaVillage_BORUTO"

New-Item -ItemType Directory -Force -Path "$dst\MeshesSrc" | Out-Null
New-Item -ItemType Directory -Force -Path "$dst\MeshesRuntime" | Out-Null
New-Item -ItemType Directory -Force -Path "$dst\Materials" | Out-Null
New-Item -ItemType Directory -Force -Path "$dst\Textures" | Out-Null

Copy-Item "$src\Meshes\*" "$dst\MeshesSrc" -Recurse -Force
Copy-Item "$src\Materials\*" "$dst\Materials" -Recurse -Force
Copy-Item "$src\Textures\*" "$dst\Textures" -Recurse -Force
```

먼저 복사하는 이유:

- `AssimpTool`이 생성하는 `material.json`은 보통 모델 기준 상대 텍스처 경로를 쓴다
- 전부 `Client/Bin/Resources` 아래에 있으면 GUID 등록과 런타임 경로 해석이 단순해진다

---

## 2단계. `.gltf`를 `.meshbin + .material.json`으로 변환

### 현재 상태

이 저장소에는 이미 변환 도구가 있다.

- `AssimpTool/Private/Converter.cpp`
- `AssimpTool/Public/Converter.h`
- `AssimpTool/Bin/AssimpTool.exe`

즉, 모델 변환기를 처음부터 새로 만들 필요는 없다.

### 현재 `AssimpTool`의 문제

`AssimpTool/Default/AssimpTool.cpp`는 지금 특정 스켈레탈 메시 2개만 하드코딩해서 변환한다.
맵 에셋 작업에는 폴더를 받아서 static mesh를 일괄 변환하는 방식이 더 맞다.

### 권장 `AssimpTool/Default/AssimpTool.cpp` 수정안

현재 `main()`을 아래 형태로 교체하는 것을 권장한다.

```cpp
#include "pch.h"
#include "Converter.h"

namespace fs = std::filesystem;

static bool IsTargetMesh(const fs::path& path)
{
    if (!path.has_extension())
        return false;

    string ext = path.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".gltf")
        return false;

    // 1차 작업에서는 충돌 메시와 하위 LOD를 제외한다.
    const string name = path.stem().string();
    if (name.starts_with("COL_"))
        return false;
    if (name.find("_Lod") != string::npos)
        return false;

    return true;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3)
    {
        wcout << L"Usage: AssimpTool.exe <src_dir> <dst_dir>" << endl;
        return 1;
    }

    fs::path srcDir = fs::absolute(argv[1]);
    fs::path dstDir = fs::absolute(argv[2]);

    if (!fs::exists(srcDir))
    {
        wcerr << L"Source directory not found: " << srcDir << endl;
        return 1;
    }

    auto converter = Assimp::Converter::Create();
    int successCount = 0;
    int failCount = 0;

    for (const auto& entry : fs::directory_iterator(srcDir))
    {
        if (!entry.is_regular_file())
            continue;

        const fs::path srcPath = entry.path();
        if (!IsTargetMesh(srcPath))
            continue;

        const fs::path dstBase = dstDir / srcPath.stem();

        const bool ok = converter->Convert(
            srcPath.wstring(),
            dstBase.wstring(),
            Assimp::EConvertModelType::StaticMesh);

        if (ok)
            ++successCount;
        else
            ++failCount;
    }

    wcout << L"Convert finished. success=" << successCount
          << L", fail=" << failCount << endl;

    return (failCount == 0) ? 0 : 2;
}
```

### 실행 예시

`AssimpTool` 빌드 후:

```powershell
.\AssimpTool\Bin\AssimpTool.exe `
  "C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Imported\Naruto\LobbyMapAssets\LM_KonohaVillage_BORUTO\MeshesSrc" `
  "C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Imported\Naruto\LobbyMapAssets\LM_KonohaVillage_BORUTO\MeshesRuntime"
```

예상 출력:

```text
MeshesRuntime/SM_ENV_LKNVLD_CommonBuilding_A.meshbin
MeshesRuntime/SM_ENV_LKNVLD_CommonBuilding_A.material.json
MeshesRuntime/SM_ENV_LKNVLD_CommonBuilding_A_Tall.meshbin
MeshesRuntime/SM_ENV_LKNVLD_CommonBuilding_A_Tall.material.json
...
```

### 여기서 반드시 확인할 것

변환된 파일을 몇 개 직접 체크해야 한다.

- `.meshbin` 생성 여부
- 짝이 되는 `.material.json` 생성 여부
- `.material.json` 안의 텍스처 경로가 실제로 유효한지

현재 converter는 텍스처 경로를 모델 파일 기준 상대 경로로 쓴다.
따라서 런타임 메시 폴더와 텍스처 폴더 위치가 너무 멀어지면 경로가 틀어질 수 있다.

---

## 3단계. `.meshbin`을 `"model"`로 인식시키기

### 이유

GUID 조회만 된다 해도, 에셋 시스템은 `.meshbin`을 모델로 인식하는 것이 맞다.

### 파일

- `Engine/Private/Asset_Manager.cpp`

### 권장 수정

현재 코드:

```cpp
if (extension == ".fbx" || extension == ".obj" || extension == ".psk" || extension == ".gltf")
    return "model";
```

아래처럼 변경:

```cpp
if (extension == ".fbx" || extension == ".obj" || extension == ".psk" ||
    extension == ".gltf" || extension == ".meshbin")
    return "model";
```

작은 수정이지만 이 프로젝트 기준으로는 맞는 방향이다.

---

## 4단계. 런타임 메시 에셋 등록 후 GUID 얻기

### 가장 쉬운 방법

1. 변환된 파일을 `Client/Bin/Resources/...` 아래 둔다
2. 에디터나 게임을 한 번 실행한다
3. `Asset_Manager`가 리소스 루트를 스캔한다
4. 생성된 GUID를 `Client/Bin/Resources/.asset_cache.json`에서 읽는다

에셋 매니저는 여기서 초기화된다.

- `Engine/Private/GameInstance.cpp`

```cpp
_assetManager = Asset_Manager::Create(TEXT("../../Client/Bin/Resources"));
```

### 필요하면 강제 스캔 함수 추가

```cpp
void RegisterImportedLobbyAssets()
{
    GAME->Scan_Assets(TEXT("../../Client/Bin/Resources/Imported/Naruto/LobbyMapAssets/LM_KonohaVillage_BORUTO"));
    GAME->Refresh_Cache();
}
```

### `.asset_cache.json`에서 basename -> GUID 매핑 만들기

아래 PowerShell은 런타임 메시에 대한 간단한 lookup 테이블을 만든다.

```powershell
$cachePath = "C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\.asset_cache.json"
$runtimeDir = "Imported\Naruto\LobbyMapAssets\LM_KonohaVillage_BORUTO\MeshesRuntime"
$outPath = "C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\LM_KonohaVillage_BORUTO_mesh_guid_map.json"

$cache = Get-Content -Raw $cachePath | ConvertFrom-Json
$result = @{}

foreach ($prop in $cache.PSObject.Properties) {
    $guid = $prop.Name
    $meta = $prop.Value
    $rel = [string]$meta.relativePath
    if ($rel -like "*$runtimeDir*") {
        $base = [System.IO.Path]::GetFileNameWithoutExtension($rel)
        $result[$base] = $guid
    }
}

$result | ConvertTo-Json -Depth 5 | Set-Content $outPath -Encoding UTF8
```

예상 결과:

```json
{
  "SM_ENV_LKNVLD_CommonBuilding_A": "....guid....",
  "SM_ENV_LKNVLD_CommonBuilding_A_Tall": "....guid....",
  "SM_ENV_LKNVLD_CommonBuilding_A5": "....guid...."
}
```

---

## 5단계. 출력할 `.level.json` 포맷 정의

이 프로젝트에서는 예전 레거시 레벨 파일처럼 `null` 컴포넌트를 섞지 않는 편이 낫다.
처음부터 깔끔한 포맷으로 생성하는 것을 권장한다.

권장 오브젝트 포맷:

```json
{
  "levelName": "KonohaVillage_BORUTO_BackdropBuildings",
  "levelIndex": 2,
  "gameObjects": [
    {
      "static_class": "SM_ENV_LKNVLD_CommonBuilding_A11",
      "object_type": "OBJECT_TYPE_STATIC_MESH",
      "layerTag": "Layer_Backdrop",
      "model_guid": "guid-for-runtime-mesh",
      "components": [
        {
          "type": "COMPONENT_TYPE_TRANSFORM",
          "position": [0.0, 0.0, 0.0],
          "rotation": [0.0, 0.0, 0.0],
          "scale": [1.0, 1.0, 1.0]
        }
      ]
    }
  ]
}
```

권장 규칙:

- `guid` 필드는 생략 가능하다
  - `GameObject` 생성자에서 기본 GUID를 만들기 때문이다
- `components`에는 실제 컴포넌트 객체만 넣는다
- transform에는 항상 `type`을 명시한다
- `model_guid`는 오브젝트 루트에 둔다
  - `StaticMeshActor::From_Json(...)`가 이미 이 위치에서 읽는다

---

## 6단계. FModel JSON을 `.level.json`으로 변환

### 원본 구조

이번 케이스의 FModel export 구조는 아래와 같다.

- `Type == "StaticMeshActor"` 엔트리
- 그에 대응하는 `Type == "StaticMeshComponent"` 엔트리
- 실제 메시 참조와 transform은 component 쪽에 들어 있다

중요 필드:

- 액터 이름:
  `entry["Name"]`
- 액터가 참조하는 컴포넌트:
  `entry["Properties"]["StaticMeshComponent"]["ObjectPath"]`
- 컴포넌트의 실제 StaticMesh:
  `component["Properties"]["StaticMesh"]["ObjectPath"]`
- 컴포넌트 transform:
  - `RelativeLocation`
  - `RelativeRotation`
  - `RelativeScale3D`

### 권장 변환 규칙

#### 메시 매핑

원본:

```text
NARUTO/Content/Environments/LobbyMapAssets/LM_KonohaVillage_BORUTO/Meshes/SM_ENV_LKNVLD_CommonBuilding_A.2
```

basename으로 변환:

```text
SM_ENV_LKNVLD_CommonBuilding_A
```

그 다음 lookup:

```text
SM_ENV_LKNVLD_CommonBuilding_A -> model_guid
```

#### 좌표 변환

이 엔진은 사실상 `Y-up`처럼 동작한다.
언리얼 배치 데이터는 `Z-up` 기준이다.

따라서 1차 변환은 아래처럼 시작하는 것이 맞다.

- position:
  - UE `(X, Y, Z)` -> Engine `(X, Z, Y)`
- scale:
  - UE `(X, Y, Z)` -> Engine `(X, Z, Y)`
- rotation:
  - 우선 `(Pitch, Yaw, Roll)` 그대로 시작
  - 방향이 어긋나면 다음 후보를 테스트
    - `Yaw = -Yaw`
    - pitch/roll 교환

position/scale 축 치환은 비교적 확실하고,
rotation은 실제 배치 결과를 보고 보정해야 한다.

### 오프라인 변환기 예시 코드

이 코드는 엔진 런타임용이 아니라, 오프라인 변환기 예시다.

```cpp
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

struct Vec3
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

static Vec3 ReadXYZ(const json& props, const char* key, const Vec3& fallback = { 0.f, 0.f, 0.f })
{
    if (!props.contains(key))
        return fallback;

    const auto& j = props[key];
    return {
        j.value("X", fallback.x),
        j.value("Y", fallback.y),
        j.value("Z", fallback.z)
    };
}

static string ExtractMeshBaseName(const string& objectPath)
{
    fs::path path(objectPath);
    return path.stem().string();
}

static vector<float> ConvertPosition(const Vec3& ue)
{
    return { ue.x, ue.z, ue.y };
}

static vector<float> ConvertScale(const Vec3& ue)
{
    return { ue.x, ue.z, ue.y };
}

static vector<float> ConvertRotation(const Vec3& ueRot)
{
    // 1차 가정값.
    // 실제 씬에서 확인한 뒤 필요할 때만 수정한다.
    return { ueRot.x, ueRot.y, ueRot.z };
}

static unordered_map<string, string> LoadGuidMap(const fs::path& path)
{
    ifstream file(path);
    json root;
    file >> root;

    unordered_map<string, string> out;
    for (auto& [k, v] : root.items())
        out.emplace(k, v.get<string>());

    return out;
}

int main()
{
    const fs::path fmodelJson =
        R"(C:\Users\moon\Desktop\FModel\Output\Exports\NARUTO\Content\Maps\LobbyMaps\KonohaVillage_BORUTO\LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.json)";

    const fs::path guidMapJson =
        R"(C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\LM_KonohaVillage_BORUTO_mesh_guid_map.json)";

    const fs::path outLevelJson =
        R"(C:\Users\moon\Desktop\Jusin\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Data\json\Levels\KonohaVillage_BORUTO_BackdropBuildings.level.json)";

    ifstream srcFile(fmodelJson);
    json source;
    srcFile >> source;

    const auto guidMap = LoadGuidMap(guidMapJson);

    unordered_map<string, json> componentsByOuterPath;
    for (const auto& entry : source)
    {
        if (entry.value("Type", "") != "StaticMeshComponent")
            continue;

        if (!entry.contains("Outer") || !entry["Outer"].contains("ObjectPath"))
            continue;

        const string outerPath = entry["Outer"]["ObjectPath"].get<string>();
        componentsByOuterPath.emplace(outerPath, entry);
    }

    json level;
    level["levelName"] = "KonohaVillage_BORUTO_BackdropBuildings";
    level["levelIndex"] = 2;
    level["gameObjects"] = json::array();

    for (const auto& actor : source)
    {
        if (actor.value("Type", "") != "StaticMeshActor")
            continue;

        if (!actor.contains("ObjectPath"))
            continue;

        auto compIt = componentsByOuterPath.find(actor["ObjectPath"].get<string>());
        if (compIt == componentsByOuterPath.end())
            continue;

        const json& component = compIt->second;
        const json& props = component["Properties"];

        if (!props.contains("StaticMesh"))
            continue;

        const string meshObjectPath = props["StaticMesh"]["ObjectPath"].get<string>();
        const string meshBaseName = ExtractMeshBaseName(meshObjectPath);

        auto guidIt = guidMap.find(meshBaseName);
        if (guidIt == guidMap.end())
            continue;

        Vec3 loc = ReadXYZ(props, "RelativeLocation");
        Vec3 rot = ReadXYZ(props, "RelativeRotation");
        Vec3 scl = ReadXYZ(props, "RelativeScale3D", { 1.f, 1.f, 1.f });

        json objectJson;
        objectJson["static_class"] = actor.value("Name", meshBaseName);
        objectJson["object_type"] = "OBJECT_TYPE_STATIC_MESH";
        objectJson["layerTag"] = "Layer_Backdrop";
        objectJson["model_guid"] = guidIt->second;
        objectJson["components"] = json::array({
            {
                { "type", "COMPONENT_TYPE_TRANSFORM" },
                { "position", ConvertPosition(loc) },
                { "rotation", ConvertRotation(rot) },
                { "scale", ConvertScale(scl) }
            }
        });

        level["gameObjects"].push_back(objectJson);
    }

    fs::create_directories(outLevelJson.parent_path());
    ofstream outFile(outLevelJson);
    outFile << level.dump(2);

    return 0;
}
```

이 정도면 첫 번째 `.level.json`을 생성하는 데 충분하다.

---

## 7단계. `StaticMeshActor` 프로토타입 등록

### 이유

레벨 로더는 프로토타입으로 등록된 object type만 clone할 수 있다.

### 파일

- `Client/Private/Loader.cpp`

### 권장 수정

추가 include:

```cpp
#include "StaticMeshActor.h"
```

그리고 static 또는 gameplay 초기화에서 프로토타입 등록:

```cpp
GAME->Add_GameObject_Prototype(
    ETOI(ELevelType::Static),
    Protocol::OBJECT_TYPE_STATIC_MESH,
    StaticMeshActor::Create(_device, _context));
```

권장 위치:

- `Loader::Register_Components()`

이유:

- 에디터/런타임 둘 다 동일한 프로토타입을 쓸 수 있다

---

## 8단계. 런타임 레벨 로더 수정

### 이유

현재 로더는 아래처럼 clone한다.

```cpp
auto gameObject = GAME->Clone_GameObject(levelIndex, objType, nullptr);
```

이 방식은 `StaticMeshActor`에 맞지 않는다.

### 파일

- `Engine/Private/Level.cpp`

### 권장 수정

추가 include:

```cpp
#include "StaticMeshActor.h"
```

그리고 clone 구간을 아래 패턴으로 바꾸는 것이 좋다.

```cpp
void* initArg = nullptr;
Client::StaticMeshActor::FStaticMeshDesc meshDesc{};

if (objType == Protocol::OBJECT_TYPE_STATIC_MESH)
{
    meshDesc.name = Utils::ToWString(objJson.value("static_class", "StaticMeshActor"));
    meshDesc.modelGuid = objJson.value("model_guid", "");

    if (meshDesc.modelGuid.empty())
    {
        LOG_WARN("StaticMesh level object missing model_guid. Skipping.");
        continue;
    }

    initArg = &meshDesc;
}

auto gameObject = GAME->Clone_GameObject(levelIndex, objType, initArg);
if (!gameObject)
    gameObject = GAME->Clone_GameObject(0, objType, initArg);

if (!gameObject)
    continue;
```

이후 컴포넌트 적용 로직은 기존 코드 흐름을 유지하면 된다.

### 왜 이게 필요한가

`StaticMeshActor::Initialize(void* arg)`는:

- `modelGuid`를 읽고
- GUID로 실제 경로를 찾고
- `Model` 컴포넌트를 만든다

즉 `initArg`가 없으면 실제 렌더할 메시가 준비되지 않는다.

---

## 9단계. 에디터 로더도 같이 수정

생성한 `.level.json`을 에디터 쪽에서도 동일하게 읽고 싶다면 에디터 serializer 경로도 같이 고치는 것이 맞다.

### 파일

- `Editor/Private/Level_Serializer.cpp`

### 권장 수정

현재 아래 구간:

```cpp
auto gameObject = GAME->Clone_GameObject(0, objType, nullptr);
```

이 부분도 `Engine/Private/Level.cpp`와 같은 `FStaticMeshDesc` 특수 처리를 넣어야 한다.

그렇지 않으면 런타임과 에디터가 서로 다른 방식으로 동작하게 된다.

---

## 10단계. 실제 생성될 오브젝트 예시

실제로는 아래와 비슷한 오브젝트가 하나씩 생성되면 된다.

```json
{
  "static_class": "SM_ENV_LKNVLD_CommonBuilding_F4",
  "object_type": "OBJECT_TYPE_STATIC_MESH",
  "layerTag": "Layer_Backdrop",
  "model_guid": "b4094a1f-77fd-42c2-9a64-0e89e6d8e591",
  "components": [
    {
      "type": "COMPONENT_TYPE_TRANSFORM",
      "position": [-2030.002, 0.0, 25685.0],
      "rotation": [0.0, -90.00012, 0.0],
      "scale": [1.0, 1.0, 1.0]
    }
  ]
}
```

의도적으로 최소 구성만 넣는 것이 좋다.
필요 없는 컴포넌트는 추가하지 않는 것이 맞다.

---

## 11단계. 검증 체크리스트

### A. 모델 변환

각 필수 메시마다 아래를 확인:

- `.meshbin` 존재
- `.material.json` 존재
- `.material.json` 안 텍스처 경로 유효

### B. 에셋 등록

아래를 확인:

- 각 `.meshbin`에 GUID 생성
- GUID가 `.asset_cache.json`에 기록

### C. 생성된 레벨 JSON

아래를 확인:

- 모든 object가 `OBJECT_TYPE_STATIC_MESH`
- 모든 object에 `model_guid` 존재
- transform component에 `type = COMPONENT_TYPE_TRANSFORM` 존재
- `components` 안에 `null` 없음

### D. 런타임 로딩

아래 로그가 없어야 한다:

- `StaticMeshActor: GUID ... not found`
- `Model runtime only supports .meshbin`
- `Failed to clone GameObject for type: OBJECT_TYPE_STATIC_MESH`

### E. 시각 결과

확인할 것:

- 오브젝트가 올바른 위치에 뜨는지
- 90도 축 회전 오류가 없는지
- 좌우 반전이 없는지
- 스케일이 씬의 다른 오브젝트와 비교해 이상하지 않은지

위치는 맞는데 회전만 틀리면 파이프라인 전체를 뜯지 말고,
rotation 변환 함수만 보정하면 된다.

---

## 난이도 추정

이번 맵 기준 난이도는 `중간`이다.

대략적인 분해:

- 에셋 복사 및 정리: `1~2시간`
- 런타임 메시 변환: `2~4시간`
- GUID 등록 및 매핑: `1~2시간`
- FModel JSON -> `.level.json` 변환기 작성: `3~5시간`
- 로더 패치: `1~2시간`
- 회전/배치 보정 테스트: `2~6시간`

쉬운 작업이 아닌 이유:

- 포맷이 두 개다
- 런타임은 평문 경로가 아니라 GUID를 요구한다
- 런타임은 `.gltf`가 아니라 `.meshbin`을 요구한다
- 현재 로더는 static mesh object 경로가 비어 있다

하지만 어려운 편까지는 아닌 이유:

- 이 소스 맵은 대부분 `StaticMeshActor`다
- 필요한 메시 수가 적고 이미 확인됐다
- 저장소 안에 변환 도구가 이미 있다
- 저장소 안에 JSON 레벨 로더도 이미 있다

---

## 첫 번째 마일스톤 권장안

처음부터 로비 전체를 목표로 하지 않는 편이 좋다.
우선 이 파일 하나만 성공시키는 것이 맞다.

- `LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.json`

1차 성공 기준:

1. 17개 고유 메시를 모두 변환한다.
2. `.level.json` 하나를 생성한다.
3. 84개의 static mesh actor를 스폰한다.
4. 아래가 육안으로 확인된다.
   - 배경 건물 라인
   - 화산암/호카게 바위 얼굴 배경
   - 원거리 나무

이게 성공하면 같은 파이프라인을 그대로 재사용할 수 있다.

- 다른 Konoha BORUTO 서브레벨
- 이벤트 장식용 서브레벨
- 더 큰 로비 단위 복원

