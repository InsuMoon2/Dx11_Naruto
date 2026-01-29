#pragma once

#include <functional>
#include <vector>
#include <algorithm>

NS_BEGIN(Engine)

template<typename... Args>
class Delegate
{
public:
    using FunctionType = std::function<void(Args...)>;

    Delegate() = default;
    ~Delegate() = default;

public:


};

NS_END
