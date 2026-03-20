#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Animation;

class ENGINE_DLL Animation_Manager final : public Base
{
public:
    explicit Animation_Manager();
    virtual ~Animation_Manager() = default;

public:
    HRESULT             Initialize(const wstring& directoryPath);

public:
    void                Load_Animations_From_Directory(const wstring& directoryPath);
    Shared<Animation>   Get_Animation(const string& name) const;

    vector<Shared<Animation>> Get_Animations_By_Prefix(const string& prefix) const;

    vector<Shared<Animation>> Get_All_Animations() const;

private:
    unordered_map<string, Shared<Animation>> _animations;

public:
    static Unique<Animation_Manager> Create(const wstring& directoryPath);
    virtual void Free() override;

};
NS_END
