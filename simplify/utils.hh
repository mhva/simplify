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

#include <functional>
#include <memory>
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

template<typename T>
using malloc_unique_ptr = std::unique_ptr<T, decltype(&::free)>;

/**
 * Converts unsigned integer to string.
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

/**
 * Case-insensitive comparison of two strings for equality.
 */
bool StreqCaseFold(const std::string &s1, const std::string &s2);

}  // namespace simplify

#endif  // LIBSIMPLIFY_UTILS_HH_
