#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Reflection_Inspector : public Component_Inspector
{
public:
    void Draw_Inspector(shared_ptr<Component> component) override;
    uint32 Get_ComponentType() const override { return 0; }

    void Draw_FromReflection(void* basePtr, const Engine::FClassReflectionInfo& info);

private:
    void Draw_Property(void* basePtr, const Engine::FPropertyInfo& prop);

};

NS_END
