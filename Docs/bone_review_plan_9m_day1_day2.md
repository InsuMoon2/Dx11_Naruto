# 수업 복습용 리뷰 플랜 — 9개월차 1일차, 2일차 Bone

> 작성일: 2026-03-13
> 목적: Bone 수업 1일차와 2일차 내용을 복습 중심으로 다시 정리하고, 코드 흐름까지 연결해서 설명할 수 있도록 만드는 계획서

---

## 한눈에 보는 진행 흐름

```mermaid
graph LR
  subgraph "1일차 — Bone 데이터 준비"
    A[aiScene::mRootNode] --> B[Ready_Bones]
    B --> C[CBone vector]
    D[aiMesh::mBones] --> E[VTXANIMMESH]
    E --> F[BlendIndex / BlendWeight]
  end

  subgraph "2일차 — Skinning 파이프라인 연결"
    C --> G[Get_BoneIndex]
    D2[aiBone::mOffsetMatrix] --> H[OffsetMatrices]
    C --> I[CombinedTransformationMatrix]
    H --> J[Bind_BoneMatrices]
    I --> J
    J --> K[g_BoneMatrices[512]]
    K --> L[Shader_VtxAnimMesh]
    L --> M[Monster Render]
  end
```

---

## Part 1 — 이번 복습의 목표

- 1일차는 `aiNode`, `aiBone`, `VTXANIMMESH`가 각각 어떤 역할을 맡는지 구분할 수 있어야 한다.
- 2일차는 `OffsetMatrix`와 `CombinedTransformationMatrix`가 어떻게 합쳐져서 셰이더의 `g_BoneMatrices`로 넘어가는지 설명할 수 있어야 한다.
- 최종적으로는 `Monster::Update() -> CModel::Play_Animation() -> CMesh::Bind_BoneMatrices() -> Shader_VtxAnimMesh` 흐름을 손으로 다시 적을 수 있어야 한다.

---

## Part 2 — 1일차 핵심 정리

### 2.1 1일차에서 무엇이 추가되었는가

| 파일 | 핵심 내용 |
|---|---|
| `Bone.h/cpp` | `CBone` 클래스 신규 추가 |
| `Model.h/cpp` | `m_Bones` 벡터, `Ready_Bones()` 재귀 추가 |
| `Mesh.h/cpp` | `MODEL::NONANIM / ANIM` 분기 추가 |
| `Engine_Struct.h` | `VTXANIMMESH` 구조체 추가 |
| `Shader_VtxMesh.hlsl` | Alpha discard 유지 |

### 2.2 Assimp 기준으로 Bone 관련 데이터 3종

```text
1. aiNode
   - 노드 계층 구조
   - 부모/자식 관계
   - 각 노드의 로컬 변환 행렬 보관

2. aiBone
   - 메쉬 기준 본 데이터
   - 어떤 정점이 어떤 본의 영향을 얼마나 받는지 보관

3. aiAnimation / aiNodeAnim
   - 시간에 따른 키프레임 데이터
   - 이번 1일차에서는 아직 본격 사용 전 단계
```

> [!IMPORTANT]
> 1일차 핵심은 "본 트리를 엔진 쪽 구조로 옮기고, 정점이 어느 본의 영향을 받는지 저장하는 준비 단계"라는 점이다.

### 2.3 CBone 클래스의 의미

`CBone`은 `aiNode` 하나를 엔진용 본 객체 하나로 옮긴 것이다.

```cpp
_char     m_szName[MAX_PATH];
_float4x4 m_TransformationMatrix;
_float4x4 m_CombinedTransformationMatrix;
_int      m_iParentBoneIndex;
```

- `m_TransformationMatrix`: 부모 기준 로컬 변환
- `m_CombinedTransformationMatrix`: 루트부터 누적된 변환
- `m_iParentBoneIndex`: 부모 본 인덱스

### 2.4 Ready_Bones()가 하는 일

