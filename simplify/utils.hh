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

#ifndef LIBSIMPLIFY_UTILS_HH_
#define LIBSIMPLIFY_UTILS_HH_

#include <system_error>
#include <utility>

#ifdef __GNUC__
# define likely(expr) __builtin_expect((expr), 1)
# define unlikely(expr) __builtin_expect((expr), 0)
#else
# define likely(expr) (expr)
# define unlikely(expr) (expr)
#endif

namespace simplify {

/**
 * Reads contents of the file specified with the @filename parameter into
 * a buffer. Returns a pointer to the buffer if succeeds or NULL if fails.
 */
std::pair<char *, size_t> ReadFile(const char *filename,
                                   std::error_code &error);

/**
 * Converts unsigned integer to string.
 *
 * There is no way to detect conversion failure, so this function should
 * be used to convert only trusted integers.
 *
 * \note The @buffer is not automatically NULL terminated.
 * \return Returns number of bytes stored in the @buffer.
 */
size_t UIntToAlpha10(size_t v, char *buffer);

/**
 * Finds double quotes within the given string and replaces them with
 * the standard C escape sequence: \". Replaces singular backslash characters
 * with the "\\" string.
 *
 * \return Returns size of the resulting string. If the buffer size was not
 *  large enough the <em>(size_t) -1</em> value will be returned.
 */
size_t EscapeDoubleQuotes(const char *input,
                          size_t input_length,
                          char *buffer,
                          size_t buffer_size);

}  // namespace simplify

#endif  // LIBSIMPLIFY_UTILS_HH_
