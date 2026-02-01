#pragma once

#include "EditorWindow.h"
#include "Editor_Logger.h"

class Console_View : public EditorWindow
{
public:
    explicit Console_View();
    virtual ~Console_View();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

public:
    static shared_ptr<Console_View> Create();

private:
    bool _autoScroll = true;

};

