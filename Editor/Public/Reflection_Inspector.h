#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Reflection_Inspector : public Component_Inspector
{
public:
    void Draw_Inspector(Shared<Component> component) override;
    uint32 Get_ComponentType() const override { return 0; }

    void Draw_FromReflection(void* basePtr, const FClassReflectionInfo& info);

    // BT Node Inspector 에서 헤더없이 프로퍼티만 쓸 떄
    static void Draw_Properties_Only(void* basePtr, const FClassReflectionInfo& info);

private:
    void        Draw_Property(void* basePtr, const FPropertyInfo& prop);
    static void Draw_Property_Simple(void* basePtr, const FPropertyInfo& prop);

private:
    float   _capturedFloat      = 0.f;
    int     _capturedInt        = 0;
    bool    _capturedBool       = false;
    float   _capturedVec3[3]    = {};
    float   _capturedColor[4]   = {};
    string  _capturedString;

};

NS_END
