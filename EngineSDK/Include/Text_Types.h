#pragma once

#include "Base.h"

NS_BEGIN(Engine)

enum class ETextHAlign { Left, Center, Right };
enum class ETextVAlign { Top, Middle, Bottom };

struct FTextStyle
{
    wstring fontFamily = L"Malgun Gothic";

    float fontSize = 24.f;
    Color color = Color(1.f, 1.f, 1.f, 1.f);

    ETextHAlign hAlign = ETextHAlign::Left;
    ETextVAlign vAlign = ETextVAlign::Middle;

    bool wordWrap = false;
};

NS_END