```cpp
HRESULT CModel::Ready_Bones(const aiNode* pAINode, _int iParentIndex)
{
    CBone* pBone = CBone::Create(pAINode, iParentIndex);
    m_Bones.push_back(pBone);

    _int iPIndex = m_Bones.size() - 1;

    for (size_t i = 0; i < pAINode->mNumChildren; i++)
        Ready_Bones(pAINode->mChildren[i], iPIndex);
}
```

이 함수는 노드 트리를 DFS 방식으로 평탄화한다.

```text
Root
├── Spine
│   ├── Neck
│   └── Arm
└── Hips

↓ DFS 배열화
[0] Root
[1] Spine
[2] Neck
[3] Arm
[4] Hips
```

> [!NOTE]
> 부모가 항상 자식보다 먼저 배열에 들어오므로, 나중에 Combined 행렬을 계산할 때 매우 편하다.

### 2.5 애니메이션 메쉬 정점 구조

```cpp
typedef struct tagVertexAnimationMesh
{
    XMFLOAT3 vPosition;
    XMFLOAT3 vNormal;
    XMFLOAT3 vTangent;
    XMFLOAT2 vTexcoord;

    XMUINT4  vBlendIndex;
    XMFLOAT4 vBlendWeight;
} VTXANIMMESH;
```

- `vBlendIndex`: 이 정점에 영향을 주는 본 인덱스 최대 4개
- `vBlendWeight`: 각 본의 영향도 최대 4개

### 2.6 Ready_VertexBuffer_For_Anim() 핵심

```cpp
for (size_t i = 0; i < m_iNumBones; i++)
{
    aiBone* pAIBone = pAIMesh->mBones[i];

    for (size_t j = 0; j < pAIBone->mNumWeights; j++)
    {
        aiVertexWeight AIVertexWeight = pAIBone->mWeights[j];

        // x, y, z, w 순서대로 빈 슬롯 채우기
    }
}
```

여기서 중요한 포인트는 다음 두 가지다.

- 애니메이션 메쉬는 `PreTransformVertices`를 쓰지 않는다.
- 본이 런타임에서 정점을 움직여야 하므로, 정점을 미리 굳혀버리면 안 된다.

### 2.7 1일차 복습 포인트

- [ ] `aiNode`와 `aiBone`의 차이를 설명할 수 있는가?
- [ ] 왜 `Ready_Bones()`가 재귀 함수인지 설명할 수 있는가?
- [ ] 왜 본 배열은 트리가 아니라 평탄화된 벡터로 관리하는가?
- [ ] 왜 애니메이션 메쉬에는 `BlendIndex`, `BlendWeight`가 필요한가?
- [ ] 왜 애니메이션 메쉬에는 `PreTransformMatrix`를 적용하지 않는가?

---

## Part 3 — 2일차 핵심 정리

### 3.1 2일차에서 바뀐 것 요약

| 파일 | 변경 내용 |
|---|---|
| `Bone.h/cpp` | `Update_CombinedTransformationMatrix()` 추가 |
| `Model.h/cpp` | `Get_BoneIndex()`, `Play_Animation()`, `Bind_BoneMatrices()` 추가 |
| `Mesh.h/cpp` | `m_OffsetMatrices`, `m_BoneMatrices`, 본 이름 매핑 추가 |
| `Shader_VtxAnimMesh.hlsl` | `g_BoneMatrices`를 이용한 스키닝 VS 추가 |
| `Loader.cpp` | Anim 전용 셰이더와 Anim 모델 프로토타입 등록 |
| `Monster.cpp` | `Play_Animation()` 호출, `Bind_BoneMatrices()` 호출 |

### 3.2 2일차의 본질

1일차가 "본 데이터 구조 준비"였다면,
2일차는 "그 본 데이터를 실제 렌더링 파이프라인에 연결하는 단계"다.

즉,

