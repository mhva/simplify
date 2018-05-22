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

#include <simplify/dictionary.hh>
#include <simplify/repository.hh>
#include <simplify/utils.hh>

#include "httpquery.hh"
#include "httpresponse.hh"
#include "contextaction.hh"

namespace simplifyd {

void ContextAction::Handle(simplify::Repository &repository,
                           HttpQuery &query,
                           HttpResponse &response)
{
    response.AddHeader("Content-Type", "application/json; charset=utf-8");
    response.AddHeader("Cache-Control", "no-cache");

    std::string &body = response.GetBody();
    body.append("{\"dicts\":[");

    for (size_t i = 0; i < repository.GetDictionaryCount(); ++i) {
        auto dict = repository.GetDictionary(i);

        // Append the name.
        {
          body.append("{\"name\":\"").append(dict->GetName()).append("\",");
        }

        // Append the id.
        {
          char text[24];
          size_t text_length = simplify::UIntToAlpha10(i, text);

          body.append("\"id\":").append(text, text_length).append(",");
        }

        // Erase trailing comma.
        body.erase(body.size() - 1);
        body.append("},");
    }

    // Erase trailing comma only if there were some dictionaries.
    if (repository.GetDictionaryCount() > 0)
        body.erase(body.size() - 1);

    body.append("]}");
}

}  // namespace simplifyd
