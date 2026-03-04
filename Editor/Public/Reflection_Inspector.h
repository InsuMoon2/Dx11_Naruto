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

private:
    float   _capturedFloat      = 0.f;
    int     _capturedInt        = 0;
    bool    _capturedBool       = false;
    float   _capturedVec3[3]    = {};
    float   _capturedColor[4]   = {};

};

NS_END