```text
본 트리 만들기
-> aiBone 이름을 모델 본 인덱스와 연결하기
-> OffsetMatrix 저장하기
-> CombinedTransformationMatrix 계산하기
-> BoneMatrices를 셰이더에 넘기기
-> Vertex Shader에서 스키닝하기
```

### 3.3 CBone::Update_CombinedTransformationMatrix()

```cpp
if (-1 == m_iParentBoneIndex)
{
    Combined = Local * PreTransformMatrix;
}
else
{
    Combined = Local * ParentCombined;
}
```

이제 `CBone`은 단순 저장용 객체가 아니라, 자기 자신의 누적 변환을 계산할 수 있는 객체가 된다.

> [!IMPORTANT]
> 루트 본은 `PreLocalTransformMatrix`까지 곱해주고, 자식 본은 부모의 Combined를 이어받아 누적한다.

### 3.4 CModel::Get_BoneIndex()와 Play_Animation()

#### Get_BoneIndex()

```cpp
_int CModel::Get_BoneIndex(const _char* pBoneName)
{
    // m_Bones를 순회하며 이름이 같은 Bone의 인덱스를 찾는다.
}
```

이 함수는 `aiBone` 이름을 엔진의 `CBone` 배열 인덱스와 연결해주는 다리 역할을 한다.

#### Play_Animation()

```cpp
for (auto& pBone : m_Bones)
    pBone->Update_CombinedTransformationMatrix(m_Bones, PreLocalTransformMatrix);
```

현재 단계의 `Play_Animation()`은 아직 키프레임 보간을 하지 않는다.
지금은 "현재 본 상태의 누적 행렬을 계산하는 함수"에 가깝다.

> [!WARNING]
> 2일차 시점은 아직 완성형 애니메이션 시스템이 아니다.
> `aiAnimation`, `aiNodeAnim`, 보간(Lerp, Slerp)은 다음 단계에서 붙을 가능성이 크다.

### 3.5 CMesh에서 새로 들어온 핵심

#### 1. `CModel*`를 받아서 본 이름을 인덱스로 변환

```cpp
_int iBoneIndex = pModel->Get_BoneIndex(pAIBone->mName.data);
m_BoneIndices.push_back(iBoneIndex);
```

여기서 메쉬 기준 본(`aiBone`)과 모델 기준 본 배열(`m_Bones`)이 연결된다.

#### 2. OffsetMatrix 저장

```cpp
memcpy(&OffsetMatrix, &pAIBone->mOffsetMatrix, sizeof(_float4x4));
XMStoreFloat4x4(&OffsetMatrix, XMMatrixTranspose(XMLoadFloat4x4(&OffsetMatrix)));
m_OffsetMatrices.push_back(OffsetMatrix);
```

`OffsetMatrix`는 바인드 포즈 기준 보정 행렬이다.

#### 3. 최종 BoneMatrices 계산

```cpp
XMStoreFloat4x4(&m_BoneMatrices[i],
    XMLoadFloat4x4(&m_OffsetMatrices[i]) *
    XMLoadFloat4x4(Bones[m_BoneIndices[i]]->Get_CombinedTransformationMatrixPtr()));
```

즉, GPU로 넘기는 최종 행렬은 아래 의미를 가진다.

```text
최종 본 행렬 = OffsetMatrix * CombinedTransformationMatrix
```

### 3.6 Shader_VtxAnimMesh.hlsl

2일차에는 정적 메쉬 셰이더와 별도로 애니메이션 전용 셰이더가 추가된다.

```hlsl
matrix BoneMatrix =
    g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
    g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
    g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
    g_BoneMatrices[In.vBlendIndex.w] * In.vBlendWeight.w;

vector vPosition = mul(float4(In.vPosition, 1.f), BoneMatrix);
```

이 부분이 바로 스키닝의 핵심이다.

- 정점 1개는 여러 본의 영향을 동시에 받을 수 있다.
- 각 본 행렬에 가중치를 곱해서 더한 뒤,
- 그 결과 행렬로 정점을 움직인다.

