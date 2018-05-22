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

#include <string>

#include <eb/error.h>

#include "error.h"
#include "error.hh"

namespace simplify {

class simplify_category_ : public std::error_category {
public:
    const char *name() const noexcept {
        return "simplify";
    }

    std::string message(int error) const {
        switch (error) {
        case simplify_error::success:
            return "Success";
        case simplify_error::invalid_syntax:
            return "Invalid syntax";
        case simplify_error::bad_configuration:
            return "Incomplete or malformed configuration file";
        case simplify_error::index_out_of_range:
            return "Index value is out of range";
        case simplify_error::no_suffix_search:
            return "Suffix search is not supported by the dictionary";
        case simplify_error::no_prefix_search:
            return "Prefix search is not supported by the dictionary";
        case simplify_error::no_exact_search:
            return "Exact word search is not supported by the dictionary";
        case simplify_error::cant_search:
            return "No known search methods are supported by the dictionary";
        case simplify_error::no_previous_search:
            return "No previous search";
        case simplify_error::bad_charset:
            return "Dictionary uses unsupported charset";
        case simplify_error::search_expr_too_long:
            return "Search expression is too long";
        case simplify_error::bad_guid:
            return "GUID value is invalid";
        case simplify_error::js_compilation_error:
            return "An error occurred while compiling JavaScript script";
        case simplify_error::js_runtime_error:
            return "An error occurred while executing JavaScript script";
        case simplify_error::already_exists:
            return "Already exists";
        case simplify_error::empty_string:
            return "Empty strings are not allowed";
        case simplify_error::empty_likely:
            return "The Likely object were left unitialized";
        case simplify_error::buffer_exhausted:
            return "Input buffer exhausted, allocate more memory";
        case simplify_error::unexpected_result_type:
            return "Unexpected result type";
        case simplify_error::no_more_results:
            return "No more results";
        default:
            return "Unkown Simplify error";
        }
    }
};

class eb_category_ : public std::error_category {
public:
    const char *name() const noexcept {
        return "eb";
    }

    std::string message(int error) const {
        std::string msg = eb_error_message(error);

        // Capitalize the first word in the error message.
        // Currently, libeb errors start at lowercase.
        if (!msg.empty())
            msg[0] = toupper(msg[0]);

        return msg;
    }
};

std::error_category &simplify_category()
{
    static simplify_category_ category;
    return category;
}

std::error_category &eb_category()
{
    static eb_category_ category;
    return category;
}

std::error_code make_error_code(simplify_error e)
{
    return std::error_code(static_cast<int>(e), simplify_category());
}

std::error_code make_error_code(eb_error e)
{
    return std::error_code(static_cast<int>(e), eb_category());
}

std::error_condition make_error_condition(simplify_error e)
{
    return std::error_condition(static_cast<int>(e), simplify_category());
}

std::error_condition make_error_condition(eb_error e)
{
    return std::error_condition(static_cast<int>(e), eb_category());
}

}  // namespace simplify
