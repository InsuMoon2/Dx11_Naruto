#include "pch.h"
#include "UI_AnimPlayer.h"

#include "UIObject.h"
#include "UI_AnimUtility.h"

void UI_AnimPlayer::Set_Asset(Shared<FUIAnimAsset> asset)
{
    _asset = asset;
    _currentFrame = (_asset ? _asset->startFrame : 0);
    _currentTime = static_cast<float> (_asset ? _asset->startFrame
                    / static_cast<float>(_asset->fps) : 0.f);
}

void UI_AnimPlayer::Bind_Target(Shared<UIObject> target)
{
    _target = target;
}

void UI_AnimPlayer::Play()
{
    if (_asset)
        _isPlaying = true;
}

void UI_AnimPlayer::Pause()
{
    _isPlaying = false;
}

void UI_AnimPlayer::Stop()
{
    _isPlaying = false;

    if (!_asset)
        return;

    _currentFrame = _asset->startFrame;
    _currentTime = static_cast<float>(_currentFrame) / static_cast<float>(_asset->fps);

    Apply_Frame(_currentFrame);
}

void UI_AnimPlayer::Set_CurrentFrame(int32 frame)
{
    if (!_asset)
        return;

    _currentFrame = Adjust_Frame(frame);
    _currentTime = static_cast<float>(_currentFrame) / static_cast<float>(_asset->fps);

    Apply_Frame(_currentFrame);
}

void UI_AnimPlayer::Set_CurrentFrameRatio(float ratio)
{
    if (!_asset) return;

    ratio = clamp(ratio, 0.f, 1.f);
    const int32 length = _asset->endFrame - _asset->startFrame;

    Set_CurrentFrame(_asset->startFrame + static_cast<int32>(length * ratio));
}

void UI_AnimPlayer::Update(float timeDelta)
{
    if (!_isPlaying || !_asset || !_target)
        return;

    _currentTime += timeDelta;
    const int32 nextFrame = static_cast<int32>(_currentTime * float(_asset->fps));

    if (!_asset->loop && nextFrame >= _asset->endFrame)
    {
        _currentFrame = _asset->endFrame;
        _currentTime = static_cast<float>(_currentFrame) / static_cast<float>(_asset->fps);

        Apply_Frame(_currentFrame);

        _isPlaying = false;

        return;
    }

    _currentFrame = Adjust_Frame(nextFrame);

    Apply_Frame(_currentFrame);
}

int32 UI_AnimPlayer::Adjust_Frame(int32 frame) const
{
    if (!_asset)
        return 0;

    const int32 start = _asset->startFrame;
    const int32 end = _asset->endFrame;

    if (!_asset->loop)
        return clamp(frame, start, end);

    const int32 length = max(1, end - start + 1);
    int32 relative = (frame - start) % length;

    return start + relative;
}

void UI_AnimPlayer::Apply_Frame(int32 frame)
{
    if (!_asset || !_target)
        return;

    float posX = _target->Get_UIPosX();
    float posY = _target->Get_UIPosY();
    float scaleX = _target->Get_UISizeX();
    float scaleY = _target->Get_UISizeY();
    float rotationZ = _target->Get_UIRotationZ();
    float alpha = _target->Get_UIOpacity();

    Vec4 color(_target->Get_UITint().x, _target->Get_UITint().y, _target->Get_UITint().z, _target->Get_UITint().w);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::PositionX))
        posX = UI_AnimUtility::Sample_Scalar(*track, frame, posX);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::PositionY))
        posY = UI_AnimUtility::Sample_Scalar(*track, frame, posY);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::ScaleX))
        scaleX = UI_AnimUtility::Sample_Scalar(*track, frame, scaleX);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::ScaleY))
        scaleY = UI_AnimUtility::Sample_Scalar(*track, frame, scaleY);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::RotationZ))
        rotationZ = UI_AnimUtility::Sample_Scalar(*track, frame, rotationZ);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::Alpha))
        alpha = UI_AnimUtility::Sample_Scalar(*track, frame, alpha);

    if (const auto* track = UI_AnimUtility::Find_Track(*_asset, EUIAnimProperty::ColorRGBA))
        color = UI_AnimUtility::Sample_Vector4(*track, frame, color);

    _target->Set_UIPosition(posX, posY);
    _target->Set_UIScale(scaleX, scaleY);
    _target->Set_UIRotationZ(rotationZ);
    _target->Set_UIOpacity(alpha);
    _target->Set_UITint(Color(color.x, color.y, color.z, color.w));
}
