#include "pch.h"
#include "Level_Manager.h"
#include "Level.h"
#include "GameInstance.h"

Level_Manager::Level_Manager()
{
}

Level_Manager::~Level_Manager()
{

}

HRESULT Level_Manager::Change_Level(uint32 levelIndex, shared_ptr<Level> level)
{
    // 기존 레벨이 있다면, 리소스 정리
    if (_currentLevel != nullptr)
        GAME->Clear_Resources(_currentLevelIndex);

    _currentLevel = level;
    _currentLevelIndex = levelIndex;

    return S_OK;
}

void Level_Manager::Update(float timeDelta)
{
    NULL_CHECK(_currentLevel);

    _currentLevel->Update(timeDelta);
}

void Level_Manager::Late_Update(float timeDelta)
{
    NULL_CHECK(_currentLevel);

    _currentLevel->Late_Update(timeDelta);
}

HRESULT Level_Manager::Render()
{
    if (_currentLevel != nullptr)
        _currentLevel->Render();

    return S_OK;

}

unique_ptr<Level_Manager> Level_Manager::Create()
{
    return make_unique<Level_Manager>();
}

void Level_Manager::Free()
{
    Base::Free();

    _currentLevel.reset();
}
