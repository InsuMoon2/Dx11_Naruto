#pragma once

#include "UIObject.h"

NS_BEGIN(Client)

class UI_MainTitleText : public UIObject
{
    GENERATED_BODY(UI_MainTitleText)

public:
    explicit UI_MainTitleText(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_MainTitleText(const UI_MainTitleText& rhs);
    virtual ~UI_MainTitleText() = default;

public:


private:


public:


};

NS_END
