#include "pch.h"
#include "CLevel_Manager.h"
#include "CLevel.h"
#include "CGameInstance.h"

CLevel_Manager::CLevel_Manager()
{
}

CLevel_Manager::~CLevel_Manager()
{

}

HRESULT CLevel_Manager::Change_Level(uint32 levelIndex, shared_ptr<CLevel> level)
{
    // 기존 레벨이 있다면, 리소스 정리
    if (_currentLevel != nullptr)
        GAME->Clear_Resources(_currentLevelIndex);

    _currentLevel = level;
    _currentLevelIndex = levelIndex;

    return S_OK;
}

void CLevel_Manager::Update(float timeDelta)
{
    NULL_CHECK(_currentLevel);

    _currentLevel->Update(timeDelta);
}

void CLevel_Manager::LateUpdate(float timeDelta)
{
    NULL_CHECK(_currentLevel);

    _currentLevel->LateUpdate(timeDelta);
}

HRESULT CLevel_Manager::Render()
{
    if (_currentLevel != nullptr)
        _currentLevel->Render();

    return S_OK;

}

unique_ptr<CLevel_Manager> CLevel_Manager::Create()
{
    return make_unique<CLevel_Manager>();
}

void CLevel_Manager::Free()
{
    CBase::Free();

    _currentLevel.reset();
}
