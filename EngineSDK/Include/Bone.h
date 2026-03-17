#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL Bone final : public Base
{
public:
    Bone();
    Bone(const Bone& rhs);
    virtual ~Bone() = default;

public:
    HRESULT         Initialize(const FBoneRaw& src);

    void            Set_LocalTransform(const Matrix& local) { _localTransform = local; };
    void            Reset_ToNodeTransform() { _localTransform = _nodeTransform; }

    void            Update_Combined(const Matrix* parent, const Matrix& preLocalTransform);

    Matrix          Get_SkinningMatrix() const;

    const string&   Get_Name() const { return _name; }
    int32           Get_ParentIndex() const { return _parentIndex; }
    const Matrix&   Get_NodeTransform() const { return _nodeTransform; }
    const Matrix&   Get_CombinedTransform() const { return _combinedTransform; }

private:
    Matrix          To_Matrix(const FMatrixBin& src);

private:
    string  _name;
    int32   _parentIndex = -1;
    bool    _hasOffsetMatrix = false;

    Matrix  _nodeTransform      = Matrix::Identity;
    Matrix  _offsetMatrix       = Matrix::Identity;
    Matrix _localTransform      = Matrix::Identity;
    Matrix _combinedTransform   = Matrix::Identity;

public:
    static Shared<Bone> Create(const FBoneRaw& src);
    Shared<Bone>        Clone() const;
    void Free() override {};

};

NS_END
