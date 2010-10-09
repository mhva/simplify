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

#include <ctype.h>
#include <string.h>

#include <algorithm>
#include <cassert>

#include "mongoose.h"
#include "httpquery.hh"

namespace simplifyd {

inline static int HexDigitToInt(unsigned char c)
{
    if (isdigit(c))
        return tolower(c) - '0';
    else /* if (isalpha(c)) */
        return 10 + (tolower(c) - 'a');
}

HttpQuery::HttpQuery(mg_request_info &request_info)
  : request_info_(request_info)
{
    query_params_.reserve(6);
    ParseQueryStringDestructive();
}

HttpQuery::~HttpQuery()
{
}

const char *HttpQuery::GetParamValue(const char *name) const
{
    auto it = LocateParam(name);

    if (it != query_params_.end())
        return (*it).value;
    else
        return NULL;
}

const char *HttpQuery::GetParamValue(const char *name, size_t &value_size) const
{
    auto it = LocateParam(name);

    if (it != query_params_.end()) {
        value_size = (*it).value_size;
        return (*it).value;
    } else {
        return NULL;
    }
}

const char *HttpQuery::GetCookieValue(const char *name) const
{
    assert(!"HttpQuery::GetCookieValue() not implemented");
    return NULL;
}

const char *HttpQuery::GetCookieValue(const char *name, size_t &value_size) const
{
    assert(!"HttpQuery::GetCookieValue() not implemented");
    return NULL;
}

void HttpQuery::ParseQueryStringDestructive()
{
    char *start = request_info_.query_string;

    // Bail out if there is no query string.
    if (start == NULL)
        return;

    while (true) {
        char *eq = strchrnul(start, '=');
        char *amp = strchrnul(start, '&');

        // If we didn't find the equal sign then this means that the query
        // string is not well-formed.
        if (*eq == '\0')
            return;

        // If the amp pointer points to the NULL byte, the job is done and
        // there is nothing else to parse.
        bool is_done = *amp == '\0';

        // Replace both '=' and '&' with NULL bytes to indicate the end
        // of parameter's name and parameter's value.
        *eq = '\0';
        *amp = '\0';

        // Do an in-place escape of parameter's value.
        size_t value_length = UnescapeString(eq + 1);

        // Save the parameter if the value have been unescaped successfully.
        if (value_length != (size_t) -1)
            query_params_.push_back(QueryParam { start, eq + 1, value_length });

        if (!is_done)
            start = amp + 1;
        else
            return;
    }
}

size_t HttpQuery::UnescapeString(char *string)
{
    size_t read_offset = 0;
    size_t write_offset = 0;

    while (string[read_offset] != '\0') {
        // Skip current character if it doesn't represent escape sequence.
        if (string[read_offset] != '%') {
            string[write_offset++] = string[read_offset++];
            continue;
        }

        // Fail if we've stumbled upon an escape sequence but it's incomplete.
        if (string[read_offset + 1] == '\0')
            return (size_t) -1;

        // Check if the escape sequence represents '%'.
        if (string[read_offset + 1] == '%') {
            string[write_offset++] = '%';
            read_offset += 2;
            continue;
        }

        // Ensure that the escape sequence has 2 hex digits in it.
        if (string[read_offset + 2] == '\0' ||
            !isxdigit(string[read_offset + 1]) ||
            !isxdigit(string[read_offset + 2])) {
            return (size_t) -1;
        }

        string[write_offset++] = HexDigitToInt(string[read_offset + 1]) * 16 + \
                                 HexDigitToInt(string[read_offset + 2]);
        read_offset += 3;
    }

    string[write_offset++] = '\0';
    return write_offset;
}

HttpQuery::ParameterList::const_iterator
HttpQuery::LocateParam(const char *name) const
{
    return std::find_if(query_params_.begin(), query_params_.end(),
        [name](const QueryParam &p) {
            return strcmp(p.name, name) == 0;
        }
    );
}

}  // namespace simplifyd
