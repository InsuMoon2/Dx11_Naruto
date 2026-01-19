#pragma once

#include "CBase.h"

BEGIN(Engine)

class CLevel;

class CLevel_Manager : public CBase
{
public:
    explicit CLevel_Manager();
    virtual ~CLevel_Manager();

public:
    HRESULT Change_Level(uint32 levelIndex, shared_ptr<CLevel> level);
    void    Update(float timeDelta);
    void    LateUpdate(float timeDelta);
    HRESULT Render();

private:
    shared_ptr<CLevel>  _currentLevel;
    uint32              _currentLevelIndex = { };

public:
    static unique_ptr<CLevel_Manager> Create();
    virtual void Free() override;

};

END
