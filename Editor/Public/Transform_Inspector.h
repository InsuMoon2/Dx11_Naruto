#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Editor)

class Transform_Inspector : public Component_Inspector
{
public:
    explicit Transform_Inspector() = default;
    virtual ~Transform_Inspector() = default;

public:
    void    Draw_Inspector(shared_ptr<Engine::Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_COMBAT_STAT; }

    static bool XYZ_DragFloat(const char* label, ImVec4 color, float& value,
        float speed, const char* uniqueId, float fieldWidth);

    static bool Draw_XYZRow(const char* id, Vec3& v, float speed);

private:
    Vec3 _capturedPos;
    Vec3 _capturedRot;
    Vec3 _capturedScale;
};

NS_END
