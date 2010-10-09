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

#ifndef SIMPLIFYD_HTTPQUERY_HH_
#define SIMPLIFYD_HTTPQUERY_HH_

#include <cstdlib>
#include <vector>

struct mg_request_info;
namespace simplifyd {

class HttpQuery {
public:
    HttpQuery(mg_request_info &);
    ~HttpQuery();

    const char *GetParamValue(const char *name) const;
    const char *GetParamValue(const char *name, size_t &value_size) const;
    const char *GetCookieValue(const char *name) const;
    const char *GetCookieValue(const char *name, size_t &value_size) const;

private:
    struct QueryParam {
        char *name;
        char *value;
        size_t value_size;
    };
    typedef std::vector<QueryParam> ParameterList;

    void ParseQueryStringDestructive();
    size_t UnescapeString(char *string);

    ParameterList::const_iterator LocateParam(const char *name) const;

private:
    mg_request_info &request_info_;
    ParameterList query_params_;
};

}  // namespace simplifyd

#endif /* SIMPLIFYD_HTTPQUERY_HH_ */
