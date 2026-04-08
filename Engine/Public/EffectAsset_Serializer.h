#pragma once
#include "Base.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL EffectAsset_Serializer : public Base
{
public:
    static HRESULT Save_EffectAsset(const string& filePath, const FEffectAssetDesc& assetDesc);
    static HRESULT Load_EffectAsset(const string& filePath, FEffectAssetDesc& outAssetDesc);

private:
    static json Serialize_Layer(const FEffectLayerDesc& layerDesc);
    static FEffectLayerDesc Deserialize_Layer(const json& j);

public:
    virtual void Free() override {}
};

NS_END
