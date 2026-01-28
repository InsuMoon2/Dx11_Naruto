#pragma once

#include "EditorWindow.h"

class Logger : public EditorWindow
{
public:
    explicit Logger();
    virtual ~Logger();

public:
    void Initialize() override;
    void Update() override;
    void OnGui() override;

public:
    static shared_ptr<Logger> Create();

};

