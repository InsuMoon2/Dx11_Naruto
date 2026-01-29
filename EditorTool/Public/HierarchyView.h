#pragma once

#include "EditorWindow.h"

NS_BEGIN(Editor)

class HierarchyView : public EditorWindow
{
public:
    explicit HierarchyView();
    virtual ~HierarchyView();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;


public:
    static shared_ptr<HierarchyView> Create();

};

NS_END
