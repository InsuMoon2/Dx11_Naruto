#pragma once

#include "Component_Inspector.h"
#include "Engine_Struct.h"

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
    void    Draw_Inspector(shared_ptr<Component> component) override;
    uint32  Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_MODEL; }

private:
    static json Build_ModelSwapJson(const json& sourceData, const string& newGuid, const string& newModelType);
    static bool Is_ModelAssetOfType(const FAssetMeta* meta, const string& expectedModelType);
    // [추가] 현재 모델 GUID 기준으로 같은 폴더에 있는 AnimBin 후보를 모을 때 호출한다.
    static vector<Shared<Animation>> Collect_FilteredAnimBins(const json& data);
    // [추가] 검색 필터를 통과한 AnimBin 전체를 모델에 한 번에 붙일 때 호출한다.
    static void Add_AllFilteredAnimBins(Shared<Model> model, const vector<Shared<Animation>>& filteredAnims, const string& searchText);
    // [추가] 현재 모델에 같은 이름의 AnimBin이 이미 붙어 있는지 확인할 때 호출한다.
    static bool Is_AnimBinAlreadyAttached(Shared<Model> model, const string& animName);

    void        Draw_ModelPicker(Shared<Model> model, json& data);
    void        Draw_MeshList(Shared<Model> model);
    void        Draw_MaterialSlots(Shared<Model> model);
    void        Draw_TextureSlot(Shared<ModelMaterial> material,
                     EMaterialTextureSlot slot, const char* label, uint32 index);

};

NS_END