### 3.7 Client 쪽 연결

#### Loader.cpp

```cpp
Prototype_Component_Shader_VtxAnimMesh
Prototype_Component_Model_Fiona -> MODEL::ANIM
```

즉, 2일차에는 애니메이션 전용 InputLayout과 셰이더를 실제 게임 로딩 단계에 등록한다.

#### Monster.cpp

```cpp
void CMonster::Update(_float fTimeDelta)
{
    m_pModelCom->Play_Animation(fTimeDelta);
}

for (size_t i = 0; i < iNumMeshes; i++)
{
    m_pModelCom->Bind_Material(...);
    m_pModelCom->Bind_BoneMatrices(m_pShaderCom, "g_BoneMatrices", i);
    m_pModelCom->Render(i);
}
```

이제 렌더링 루프는 단순히 텍스처만 바인딩하는 수준이 아니라,
본 행렬까지 매 프레임 셰이더에 넘겨주는 구조가 된다.

### 3.8 2일차 복습 포인트

- [ ] `Get_BoneIndex()`가 왜 필요한지 설명할 수 있는가?
- [ ] `aiBone::mOffsetMatrix`를 왜 따로 저장하는지 설명할 수 있는가?
- [ ] `CombinedTransformationMatrix`와 `OffsetMatrix`의 역할을 구분할 수 있는가?
- [ ] 왜 `Bind_BoneMatrices()`가 메쉬 단위로 호출되는지 설명할 수 있는가?
- [ ] `Shader_VtxAnimMesh`에서 가중 합 스키닝이 어떻게 동작하는지 설명할 수 있는가?
- [ ] 현재 `Play_Animation()`이 진짜 키프레임 애니메이션이 아니라는 점을 이해했는가?

---

## Part 4 — 1일차와 2일차 비교 정리

| 구분 | 1일차 Bone | 2일차 Bone2 |
|---|---|---|
| 초점 | 본 데이터 구조 만들기 | 본 데이터를 GPU 렌더링과 연결하기 |
| 핵심 클래스 | `CBone`, `CModel`, `CMesh` | `CBone`, `CModel`, `CMesh`, `Shader_VtxAnimMesh` |
| CPU 작업 | 본 트리 평탄화, 정점 가중치 저장 | Combined 계산, Offset 적용, BoneMatrices 생성 |
| GPU 작업 | 아직 없음 | 스키닝 Vertex Shader 적용 |
| 상태 | 준비 단계 | 파이프라인 연결 단계 |
| 남은 과제 | 애니메이션 키프레임 처리 | 실제 시간 기반 보간 추가 |

---

## Part 5 — 복습 플랜 계획서

### 5.1 1차 복습 — 구조 복원 (30분)

목표: 큰 그림을 잊지 않는 것

- `aiNode`, `aiBone`, `aiAnimation`의 차이를 노트에 직접 정리한다.
- `Ready_Bones()`가 어떻게 트리를 배열로 바꾸는지 그림으로 그린다.
- `VTXMESH`와 `VTXANIMMESH` 차이를 표로 다시 적는다.
- `BlendIndex`, `BlendWeight`가 왜 4개인지 말로 설명해본다.

### 5.2 2차 복습 — 코드 추적 (40분)

목표: 함수 호출 흐름을 코드 기준으로 따라가는 것

복습 순서:

```text
Model::Initialize_Prototype
-> Ready_Bones
-> Ready_Meshes
-> Mesh::Ready_VertexBuffer_For_Anim
-> Model::Play_Animation
-> Mesh::Bind_BoneMatrices
-> Shader_VtxAnimMesh
```

- 각 함수가 "입력 / 처리 / 출력" 무엇을 담당하는지 한 줄씩 적는다.
- `Get_BoneIndex()`가 메쉬 기준 본과 모델 기준 본을 연결한다는 점을 체크한다.
- `OffsetMatrix * CombinedTransformationMatrix`를 반드시 외운다.

