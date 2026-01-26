#include "pch.h"
#include "CPrototype_Manager.h"

CPrototype_Manager::CPrototype_Manager()
{
}

CPrototype_Manager::~CPrototype_Manager()
{
}

HRESULT CPrototype_Manager::Initialize(uint32 numLevels)
{
    _numLevels = numLevels;

    _prototypes.resize(numLevels);

    return S_OK;
}

HRESULT CPrototype_Manager::Add_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<CBase> prototype)
{
    if (Find_Prototype(levelIndex, prototypeTag) != nullptr)
    {
        MSG_BOX("This object is already exists.");

        return E_FAIL;
    }

    _prototypes[levelIndex].emplace(prototypeTag, prototype);

    return S_OK;
}

HRESULT CPrototype_Manager::Remove_Prototype(uint32 levelIndex)
{
    if (levelIndex >= _numLevels)
        return E_FAIL;

    _prototypes[levelIndex].clear();

    return S_OK;
}

shared_ptr<CBase> CPrototype_Manager::Find_Prototype(uint32 levelIndex, const wstring& prototypeTag)
{
    if (levelIndex >= _numLevels)
        return nullptr;

    auto iter = _prototypes[levelIndex].find(prototypeTag);

    if (iter == _prototypes[levelIndex].end())
        return nullptr;

    return iter->second;
}

unique_ptr<CPrototype_Manager> CPrototype_Manager::Create(uint32 numLevels)
{
    auto instance = make_unique<CPrototype_Manager>();

    if (FAILED(instance->Initialize(numLevels)))
    {
        MSG_BOX("Failed to Created : Prototype Manager");

        return nullptr;
    }

    return instance;
}

void CPrototype_Manager::Free()
{
    CBase::Free();

    
}
