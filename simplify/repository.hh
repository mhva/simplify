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

#ifndef LIBSIMPIFY_REPOSITORY_HH_
#define LIBSIMPIFY_REPOSITORY_HH_

#include <memory>
#include <string>
#include <vector>
#include <simplify/dictionary.hh>
#include <simplify/likely.hh>

namespace simplify {

class Repository : public std::enable_shared_from_this<Repository>
{
public:
    typedef std::vector<std::shared_ptr<Dictionary>> DictionaryList;

    /**
     * Saves repository state to disk.
     *
     * \return Returns error code, if any.
     */
    std::error_code SaveState();

    /**
     * Adds dictionary to this repository. Adding dictionary means that it
     * will be automatically restored when this repository is created again
     * (e.g. next time program launches).
     *
     * \param d Pointer to dictionary.
     */
    void AddDictionary(std::unique_ptr<Dictionary> d);

    /**
     * Removes dictionary from this repository. The lifetime of the dictionary
     * instance is governed by shared_ptr, and, unless there's no external
     * references, the actual instance won't be deleted.
     *
     * \param pos Dictionary index.
     */
    void RemoveDictionary(size_t pos);

    /**
     * Removes dictionary from this repository.
     *
     * param it Iterator pointing to the dictionary entry.
     */
    void RemoveDictionary(DictionaryList::iterator it);

    /**
     * Returns iterator object pointing to the beginning of the dictionary
     * list.
     */
    DictionaryList::iterator Begin();
    DictionaryList::const_iterator Begin() const;

    /**
     * Returns iterator object pointing to the end of the dictionary list.
     */
    DictionaryList::iterator End();
    DictionaryList::const_iterator End() const;

    /**
     * Returns a number of dictionaries currently loaded in the repository.
     */
    size_t GetDictionaryCount() const;

    /**
     * Returns a dictionary with index @pos from the repository.
     *
     * \return Returns a pointer to the dictionary object; if the dictionary
     *  with specified index doesn't exist in the repository, NULL is returned.
     */
    std::shared_ptr<Dictionary> GetDictionary(size_t pos);
    std::shared_ptr<const Dictionary> GetDictionary(size_t pos) const;

    /**
     * Creates or restores dictionary repository.
     *
     * \param config_path Path to configuration file. If this file does not
     *  exist or is null, an empty repository will be created.
     */
    static Likely<std::shared_ptr<Repository>> New(const char *config_path);

private:
    Repository(const char *config_path, DictionaryList &&dicts);

public:
    ~Repository();

private:
    std::string config_path_;
    DictionaryList dicts_;
};

}  // namespace simplify

#endif  // LIBSIMPIFY_REPOSITORY_HH_
