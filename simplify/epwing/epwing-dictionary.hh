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

#ifndef EPWING_DICTIONARY_HH_
#define EPWING_DICTIONARY_HH_

#include <string>
#include <vector>

#include <simplify/json_fwd.hh>
#include <simplify/dictionary.hh>

namespace simplify {

class EpwingDictionary : public Dictionary {
public:
    virtual ~EpwingDictionary();

    /**
     * Selects a sub-book within dictionary.
     *
     * \param index Sub-book index.
     */
    std::error_code SelectSubBook(int index);

    /**
     * Returns a list of names of each sub-book within this dictionary.
     */
    Likely<std::vector<std::string>> ListSubBooks() const;

    /**
     * Creates new EpwingDictionary dictionary instance.
     *
     * \param name User-facing name of the dictionary.
     * \param path Path to dictionary.
     * \param script_path Path to custom script file (can be null).
     */
    static Likely<EpwingDictionary *>
        New(const char *name, const char *path, const char *script_path);

    /**
     * Creates new instance of EpwingDictionary with given state.
     *
     * \param name User-facing name of the dictionary.
     * \param path Path to dictionary.
     * \param state Dictionary state.
     */
    static Likely<EpwingDictionary *>
        NewWithState(const char *name, const char *path, const nlohmann::json &state);

public:
    DictionaryType GetType() const override;

    Likely<SearchResults *> Search(const char *expr, size_t limit) override;

    Likely<std::unique_ptr<char[]>>
        ReadText(const char *guid, size_t *text_length) override;

    class Private;

private:
    explicit EpwingDictionary(const char *name);
    explicit EpwingDictionary(std::string name);

    std::error_code
        Initialize(const char *dict_path, const char *script_path,
                   const nlohmann::json *state);

    Likely<SearchResults *> GetResults(size_t max_count);

private:
    friend void to_json(nlohmann::json &, const EpwingDictionary *);
    friend void to_json(nlohmann::json &, const EpwingDictionary &);

    Private *d;
};

void to_json(nlohmann::json &, const EpwingDictionary *);
void to_json(nlohmann::json &, const EpwingDictionary &);

}  // namespace simplify

#endif  // EPWING_DICTIONARY_HH_
