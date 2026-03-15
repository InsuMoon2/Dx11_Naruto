#pragma once

#include "UI_AnimTypes.h"

NS_BEGIN(Engine)

class ENGINE_DLL UI_AnimUtility
{
public:
    static void Sort_Keys(FUIAnimTrack& track);

    static FUIAnimTrack* Find_Track(FUIAnimAsset& asset, EUIAnimProperty property);
    static const FUIAnimTrack* Find_Track(const FUIAnimAsset& asset, EUIAnimProperty property);

    static float Sample_Scalar(const FUIAnimTrack& track, int32 frame, float defaultValue);
    static Vec4  Sample_Vector4(const FUIAnimTrack& track, int32 frame, const Vec4& defaultValue);

    static Vec4 Make_DefaultValue(EUIAnimProperty property);

private:
    static int32 Find_LeftKey_Index(const FUIAnimTrack& track, int32 frame);
    static int32 Find_RightKey_Index(const FUIAnimTrack& track, int32 frame);

    static float Lerp_Scalar(const FUIAnimKey& a, const FUIAnimKey& b, int32 frame);
    static Vec4  Lerp_Vector4(const FUIAnimKey& a, const FUIAnimKey& b, int32 frame);

};

NS_END
