#pragma once

#include <functional>

namespace shield
{
struct fallback_policy
{
    void run();

    std::function<void()> fallback;
};

static const fallback_policy default_fallback_policy;
}