# Point 파티클 월드 고정 스폰 구현 가이드라인 (Lock World On Spawn)

## 1. 개요

치도리(Chidori) 스킬처럼 번개가 **손 위치에서 한 번 스폰된 후, 손이 움직여도 이미 생성된 파티클 묶음은 따라오지 않게** 만드는 기능이다.

- **기본값** `false` → 기존 모든 이펙트는 영향 없음 (owner transform을 그대로 따라감)
- **`true` 설정 시** → 스폰 시 현재 월드 위치를 한 번만 기록하고 이후 부모-자식 추적을 끊음
- Scale Over Time, Color/Opacity Over Time 등 **머티리얼 애니메이션은 계속 작동**

---

## 2. 변경 파일 목록

| 파일 | 변경 내용 |
|---|---|
| `Engine/Public/EffectAsset_Types.h` | `FEffectPointLayerDesc`에 `lockWorldOnSpawn` 필드 추가 |
| `Engine/Public/EffectComponent.h` | `FActiveLayer`에 `worldLocked`, `lockedWorldMatrix` 추가 |
| `Engine/Private/EffectComponent.cpp` | `Create_LayerObject`, `Apply_LayerTransformInternal`, `Apply_LayerPositionInternal` 수정 |
| `Engine/Private/EffectAsset_Serializer.cpp` | Point 레이어 저장/로드에 `lockWorldOnSpawn` 추가 |
| `Editor/Private/Effect_View.cpp` | Point 설정 UI에 체크박스 추가 |

---

## 3. EffectAsset_Types.h 수정

**위치:** `Engine/Public/EffectAsset_Types.h`  
`FEffectPointLayerDesc` 구조체 내 `moveMode` 바로 아래에 추가한다.

```cpp
// [추가] FEffectPointLayerDesc 내부
uint8 moveMode = 2;                        // 0=Drop(낙하), 1=Spread(방사), 2=Static(정지)

bool lockWorldOnSpawn = false;             // true면 스폰 시점의 월드 위치에 파티클 묶음을 고정하고
                                           // 이후 owner/BoneMatrix를 따라가지 않는다.
                                           // 치도리처럼 "손이 움직여도 번개는 제자리" 연출에 사용한다.
                                           // 기본값 false = 기존처럼 owner transform을 계속 따라간다.
```

---

## 4. EffectComponent.h 수정

**위치:** `Engine/Public/EffectComponent.h`  
`FActiveLayer` 구조체에 월드 고정 상태를 추적하는 두 멤버를 추가한다.

```cpp
struct FActiveLayer
{
    FEffectLayerDesc desc;
    Shared<GameObject> obj;

    float elapsed  = 0.f;
    bool  started  = false;
    bool  finished = false;

    // [추가] lockWorldOnSpawn이 true인 Point 레이어가 스폰되면 true로 설정된다.
    // true인 동안 Apply_LayerTransformInternal / Apply_LayerPositionInternal을 skip한다.
    bool   worldLocked       = false;

    // [추가] lockWorldOnSpawn 시 기록하는 스폰 시점의 월드 행렬.
    // 이후 매 프레임 이 행렬을 오브젝트에 직접 적용해 위치를 유지한다.
    Matrix lockedWorldMatrix = Matrix::Identity;
};
```

---

## 5. EffectComponent.cpp 수정

### 5-1. Create_LayerObject() — Point 레이어 생성 직후 월드 고정 처리

**위치:** `Engine/Private/EffectComponent.cpp`, `Create_LayerObject` 함수 내  
현재 Point 오브젝트 생성 후 `Apply_LayerTransformInternal(layer)` 를 호출하는 부분 바로 뒤에 추가한다.

```cpp
// [기존 코드 유지]
layer.obj = GAME->Clone_GameObject(
    0,
    Protocol::OBJECT_TYPE_INSTANCED_PARTICLE_POINT,
    &pointDesc);

if (!layer.obj)
    return E_FAIL;

Apply_LayerTransformInternal(layer);

// ... (Apply_LayerAnimatedMaterialInternal 등 기존 호출 유지)

// [추가] lockWorldOnSpawn == true이면 스폰 시점 월드 행렬을 기록하고 부모를 끊는다.
if (layer.desc.point.lockWorldOnSpawn)
{
    // Apply_LayerTransformInternal이 Set_Parent()를 통해 부모를 연결한 직후이므로,
    // 이 시점의 WorldMatrix에는 owner 위치 + 레이어 오프셋이 반영되어 있다.
    layer.worldLocked       = true;
    layer.lockedWorldMatrix = layer.obj->Get_Transform()->Get_WorldMatrix();

    // 부모-자식 연결을 끊어 이후 owner가 움직여도 따라오지 않게 한다.
    layer.obj->Get_Transform()->Set_Parent(nullptr);

    // 기록한 월드 행렬을 직접 오브젝트에 적용해 현재 위치를 유지한다.
    layer.obj->Get_Transform()->Set_WorldMatrix(layer.lockedWorldMatrix);
}

return S_OK;
```

