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

#include <string>
#include <vector>
#include <simplify/likely.hh>

namespace simplify {

enum class DictionaryType {
    Unknown,
    Epwing
};

class Dictionary;
class Repository
{
public:
    typedef std::vector<Dictionary *> DictionaryList;

    /**
     * Creates dictionary object and registers it in the repository.
     *
     * \param name Name of the dictionary. Dictionary name will be used when
     *  initializing files and directories in the repository. Make sure that
     *  the name doesn't contain special characters forbidden to use by the
     *  underlying filesystem.
     * \param path Path to the dictionary file/directory.
     * \param type Specifies dictionary type. If the dictionary type is not
     *  known in advance, it is possible to detect it with the
     *  @IdentifyDictionary() method call.
     */
    Likely<Dictionary *> NewDictionary(const char *name,
                                       const char *path,
                                       DictionaryType type);

    /**
     * Unregisters dictionary object from the repository and destroys it.
     *
     * \note After calling this function user scripts will be left intact,
     *  but dictionary configuration will be completely erased.
     */
    void DeleteDictionary(Dictionary *dict);

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
     * Returns a dictionary with index @index from the repository.
     *
     * \return Returns a pointer to the dictionary object; if the dictionary
     *  with specified index doesn't exist in the repository, NULL is returned.
     */
    Dictionary *GetDictionaryByIndex(size_t index);
    const Dictionary *GetDictionaryByIndex(size_t index) const;

    /**
     * Tries to guess dictionary type.
     *
     * \note Currently, all supported dictionary formats can be detected
     *  reliably.
     *
     * \return Returns dictionary type. If the method fails to detect dictionary
     *  type, DictionaryType::Unknown will be returned.
     */
    static DictionaryType IdentifyDictionary(const char *path);

    /**
     * Creates or restores dictionary repository.
     *
     * \param repository_path Directory where repository should be initialized.
     *  The directory must exist and must be accessible by the current user.
     *
     * \note If the repository at this given directory doesn't exist, it will
     *  be created. In this case user must have read/write permissions for
     *  this directory.
     *
     * \note It is not advisable to create repository in the directory where
     *  some files do already exist. The function will not fail just because of
     *  this, but beware, creating repository in a non-empty directory might
     *  lead to a loss of some data.
     */
    static Likely<Repository *> New(const char *repository_path);

private:
    Repository(const char *repository_path);

public:
    ~Repository();

private:
    DictionaryList dicts_;
    std::string repository_path_;
};

}  // namespace simplify

#endif  // LIBSIMPIFY_REPOSITORY_HH_
