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

    if (dict_id == nullptr) {
        body.append("{\"error\":\"No dictionary ID specified\"}");
        return;
    } else if (guid == nullptr) {
        body.append("{\"error\":\"No article GUID specified\"}");
        return;
    }

    auto dict = repository.GetDictionary(strtol(dict_id, NULL, 10));

    if (dict == nullptr) {
        body.append("{\"error\":\"Invalid dictionary index specified\"}");
        return;
    }

    size_t text_length = 0;
    simplify::Likely<std::unique_ptr<char[]>> likely_text =
        dict->ReadText(guid, &text_length);
    if (!likely_text.is_error()) {
      body.append("{\"article\":\"")
          .append(likely_text.value_checked().get(), text_length)
          .append("\"}");
    } else {
      body.append("{\"error\":\"An error occurred while retrieving "
                  "article data from the dictionary: ")
          .append(likely_text.error_code().message())
          .append("\"}");
      return;
    }

    // FIXME: Bad 'Expires' header.
    response.AddHeader("Expires", "Tue, 1 Sep 2015 00:00:00 GMT");
    response.AddHeader("Cache-Control", "max-age=3600,public");
}

}  // namespace simplifyd
