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

#ifndef SIMPLIFYD_CONTEXTACTION_HH_
#define SIMPLIFYD_CONTEXTACTION_HH_

#include "action.hh"

namespace simplifyd {

class ContextAction : public Action
{
public:
    void Handle(simplify::Repository &repository,
                HttpQuery &query,
                HttpResponse &response);
};

}  // namespace simplifyd

#endif  // SIMPLIFYD_CONTEXTACTION_HH_
