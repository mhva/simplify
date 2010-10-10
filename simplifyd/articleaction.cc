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

#include <cstdlib>
#include <iostream>

#include <simplify/dictionary.hh>
#include <simplify/repository.hh>
#include <simplify/utils.hh>

#include "httpquery.hh"
#include "httpresponse.hh"
#include "articleaction.hh"

namespace simplifyd {

void ArticleAction::Handle(simplify::Repository &repository,
                           HttpQuery &query,
                           HttpResponse &response)
{
    const char *dict_id = query.GetParamValue("id");
    const char *guid = query.GetParamValue("guid");
    std::string &body = response.GetBody();

    response.AddHeader("Content-Type", "application/json; charset=utf-8");

    if (dict_id == NULL) {
        body.append("{\"error\":\"No dictionary ID specified\"}");
        return;
    } else if (guid == NULL) {
        body.append("{\"error\":\"No article GUID specified\"}");
        return;
    }

    simplify::Dictionary *dict =
        repository.GetDictionaryByIndex(strtol(dict_id, NULL, 10));

    if (dict == NULL) {
        body.append("{\"error\":\"Invalid dictionary index specified\"}");
        return;
    }

    size_t buffer_size = 16 * 1024;

    while (true) {
        char buffer[buffer_size];

        simplify::Likely<size_t> likely_length =
            dict->ReadText(guid, buffer, buffer_size);

        if (likely_length) {
            body.append("{\"article\":\"")
                .append(buffer, (size_t)likely_length)
                .append("\"}");
            break;
        } else if (likely_length.error_code() ==
                        simplify::simplify_error::buffer_exhausted) {
            // Looks like the size of the buffer was not large enough.
            // Increase it just a tiny little bit and attempt to read the
            // article again.
            buffer_size += 4096;
        } else {
            // Bail out if something bad happened.
            body.append("{\"error\":\"An error occurred while retrieving "
                        "article data from the dictionary: ")
                .append(likely_length.error_code().message())
                .append("\"}");
            return;
        }
    }

    // FIXME: Bad 'Expires' header.
    response.AddHeader("Expires", "Tue, 1 Sep 2015 00:00:00 GMT");
    response.AddHeader("Cache-Control", "max-age=3600,public");
}

}  // namespace simplifyd
