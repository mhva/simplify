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

#ifndef SIMPLIFYD_ACTION_HH_
#define SIMPLIFYD_ACTION_HH_

#include <cstdlib>

namespace simplify { class Repository; }
namespace simplifyd { class HttpQuery; }
namespace simplifyd { class HttpResponse; }
namespace simplifyd {

class Action
{
public:
    virtual ~Action() {};
    virtual void Handle(simplify::Repository &repository,
                        HttpQuery &query, HttpResponse &response) = 0;
};

}  // namespace simplifyd

#endif /* SIMPLIFYD_ACTION_HH_ */
