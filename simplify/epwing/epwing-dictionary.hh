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

#include <simplify/dictionary.hh>

namespace simplify {

class EpwingDictionary : public Dictionary {
public:
    ~EpwingDictionary();

    Likely<std::vector<std::string>> ListSubBooks() const;
    std::error_code SelectSubBook(size_t index);

    Likely<SearchResults *> Search(const char *expr, size_t limit);

    Likely<size_t> ReadText(const char *guid, char **ptr);
    Likely<size_t> ReadText(const char *guid, char *buffer, size_t buffer_size);

    static Likely<EpwingDictionary *> New(const char *path, Config *conf);
    static Likely<EpwingDictionary *> New(Config *conf);

private:
    EpwingDictionary(Config *conf);

    std::error_code Initialize();
    Likely<SearchResults *> GetResults(size_t max_count);

public:
    class Private;

private:
    Private *d;
};

}  // namespace simplify

#endif  // EPWING_DICTIONARY_HH_
