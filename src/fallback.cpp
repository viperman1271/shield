#include <shield/fallback.hpp>

namespace shield
{
void fallback_policy::run()
{
    if (fallback)
    {
        fallback();
    }
}
} // shield