### 5.3 3차 복습 — 렌더링 흐름 설명 연습 (30분)

목표: 누군가에게 설명할 수 있을 정도로 정리하는 것

아래 문장을 막힘 없이 설명할 수 있어야 한다.

```text
애니메이션 메쉬는 정점마다 어떤 본의 영향을 받을지 저장하고,
매 프레임 모델이 본의 Combined 행렬을 계산한 뒤,
메쉬가 OffsetMatrix와 합쳐서 BoneMatrices를 만들고,
셰이더가 그 행렬들을 가중 합으로 섞어서 최종 정점을 만든다.
```

### 5.4 4차 복습 — 확인 테스트 (20분)

- [ ] 본 트리를 왜 벡터로 바꾸는가?
- [ ] `BlendIndex`와 `m_BoneIndices`는 무엇이 다른가?
- [ ] `OffsetMatrix`는 어느 시점 데이터인가?
- [ ] `CombinedTransformationMatrix`는 부모와 어떤 관계를 가지는가?
- [ ] 왜 2일차에는 `Shader_VtxMesh`가 아니라 `Shader_VtxAnimMesh`를 써야 하는가?
- [ ] 현재 코드에서 `fTimeDelta`는 어디까지 활용되고 있고, 무엇이 아직 빠져 있는가?

---

## Part 6 — 헷갈리기 쉬운 포인트 정리

### 6.1 `aiBone` 인덱스와 `CBone` 인덱스는 같은가?

아니다.

- `aiBone` 인덱스는 "메쉬 내부에서 이 본이 몇 번째인가"이다.
- `CBone` 인덱스는 "모델 전체 본 배열에서 몇 번째인가"이다.

그래서 2일차에 `Get_BoneIndex()`가 꼭 필요하다.

### 6.2 OffsetMatrix와 CombinedMatrix는 같은가?

아니다.

- `OffsetMatrix`: 바인드 포즈 기준 보정 행렬
- `CombinedTransformationMatrix`: 현재 계층 기준 누적 행렬

최종적으로 둘을 곱해서 셰이더에 넘긴다.

### 6.3 2일차면 이미 애니메이션이 완성된 것인가?

아직 아니다.

2일차는 "스키닝 가능한 렌더 구조"까지 연결한 단계에 가깝다.
실제 키프레임 보간은 다음 단계에서 들어올 가능성이 높다.

---

## Part 7 — 최종 체크리스트

### 1일차 체크

- [ ] `CBone`이 왜 필요한지 설명할 수 있다.
- [ ] `Ready_Bones()`의 DFS 구조를 이해했다.
- [ ] `VTXANIMMESH`의 입력 레이아웃을 이해했다.
- [ ] `aiBone -> 정점 가중치` 매핑을 코드로 따라갈 수 있다.

### 2일차 체크

- [ ] `Get_BoneIndex()`의 존재 이유를 이해했다.
- [ ] `OffsetMatrix` 저장 흐름을 이해했다.
- [ ] `Play_Animation()`이 어떤 계산을 수행하는지 이해했다.
- [ ] `Bind_BoneMatrices()`가 GPU에 넘기는 데이터가 무엇인지 이해했다.
- [ ] `Shader_VtxAnimMesh`에서 가중 합 스키닝을 설명할 수 있다.

### 최종 목표 체크

- [ ] "1일차는 데이터 준비, 2일차는 렌더 연결"이라고 한 문장으로 정리할 수 있다.
- [ ] 다음 단계로 무엇이 남았는지 말할 수 있다: `aiAnimation`, `aiNodeAnim`, 키프레임 보간, 실제 재생 상태 관리

---

## 부록 — 한 문장 요약

```text
1일차는 Bone 데이터를 엔진 구조로 옮기는 날이고,
2일차는 그 Bone 데이터를 실제 셰이더 스키닝으로 연결하는 날이다.
```
