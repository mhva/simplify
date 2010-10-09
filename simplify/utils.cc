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
#include <cstdio>
#include <iostream>

#include "utils.hh"

namespace simplify {

std::pair<char *, size_t> ReadFile(const char *filename, std::error_code &error)
{
    error.clear();

    FILE *file = std::fopen(filename, "rb");
    long fsize;

    if (file == NULL || std::fseek(file, 0, SEEK_END) == -1 ||
        (fsize = std::ftell(file)) == -1) {
        error = make_error_code(static_cast<std::errc>(errno));
        return std::make_pair((char *)NULL, 0);
    }

    rewind(file);

#ifdef HAVE_POSIX_FADVISE
    posix_fadvise(fileno(file), 0, fsize, POSIX_FADV_SEQUENTIAL);
#endif

    char *buffer = NULL;
    size_t buffer_offset = 0;
    size_t buffer_size = fsize;
    size_t length = 0;

    // Read the whole file into buffer.
    while (true) {
        buffer = static_cast<char *>(realloc(buffer, buffer_size));

        size_t got =
            fread(buffer + buffer_offset, 1, buffer_size - buffer_offset, file);

        if (std::ferror(file) != 0) {
            std::fclose(file);

            free(buffer);
            error = make_error_code(static_cast<std::errc>(errno));
            return std::make_pair((char *)NULL, 0);
        }

        if (feof(file) != 0) {
            length = buffer_offset + got;
            break;
        } else {
            // Continue reading file with larger buffer.
            buffer_offset = buffer_size;
            buffer_size *= 1.5f;
        }
    }

    std::fclose(file);
    return std::make_pair(buffer, length);
}

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

}  // namespace simplify
