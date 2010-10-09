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

#ifndef CSIMPLIFY_DICTIONARY_H_
#define CSIMPLIFY_DICTIONARY_H_

#include <stdlib.h>

typedef void simplify_dictionary_t;
typedef void simplify_sresult_t;
typedef struct simplify_sresults simplify_sresults_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Searches the dictionary for specified string. Search results can
 * later be retrieved by the @simplify_dictionary_get_search_results()
 * function.
 *
 * \param expr An UTF-8 encoded search expression.
 *
 * NOTE: Every dictionary type has its own search behavior, specifics
 * of search behavior of each dictionary type are documented <somewhere>.
 */
int simplify_dictionary_search(simplify_dictionary_t *dict, const char *expr);

/**
 * Retrieves results from previous search. It's a good idea to specify
 * a reasonable @max_count to avoid taking up too much memory for search
 * results.
 *
 * Each search result contains a GUID (Global Unique IDentifier) that
 * identifies content of the article. To fetch article's content user
 * should call the @simplify_dictionary_read_guid() function and supply
 * it with acquired GUID.
 */
simplify_sresults_t *
simplify_dictionary_get_search_results(simplify_dictionary_t *dict,
                                       size_t max_count);

/**
 * Reads text from the dictionary.
 *
 * \param text_guid A GUID of a dictionary entity.
 *
 * \note By default all dictionary implementations return HTML formatted text,
 *  but user scripts can override this behavior and use custom text formatting.
 *  The text returned from this function is always UTF-8 encoded.
 *
 * \return Returns text associated with the given GUID (see note).
 */
char *simplify_dictionary_read_guid(simplify_dictionary_t *dict,
                                    const char *text_guid);

/**
 * Returns the name of the dictionary.
 */
const char *simplify_dictionary_get_name(simplify_dictionary_t *dict);


/**
 * Returns the number of results.
 */
size_t search_results_count(simplify_sresults_t *sr);

/**
 * Resets fetch pointer.
 *
 * This function is useful if you want to iterate over search results multiple
 * times. It's not necessary to call this function first time you want to
 * iterate over results, because the fetch pointer is already being reset.
 */
void search_results_reset_fetch(simplify_sresults_t *sr);

/**
 * Destroys search results object and frees memory that belongs to it.
 */
void search_results_destroy(simplify_sresults_t *sr);

/**
 * Returns one result from the search results object and sets fetch pointer to
 * point to the next result.
 *
 * Example usage:
 *  simplify_sresult *sresult;
 *
 *  while ((sresult = search_results_fetch_one(sr))) {
 *      [Useful Payload]
 *  }
 *
 * \note The pointer returned from this function is valid until the parent
 *  search results object is destroyed.
 *
 * \return Returns a pointer to a search result. If there are no results left,
 *  NULL will be returned.
 */
simplify_sresult_t *search_results_fetch_one(simplify_sresults_t *sr);

/**
 * Returns name of the search result.
 *
 * \note The pointer returned from this function is valid until the parent
 *  search results object is destroyed.
 */
const char *search_result_get_name(simplify_sresult_t *sr);

/**
 * Returns GUID of the article associated with the search result.
 *
 * The actual text of the article can be read by supplying GUID to the
 * simplify_dictionary_read_guid() function.
 *
 * \note The pointer returned from this function is valid until the parent
 *  search results object is destroyed.
 */
const char *search_result_get_text_guid(simplify_sresult_t *sr);

#ifdef __cplusplus
}
#endif

#endif  /* CSIMPLIFY_DICTIONARY_H_ */
