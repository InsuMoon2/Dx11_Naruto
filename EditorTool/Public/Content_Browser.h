#pragma once

#include "EditorWindow.h"

NS_BEGIN(Editor)

class Content_Browser : public EditorWindow
{
public:
    explicit Content_Browser();
    virtual ~Content_Browser();

public:
    void    Initialize() override;
    void    Update() override;
    void    OnGui() override;

public:
    static shared_ptr<Content_Browser> Create();
};

NS_END
