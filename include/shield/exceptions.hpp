#pragma once

#include <exception>

namespace shield
{
class unused_exception : public std::exception
{
};
}