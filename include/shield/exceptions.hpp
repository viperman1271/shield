/* Copyright (c) 2025 Michael Filion
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files(the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and /or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <exception>
#include <stdexcept>
#include <string>

namespace shield
{
class unused_exception : public std::exception
{
};

class runtime_error : public std::runtime_error
{
public:
    explicit runtime_error(const char* msg)
        : std::runtime_error(msg)
    {
    }

    explicit runtime_error()
        : std::runtime_error("Unknown Shield runtime error")
    {
    }
};

class cannot_obtain_value_exception : public runtime_error
{
};

class internal_exception : public runtime_error
{
public:
    explicit internal_exception(const char* msg)
        : runtime_error(msg)
    {
    }
};

class open_circuit_exception : public runtime_error
{
public:
    open_circuit_exception()
        : runtime_error("Circuit is OPEN and no fallback value could be obtained.")
    {
    }
};

class fallback_exception : public runtime_error
{
public:
    fallback_exception()
        : runtime_error("Fallback policy was configured to throw exceptions.")
    {
    }
};
}