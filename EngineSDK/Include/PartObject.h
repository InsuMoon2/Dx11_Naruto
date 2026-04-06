#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Model;

class ENGINE_DLL PartObject : public GameObject
{
    GENERATED_BODY(PartObject)

public:
    struct FPartObjectDesc : public FGameObjectDesc
    {
        //const Matrix*   parentMatrix = nullptr;

        Shared<Transform> parentTransform;

        wstring         modelAssetTag;

        // 마스터 모델 -> 얘 기준으로 애니메이션 동작하도록
        // 모든 파츠가 행렬을 따로따로 다 계산을 해주면 코스트 낭비
        Shared<Model>   masterPoseModel;
    };

public:
    explicit PartObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit PartObject(const PartObject& rhs);
    virtual ~PartObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    const Matrix& Get_CombinedWorldMatrix() const { return _combinedWorldMatrix; }

protected:
    void Update_CombinedWorldMatrix(const Matrix& childMatrix)
    {
        //if (_parentMatrix)
        //    _combinedWorldMatrix = childMatrix * (*_parentMatrix);

        if (auto parentTransform = _parentTransform.lock())
        {
            _combinedWorldMatrix = childMatrix * parentTransform->Get_WorldMatrix();
        }
        else
        {
            _combinedWorldMatrix = childMatrix;
        }
    };

protected:
    const Matrix*   _parentMatrix = nullptr;

    Weak<Transform> _parentTransform;

    Matrix          _combinedWorldMatrix = Matrix::Identity;

public:
    Shared<GameObject> Clone(void* arg) override = 0;
    void Free() override;

};

NS_END
