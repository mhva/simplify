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

#ifndef SIMPLIFY_ERROR_H_
#define SIMPLIFY_ERROR_H_

#define SIMPLIFY_ENOERR       0
#define SIMPLIFY_EBADSYNTAX   1
#define SIMPLIFY_EBADCONFIG   2
#define SIMPLIFY_ERANGE       3
#define SIMPLIFY_ENOSUFSRCH   4
#define SIMPLIFY_ENOPREFSRCH  5
#define SIMPLIFY_ENOEXCTSRCH  6
#define SIMPLIFY_ECANTSRCH    7
#define SIMPLIFY_ENOPREVSRCH  8
#define SIMPLIFY_EBADCHARSET  9
#define SIMPLIFY_EEXPRTOOLONG 10
#define SIMPLIFY_EBADGUID     11
#define SIMPLIFY_ECOMPILE     12
#define SIMPLIFY_EEXECUTE     13
#define SIMPLIFY_EEXISTS      14
#define SIMPLIFY_EEMPTYSTR    15
#define SIMPLIFY_EUNLIKELY    16
#define SIMPLIFY_EUNKNOWN     32767

#ifdef __cplusplus

#include <system_error>

/**
 * Sets global error code to @error.
 *
 * This function is not a part of C bindings.
 */
void simplify_set_error_code(const std::error_code &error);

extern "C" {
#endif

/**
 * Returns number of last error.
 *
 * \note Simplify C bindings do not support error categories, thus it is
 *  impossible to distinguish between errors originating from different
 *  modules (v8, libeb, etc.). This function does only support errors
 *  originated from the simplify library itself.
 *
 * \retunrs Returns number of last error. If error did not originate from
 *  the simplify library, the SIMPLIFY_EUNKNOWN will be returned.
 */
int simplify_error_code();

/**
 * Returns message describing last error.
 *
 * \note Pointer to a string returned from this function is only valid until
 *  next call to simplify_error_message().
 */
const char *simplify_error_message();

/**
 * Returns name of the error category associated with last error.
 *
 * \note Pointer to a string returned from this function is only valid until
 *  next call to simplify_error_category().
 */
const char *simplify_error_category();

#ifdef __cplusplus
}
#endif
#endif  /* SIMPLIFY_ERROR_H_ */
