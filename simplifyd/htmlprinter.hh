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

#ifndef SIMPLIFY_HTMLPRINTER_HH_
#define SIMPLIFY_HTMLPRINTER_HH_

#include <string>
#include <utility>

namespace simplifyd {

class HtmlPrinter
{
public:
    HtmlPrinter();
    ~HtmlPrinter();

    void OpenGenericPage(std::string &buffer);
    void CloseGenericPage(std::string &buffer);

    void OpenSearchPage(std::string &buffer);
    void CloseSearchPage(std::string &buffer);

    void OpenArticlePage(std::string &buffer);
    void CloseArticlePage(std::string &buffer);

    void PutSearchResults(simplify::Dictionary::SearchResults *results,
                          std::string &buffer);

    static HtmlPrinter &Instance();

private:
};

}  // namespace simplifyd

#endif /* SIMPLIFY_HTMLPRINTER_HH_ */
