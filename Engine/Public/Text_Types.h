#pragma once

#include "Base.h"

NS_BEGIN(Engine)

enum class ETextHAlign { Left, Center, Right };
enum class ETextVAlign { Top, Middle, Bottom };

// UI 텍스트가 기본으로 사용할 로컬 폰트 패밀리명이다.
inline constexpr const wchar_t* UI_DEFAULT_FONT_FAMILY = L"Open Sans SemiBold";

// DirectWrite에 등록할 OpenSans SemiBold 폰트 파일의 리소스 상대 경로다.
inline constexpr const wchar_t* UI_DEFAULT_FONT_FILE = L"../../Client/Bin/Resources/Fonts/OpenSans-SemiBold.ttf";

struct FTextStyle
{
    wstring fontFamily = UI_DEFAULT_FONT_FAMILY;

    float fontSize = 24.f;
    Color color = Color(1.f, 1.f, 1.f, 1.f);

    ETextHAlign hAlign = ETextHAlign::Left;
    ETextVAlign vAlign = ETextVAlign::Middle;

    bool wordWrap = false;
};

NS_END
