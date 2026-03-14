#include "pch.h"
#include "UI_AnimUtility.h"

void UI_AnimUtility::Sort_Keys(FUIAnimTrack& track)
{
    sort(track.keys.begin(), track.keys.end(),
        [](const FUIAnimKey& a, const FUIAnimKey& b)
        {
            return a.frame < b.frame;
        });
}

FUIAnimTrack* UI_AnimUtility::Find_Track(FUIAnimAsset& asset, EUIAnimProperty property)
{
    for (auto& track : asset.tracks)
    {
        if (track.property == property)
            return &track;
    }

    return nullptr;
}

const FUIAnimTrack* UI_AnimUtility::Find_Track(const FUIAnimAsset& asset, EUIAnimProperty property)
{
    for (const auto& track : asset.tracks)
    {
        if (track.property == property)
            return &track;
    }
    return nullptr;
}

float UI_AnimUtility::Sample_Scalar(const FUIAnimTrack& track, int32 frame, float defaultValue)
{
    if (track.keys.empty())
        return defaultValue;

    if (track.keys.size() == 1)
        return track.keys[0].value.x;

    const int32 leftIndex = Find_LeftKey_Index(track, frame);
    const int32 rightIndex = Find_RightKey_Index(track, frame);

    if (leftIndex < 0)
        return track.keys.front().value.x;

    if (rightIndex < 0)
        return track.keys.back().value.x;

    // Left, Right가 같으면 Left로 세팅
    if (leftIndex == rightIndex) return track.keys[leftIndex].value.x;

    return Lerp_Scalar(track.keys[leftIndex], track.keys[rightIndex], frame);
}

Vec4 UI_AnimUtility::Sample_Vector4(const FUIAnimTrack& track, int32 frame, const Vec4& defaultValue)
{
    if (track.keys.empty()) return defaultValue;
    if (track.keys.size() == 1) return track.keys[0].value;

    const int32 leftIndex = Find_LeftKey_Index(track, frame);
    const int32 rightIndex = Find_RightKey_Index(track, frame);

    if (leftIndex < 0) return track.keys.front().value;
    if (rightIndex < 0) return track.keys.back().value;
    if (leftIndex == rightIndex) return track.keys[leftIndex].value;

    return Lerp_Vector4(track.keys[leftIndex], track.keys[rightIndex], frame);
}

Vec4 UI_AnimUtility::Make_DefaultValue(EUIAnimProperty property)
{
    switch (property)
    {
    case EUIAnimProperty::ScaleX:
    case EUIAnimProperty::ScaleY:
    case EUIAnimProperty::Alpha:
        return Vec4(1.f, 0.f, 0.f, 0.f);

    case EUIAnimProperty::ColorRGBA:
        return Vec4(1.f, 1.f, 1.f, 1.f);

    default:
        return Vec4(0.f, 0.f, 0.f, 0.f);
    }
}

int32 UI_AnimUtility::Find_LeftKey_Index(const FUIAnimTrack& track, int32 frame)
{
    int32 result = 1;

    int32 size = static_cast<int32>(track.keys.size());

    for (int32 i = 0; i < size; ++i)
    {
        if (track.keys[i].frame <= frame)
            result = i;
    }

    return result;
}

int32 UI_AnimUtility::Find_RightKey_Index(const FUIAnimTrack& track, int32 frame)
{
    int32 size = static_cast<int32>(track.keys.size());

    for (int32 i = 0; i < size; ++i)
    {
        if (track.keys[i].frame >= frame)
            return i;
    }

    return -1;
}

float UI_AnimUtility::Lerp_Scalar(const FUIAnimKey& a, const FUIAnimKey& b, int32 frame)
{
    if (a.frame == b.frame)
        return a.value.x;

    float t = static_cast<float>(frame - a.frame) / static_cast<float>(b.frame - a.frame);
    t = ::clamp(t, 0.f, 1.f);

    return ::lerp(a.value.x, b.value.x, t);
}

Vec4 UI_AnimUtility::Lerp_Vector4(const FUIAnimKey& a, const FUIAnimKey& b, int32 frame)
{
    if (a.frame == b.frame)
        return a.value;

    float t = float(frame - a.frame) / float(b.frame - a.frame);
    t = clamp(t, 0.f, 1.f);

    return Vec4(
        ::lerp(a.value.x, b.value.x, t),
        ::lerp(a.value.y, b.value.y, t),
        ::lerp(a.value.z, b.value.z, t),
        ::lerp(a.value.w, b.value.w, t));
}
