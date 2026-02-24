#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class PipeLine : public Base
{
public:
    explicit PipeLine();
    virtual ~PipeLine() = default;

public:
    void Update();

public: /* Getter */
    const Matrix* Get_Transform(ETransformState state) const;
    const Vec4*   Get_CamPosition() const;

public: /* Setter */
    void Set_Transform(ETransformState state, const Matrix& matrix);

public:
    HRESULT Bind_TransformMatrix(ETransformState state, Shared<Shader> shader, const char* constantName);
    HRESULT Bind_TransformMatrix_Inverse(ETransformState state, Shared<Shader> shader, const char* constantName);

private:
    Matrix _transformMatrices[ETOI(ETransformState::END)];
    Matrix _transformInverseMatrices[ETOI(ETransformState::END)];

public:
    static Unique<PipeLine> Create();
    virtual void Free() override;
};

NS_END
