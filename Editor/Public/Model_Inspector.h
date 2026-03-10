#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Engine)
class Model;
class ModelMaterial;
NS_END

NS_BEGIN(Editor)

class Model_Inspector : public Component_Inspector
{
public:
    explicit Model_Inspector() = default;
    virtual ~Model_Inspector() = default;

public:
    void Draw_Inspector(shared_ptr<Component> component) override;
    uint32 Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_MODEL; }

private:
    void Draw_ModelPicker(Shared<Model> model, json& data);
    void Draw_MeshList(Shared<Model> model);
    void Draw_MaterialSlots(Shared<Model> model);
    void Draw_TextureSlot(Shared<ModelMaterial> material,
              EMaterialTextureSlot slot, const char* label, uint32 index);

};

NS_END
