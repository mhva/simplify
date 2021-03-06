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
#include <system_error>

#include <simplify/dictionary.hh>
#include <simplify/repository.hh>
#include <simplify/utils.hh>

#include "httpresponse.hh"
#include "server.hh"
#include "searchaction.hh"

namespace simplifyd {

void SearchAction::Handle(simplify::Repository &repository,
                          HttpQuery &query,
                          HttpResponse &response)
{
    std::string &body = response.GetBody();
    const char *dict_id = query.GetParamValue("id");
    const char *expr = query.GetParamValue("q");

    response.AddHeader("Content-Type", "application/json; charset=utf-8");
    response.AddHeader("Cache-Control", "no-cache");

    if (expr == NULL) {
        body.append("{\"error\":\"Empty search expression\"}");
        return;
    }

    body.append("{");

    // Decide which method of search to use.
    // If the dictionary ID is specified we search only the dictionary with
    //  the given ID.
    // If the dictionary ID is missing we search all dictionaries.
    if (dict_id != NULL) {
        auto dict = repository.GetDictionary(strtol(dict_id, NULL, 10));

        if (dict != NULL) {
            body.append("\"").append(dict->GetName()).append("\":");
            SearchDict(repository, *dict, expr, response);
        } else {
            body.append("{\"error\":\"Invalid dictionary id:\"}");
            return;
        }
    } else {
        SearchAll(repository, expr, response);
    }

    body.append("}");
}

void SearchAction::SearchDict(simplify::Repository &repository,
                              simplify::Dictionary &dict,
                              const char *expr,
                              HttpResponse &response)
{
    // TODO: Make the limit tweakable.
    size_t result_limit = 800;

    simplify::Likely<simplify::Dictionary::SearchResults *> likely_results =
        dict.Search(expr, result_limit);
    std::string &body = response.GetBody();

    if (!likely_results) {
        body.append("{\"error\":\"") \
            .append(likely_results.error_code().message()) \
            .append("\"}");
        return;
    }

    // Format the 'limit' JSON entry which contains the maximum number of
    // search results and open the 'results' array.
    {
        char tmp[64];
        size_t length = simplify::UIntToAlpha10(result_limit, tmp);

        // Add the 'limit' integer.
        body.append("{\"limit\":")
            .append(tmp, length)
            .append(1, ',');

        // Open the 'results' array.
        body.append("\"results\":[");
    }

    simplify::Dictionary::SearchResults *results = likely_results;

    // 4K is ought to be enough for a search result name or GUID.
    // Anything near or beyond that size is just crazy anyway.
    char text_buffer[4 * 1024];
    size_t accepted_results_count = 0;
    std::error_code seek_error;

    while (!(seek_error = results->SeekNext())) {
        // Integer containing an offset in the body where the current
        // result starts. We use the value of this variable to erase
        // result's text in case anything goes wrong to avoid producing
        // malformed JSON.
        size_t result_start = body.length();
        size_t length = 0;

        simplify::Likely<size_t> likely_length =
            results->FetchGuid(text_buffer, sizeof(text_buffer));

        if (likely_length) {
            body.append("[\"")
                .append(text_buffer, likely_length)
                .append("\",");
        } else {
            std::cout << "An error occurred while retrieving article GUID "
                      << "from " << dict.GetName() << ": "
                      << likely_length.error_code().message() << std::endl;
            body.erase(result_start);
            continue;
        }

        auto maybe_heading = results->FetchHeading(&length);
        if (maybe_heading) {
            body.append("\"")
                .append(maybe_heading.value_checked().get(), length)
                .append("\",");
        } else {
            std::error_code &e = maybe_heading.error_code();

            // Do not log things if the dictionary returned an unexpected
            // result type error. Actually, this is quite legitimate
            // behavior, user scripts may use this trick to skip unwanted
            // search results. It also should be noted, that this behavior
            // is only acceptable for scripts executed by the FetchHeading
            // method, for any other method this is an error.
            if (e != simplify::simplify_error::unexpected_result_type) {
                std::cout << "An error occurred while retrieving heading "
                          << "for a search result with GUID " << text_buffer
                          << "from " << dict.GetName() << ": "
                          << maybe_heading.error_code().message()
                          << std::endl;
            }

            body.erase(result_start);
            continue;
        }

        auto maybe_tags = results->FetchTags(&length);
        if (maybe_tags) {
            // Only append tags if the string is not empty.
            if (length > 0) {
                body.append("\"")
                    .append(maybe_tags.value_checked().get(), length)
                    .append("\",");
            }
        } else {
            std::cout << "An error occurred while retrieving tags for a "
                      << "search entry with GUID " << text_buffer
                      << " from " << dict.GetName() << ": "
                      << maybe_tags.error_code().message() << std::endl;
            body.erase(result_start);
        }

        // Erase last comma.
        body.erase(body.size() - 1);
        body.append("],");

        ++accepted_results_count;
    }

    delete results;

    // Erase last comma if we have at least one result. There will be no
    // comma if there are no results.
    if (accepted_results_count > 0)
        body.erase(body.size() - 1);

    body.append("]}");
}

void SearchAction::SearchAll(simplify::Repository &repository,
                             const char *expr,
                             HttpResponse &response)
{
    std::string &body = response.GetBody();

    for (auto it = repository.Begin(); it != repository.End(); ++it) {
        body.append("\"").append((*it)->GetName()).append("\":");
        SearchDict(repository, **it, expr, response);
        body.append(",");
    }

    body.erase(body.size() - 1);
}

}  // namespace simplifyd
