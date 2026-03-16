#include "pch.h"
#include "Bone.h"

Bone::Bone()
{
}

HRESULT Bone::Initialize(const FBoneRaw& src)
{
    _name = src.name;
    _parentIndex = src.parentIndex;
    _hasOffsetMatrix = src.hasOffsetMatrix;

    _nodeTransform = To_Matrix(src.nodeTransform);
    _offsetMatrix = To_Matrix(src.offsetMatrix);
    _localTransform = _nodeTransform;
    _combinedTransform = Matrix::Identity;

    return S_OK;
}

void Bone::Update_Combined(const Matrix* parent, const Matrix& preLocalTransform)
{
    if (parent != nullptr)
    {
        _combinedTransform = _localTransform * (*parent);
    }
    else
    {
        _combinedTransform = _localTransform * preLocalTransform;
    }

}

Matrix Bone::Get_SkinningMatrix() const
{
    if (_hasOffsetMatrix)
        return _offsetMatrix * _combinedTransform;

    return _combinedTransform;
}

Matrix Bone::To_Matrix(const FMatrixBin& src)
{
    Matrix out{};

    memcpy(&out, src.m, sizeof(Matrix));
    return out;
}

Shared<Bone> Bone::Create(const FBoneRaw& src)
{
    Shared<Bone> instance = make_shared<Bone>();

    if (FAILED(instance->Initialize(src)))
    {
        LOG_ERROR("Failed to Create : Bone");
        return nullptr;
    }

    return instance;
}
