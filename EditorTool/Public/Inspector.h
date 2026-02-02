#pragma once

#include "EditorWindow.h"

NS_BEGIN(Editor)

class Inspector : public EditorWindow
{
public:
    explicit Inspector();
    virtual ~Inspector();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

public:
    static shared_ptr<Inspector> Create();

};

NS_END
