#include "pch.h"
#include "PipeLine.h"

#include "Shader.h"

PipeLine::PipeLine()
{
    for (uint32 i = 0; i < ETOI(ETransformState::END); ++i)
    {
        _transformMatrices[i] = Matrix::Identity;
        _transformInverseMatrices[i] = Matrix::Identity;
    }
}

void PipeLine::Update()
{
    for (uint32 i = 0; i < ETOI(ETransformState::END); ++i)
    {
        _transformInverseMatrices[i] = _transformMatrices[i].Invert();
    }
}

const Matrix* PipeLine::Get_Transform(ETransformState state) const
{
    return &_transformMatrices[ETOI(state)];
}

const Vec4* PipeLine::Get_CamPosition() const
{
    return reinterpret_cast<const Vec4*>(
        &_transformInverseMatrices[ETOI(ETransformState::View)].m[3]);
}

void PipeLine::Set_Transform(ETransformState state, const Matrix& matrix)
{
    _transformMatrices[ETOI(state)] = matrix;
}

HRESULT PipeLine::Bind_TransformMatrix(ETransformState state, Shared<Shader> shader, const char* constantName)
{
    return shader->Bind_Matrix(constantName, &_transformMatrices[ETOI(state)]);
}

HRESULT PipeLine::Bind_TransformMatrix_Inverse(ETransformState state, Shared<Shader> shader, const char* constantName)
{
    return shader->Bind_Matrix(constantName, &_transformInverseMatrices[ETOI(state)]);
}

Unique<PipeLine> PipeLine::Create()
{
    return make_unique<PipeLine>();
}

void PipeLine::Free()
{
    Base::Free();
}
