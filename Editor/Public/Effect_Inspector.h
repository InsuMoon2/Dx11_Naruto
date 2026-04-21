#pragma once

#include "Component_Inspector.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Editor)

class Effect_Inspector final : public Component_Inspector
{
public:
    explicit Effect_Inspector() = default;
    virtual ~Effect_Inspector() = default;

public:
    void   Draw_Inspector(shared_ptr<Component> component) override;
    uint32 Get_ComponentType() const override { return Protocol::COMPONENT_TYPE_EFFECT; }

private:
    void Draw_PlaybackSection(Shared<EffectComponent> effectCom);
    // 이펙트 폴더에 있는 effect asset 목록을 popup으로 띄워 선택할 때 호출한다.
    void Draw_EffectAssetPickerPopup();
    void Draw_LayerSection(Shared<EffectComponent> effectCom);

private:
    // 인스펙터에서 미리보기 대상으로 선택한 effect asset 이름을 보관한다.
    char _previewAssetName[256] = {};
    // effect asset popup에서 파일명을 필터링할 때 사용하는 검색 버퍼다.
    char _effectAssetSearchBuf[256] = {};
};

NS_END
