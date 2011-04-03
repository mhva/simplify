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

#ifndef LIBSIMPLIFY_ERROR_HH_
#define LIBSIMPLIFY_ERROR_HH_

#include <system_error>

namespace simplify {

enum class simplify_error {
    success                = 0,
    invalid_syntax         = 1,
    bad_configuration      = 2,
    index_out_of_range     = 3,
    no_suffix_search       = 4,
    no_prefix_search       = 5,
    no_exact_search        = 6,
    cant_search            = 7,
    no_previous_search     = 8,
    bad_charset            = 9,
    search_expr_too_long   = 10,
    bad_guid               = 11,
    js_compilation_error   = 12,
    js_runtime_error       = 13,
    already_exists         = 14,
    empty_string           = 15,
    empty_likely           = 16,
    buffer_exhausted       = 17,
    unexpected_result_type = 18,
    no_more_results        = 19
};

/*
 * eb_error enum class is left empty because currently it doesn't make
 * sense for library users to handle libeb errors on per-error basis.
 *
 * Library users do indeed have a way to see what did libeb return in
 * case of an error, but only in a text form.
 */
enum class eb_error {};

std::error_category &simplify_category();
std::error_category &eb_category();

std::error_code make_error_code(simplify_error);
std::error_condition make_error_condition(simplify_error);

std::error_code make_error_code(eb_error);
std::error_condition make_error_condition(eb_error);

}  // namespace simplify

namespace std {

    template<>
    struct is_error_code_enum<simplify::simplify_error> : public true_type {};

    template<>
    struct is_error_code_enum<simplify::eb_error> : public true_type {};

}  // namespace std

#endif  // LIBSIMPLIFY_ERROR_HH_