---

### 5-2. Apply_LayerTransformInternal() — worldLocked 레이어 skip

**위치:** `Engine/Private/EffectComponent.cpp`, `Apply_LayerTransformInternal` 함수 상단

```cpp
void EffectComponent::Apply_LayerTransformInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    // [추가] 월드 고정 레이어는 Set_Parent / 위치 재적용을 전부 skip한다.
    // 스폰 시 기록한 lockedWorldMatrix를 매 프레임 재적용해 위치를 유지한다.
    if (layer.worldLocked)
    {
        layer.obj->Get_Transform()->Set_WorldMatrix(layer.lockedWorldMatrix);
        return;
    }

    // [기존 코드 그대로 유지]
    auto owner = Get_Owner();
    auto childTransform = layer.obj->Get_Transform();
    // ... (이하 기존 로직 전부 유지)
}
```

---

### 5-3. Apply_LayerPositionInternal() — worldLocked 레이어 skip

**위치:** `Engine/Private/EffectComponent.cpp`, `Apply_LayerPositionInternal` 함수 상단

```cpp
void EffectComponent::Apply_LayerPositionInternal(FActiveLayer& layer)
{
    if (!layer.obj)
        return;

    // [추가] 월드 고정 레이어는 위치 재적용을 skip한다.
    if (layer.worldLocked)
        return;

    // [기존 코드 그대로 유지]
    auto childTransform = layer.obj->Get_Transform();
    // ... (이하 기존 로직 전부 유지)
}
```

> **주의:** `Apply_LayerScaleInternal()`과 `Apply_LayerAnimatedMaterialInternal()`은 **수정하지 않는다.**  
> Scale Over Time, Opacity Over Time 등 머티리얼 애니메이션은 고정된 위치에서도 계속 작동해야 하기 때문이다.

---

## 6. EffectAsset_Serializer.cpp 수정

### 6-1. Serialize_Layer() — Point 저장 부분에 추가

**위치:** `Engine/Private/EffectAsset_Serializer.cpp`, `Serialize_Layer` 함수  
`j["moveMode"] = point.moveMode;` 바로 아래에 추가한다.

```cpp
j["moveMode"] = point.moveMode;
j["lockWorldOnSpawn"] = point.lockWorldOnSpawn; // [추가] 월드 고정 스폰 여부
```

### 6-2. Deserialize_Layer() — Point 로드 부분에 추가

**위치:** `Engine/Private/EffectAsset_Serializer.cpp`, `Deserialize_Layer` 함수  
`if (j.contains("moveMode")) point.moveMode = j["moveMode"];` 바로 아래에 추가한다.

```cpp
if (j.contains("moveMode"))        point.moveMode        = j["moveMode"];
if (j.contains("lockWorldOnSpawn")) point.lockWorldOnSpawn = j["lockWorldOnSpawn"]; // [추가]
```

---

## 7. Effect_View.cpp 수정 (에디터 UI)

**위치:** `Editor/Private/Effect_View.cpp`  
Point 레이어 설정 UI에서 `moveMode` 콤보박스 근처에 체크박스를 추가한다.

```cpp
// [기존] moveMode 콤보박스 등 Point 설정 UI ...

// [추가] Lock World On Spawn 체크박스
ImGui::Separator();
if (ImGui::Checkbox("Lock World On Spawn", &layer.point.lockWorldOnSpawn))
{
    _isDirty = true;
}
if (ImGui::IsItemHovered())
{
    ImGui::SetTooltip(
        "true: 파티클 묶음이 스폰된 위치에 고정됩니다.\n"
        "owner(손/스킬 오브젝트)가 움직여도 파티클은 따라오지 않습니다.\n"
        "치도리처럼 번개를 손 위치에 고정할 때 사용하세요."
    );
}
```

---

## 8. JSON 설정 예시 (Chidori_Point.effect.json)

치도리 번개 Point 레이어에만 아래 옵션을 추가한다. 나머지 이펙트는 기본값(`false` or 미포함)이라 영향 없다.

```json
{
  "kind": 0,
  "layerName": "Chidori_Lightning",
  "lockWorldOnSpawn": true,
  "isLoop": true,
  "numInstances": 30,
  "spawnShape": 4,
  "spawnRadius": 0.3,
  ...
}
```

---

## 9. 검증 체크리스트

- [ ] 치도리 스킬 시작 시, 파티클이 **손 위치**에서 생성됨
- [ ] 이후 캐릭터 손이 움직여도 **이미 생성된 파티클 묶음은 고정**됨
- [ ] Scale Over Time, Opacity Over Time 등 **머티리얼 애니메이션은 계속 반영**됨
- [ ] 라센간 / 파이어볼 등 **기존 이펙트는 lockWorldOnSpawn 기본값(false)이라 기존처럼 owner를 따라감**
- [ ] 에디터에서 체크박스를 켜고 저장→로드 후에도 **JSON에 정상 유지**됨
