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

#ifndef DICTIONARY_HH_
#define DICTIONARY_HH_

#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <simplify/likely.hh>

namespace simplify {

enum class DictionaryType {
    Unknown,
    Epwing
};

class Repository;

/**
 * A base class for implementing an accessor for custom dictionary type.
 */
class Dictionary {
public:
    class SearchResults {
    public:
        virtual ~SearchResults() = default;

        virtual size_t GetCount() const = 0;
        virtual std::error_code SeekNext() = 0;

        virtual Likely<size_t> FetchGuid(char *buffer, size_t size) = 0;
        virtual Likely<std::unique_ptr<char[]>> FetchHeading(size_t *size) = 0;
        virtual Likely<std::unique_ptr<char[]>> FetchTags(size_t *size) = 0;
    };

    explicit Dictionary(const char *name);
    explicit Dictionary(std::string name);
    virtual ~Dictionary();

    /**
     * Searches the dictionary using specified expression.
     *
     * \param expr A UTF-8 encoded search expression.
     * \param limit Maximum number of results to fetch.
     *
     * NOTE: Every dictionary type has its own search behavior, specifics
     * of search behavior of each dictionary type are documented in the
     * dictionary subclass.
     */
    virtual Likely<SearchResults *> Search(const char *expr, size_t limit) = 0;

    /**
     * Retrieves results from previous search. It's a good idea to specify
     * a reasonable @max_count to avoid taking up too much memory for search
     * results.
     *
     * Each search result contains a GUID (Global Unique IDentifier) that
     * identifies content of the article. To fetch article's content user
     * should call the @ReadText() method and supply it with acquired GUID.
     *
     * \param accumulator A vector which will accumulate retrieved results.
     * \param max_count Maximum number of results to retrieve.
     */
    /*
    virtual Likely<size_t> GetResults(std::vector<Result> &accumulator,
                                      size_t max_count = -1) = 0;
    */

    /**
     * \overload
     *
     * Retrieves results from previous search.
     *
     * \param accumulator A pointer to array of struct @Result where results
     *  should be stored.
     * \param max_count Maximum number of items to retrieve. The number
     *  should be less or equal to the size of the array.
     */
    /*
    virtual Likely<size_t> GetResults(Result *accumulator,
                                      size_t max_count) = 0;
    */

    /**
     * Reads text from the dictionary.
     *
     * \param[in]  guid A GUID of a dictionary entity.
     * \param[out] ptr  A pointer that will receive a text from the dictionary.
     *
     * \note By default all dictionary implementations return HTML formatted
     *  text, but user scripts can override this behavior and use custom text
     *  formatting. The text returned from this function is always UTF-8
     *  encoded.
     *
     * \return Returns a number of characters read from the dictionary.
     */
    virtual Likely<std::unique_ptr<char[]>> ReadText(const char *guid,
                                                     size_t *text_length) = 0;

    /**
     * Returns dictionary name.
     */
    const char *GetName() const;

    /**
     * Sets dictionary name.
     */
    void SetName(const std::string &name);

    /**
     * Returns dictionary's type.
     */
    virtual DictionaryType GetType() const = 0;

    /**
     * Returns repository that this dictionary is part of. If there's no
     * repository or it's been deleted, returns an empty shared_ptr.
     */
    std::shared_ptr<Repository> GetRepository();

    /**
     * Returns repository that this dictionary is part of.
     */
    std::shared_ptr<const Repository> GetRepository() const;

protected:
    /**
     * Sets parent repository.
     */
    void SetRepository(std::shared_ptr<Repository> repo);

    /**
     * Short-hand for calling GetRepository()->SaveState() (with neccessary
     * safe-guards).
     */
    std::error_code SaveState();

private:
    friend class Repository;

    std::weak_ptr<Repository> repo_;
    std::string name_;
};

}  // namespace simplify

#endif  // DICTIONARY_HH_
