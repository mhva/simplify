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

#include "error.h"
#include "dictionary.h"

#define S_DICT(d) reinterpret_cast<simplify::Dictionary *>(d)
#define S_RESULT(r) reinterpret_cast<simplify::Dictionary::Result *>(r)

using namespace simplify;

struct simplify_sresults {
    Dictionary::Result *results;
    size_t fetch_pointer;
    size_t count;
};


int simplify_dictionary_search(simplify_dictionary_t *dict, const char *expr)
{
    std::error_code error = S_DICT(dict)->Search(expr);

    if (!error) {
        return 0;
    } else {
        simplify_set_error_code(error);
        return -1;
    }
}

simplify_sresults_t *
simplify_dictionary_get_search_results(simplify_dictionary_t *dict,
                                       size_t max_count)
{
    Dictionary::Result *results = new Dictionary::Result[max_count];

    Likely<size_t> likely_count = S_DICT(dict)->GetResults(results, max_count);
    if (likely_count) {
        simplify_sresults_t *sresults = static_cast<simplify_sresults_t *>(
                malloc(sizeof(simplify_sresults_t)));

        sresults->results = results;
        sresults->count = likely_count;
        sresults->fetch_pointer = 0;
        return sresults;
    } else {
        simplify_set_error_code(likely_count.error_code());
        return NULL;
    }
}

char *simplify_dictionary_read_guid(simplify_dictionary_t *dict,
                                    const char *text_guid)
{
    Likely<char *> likely_text = S_DICT(dict)->ReadText(text_guid);

    if (likely_text) {
        return likely_text;
    } else {
        simplify_set_error_code(likely_text.error_code());
        return NULL;
    }
}

const char *simplify_dictionary_get_name(simplify_dictionary_t *dict)
{
    return S_DICT(dict)->GetName();
}

size_t search_results_count(simplify_sresults_t *sr)
{
    return sr->count;
}

void search_results_reset_fetch(simplify_sresults_t *sr)
{
    sr->fetch_pointer = 0;
}

void search_results_destroy(simplify_sresults_t *sr)
{
    delete[] sr->results;
    free(sr);
}

simplify_sresult_t *search_results_fetch_one(simplify_sresults_t *sr)
{
    if (sr->fetch_pointer < sr->count)
        return &sr->results[sr->fetch_pointer++];
    else
        return NULL;
}

const char *search_result_get_name(simplify_sresult_t *sr)
{
    return S_RESULT(sr)->result_name;
}

const char *search_result_get_text_guid(simplify_sresult_t *sr)
{
    return S_RESULT(sr)->text_guid;
}
