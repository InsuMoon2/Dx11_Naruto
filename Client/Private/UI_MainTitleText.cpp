#include "pch.h"
#include "UI_MainTitleText.h"

UI_MainTitleText::UI_MainTitleText(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

UI_MainTitleText::UI_MainTitleText(const UI_MainTitleText& rhs)
    : UIObject(rhs)
{
}
