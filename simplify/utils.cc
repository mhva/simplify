/*
   Copyright (C) 2010 Anton Mihalyov <anton@bytepaper.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifdef POSIX
# include <fcntl.h>
#endif

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <iostream>

#include "utils.hh"

namespace simplify {

size_t UIntToAlpha10(size_t v, char *buffer)
{
    char *out = buffer;
    do *out++ = '0' + v % 10; while (v /= 10);

    std::reverse(buffer, out);
    return out - buffer;
}

size_t EscapeDoubleQuotes(const char *input,
                          size_t input_length,
                          char *buffer,
                          size_t buffer_size)
{
    if (unlikely(input_length == 0))
        return 0;

    if (unlikely(buffer_size < 2 || input_length > buffer_size))
        return (size_t) -1;

    const char *in = input;
    char *out = buffer;
    const char * const in_boundary = input + input_length;
    const char * const out_boundary = buffer + buffer_size - 2;

    for (; in < in_boundary && out <= out_boundary; ++in) {
        char c = *in;

        if (likely(c != '"' && c != '\\')) {
            // Got a character that doesn't need to be escaped.
            *out++ = c;
        } else {
            // Got a special character.
            *out++ = '\\';
            *out++ = c;
        }
    }

    if (likely(in == in_boundary))
        return out - buffer;
    else
        return -1;
}

bool StreqCaseFold(const std::string &s1, const std::string &s2)
{
    return std::equal(s1.begin(), s1.end(), s2.begin(), [](auto x, auto y) {
        return tolower(x) == tolower(y);
    });
}

}  // namespace simplify
