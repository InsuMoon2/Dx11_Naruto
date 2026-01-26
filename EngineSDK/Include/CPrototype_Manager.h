#pragma once

#include "CBase.h"

NS_BEGIN(Engine)

class CPrototype_Manager : public CBase
{
public:
    explicit CPrototype_Manager();
    virtual ~CPrototype_Manager();

public:
    HRESULT Initialize(uint32 numLevels);

    HRESULT Add_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<CBase> prototype);
    HRESULT Remove_Prototype(uint32 levelIndex);
    //shared_ptr<CBase> Clone_Prototype();

private:
    shared_ptr<CBase> Find_Prototype(uint32 levelIndex, const wstring& prototypeTag);

private:
    using Prototypes = unordered_map<wstring, shared_ptr<CBase>>;
    vector<Prototypes> _prototypes;

    uint32 _numLevels = { };

public:
    static unique_ptr<CPrototype_Manager> Create(uint32 numLevels);
    virtual void Free() override;

};

NS_END
