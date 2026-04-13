#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Level;

class Level_Manager : public Base
{
public:
    explicit Level_Manager();
    virtual ~Level_Manager();

public:
    HRESULT Change_Level(uint32 levelIndex, shared_ptr<Level> level);

    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);
    HRESULT Render();

public:
    uint32  Get_CurrentLevel() const { return _currentLevelIndex; }

    Shared<Level> Get_CurrentLevelType() const { return _currentLevel; }

private:
    Shared<Level>       _currentLevel;
    uint32              _currentLevelIndex = { };

public:
    static unique_ptr<Level_Manager> Create();
    virtual void Free() override;

};

NS_END
