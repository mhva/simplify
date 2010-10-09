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

#ifndef SIMPLIFYD_SERVER_HH_
#define SIMPLIFYD_SERVER_HH_

#include <string.h>

#include <system_error>
#include <unordered_map>

#include "hash.hh"
#include "httpquery.hh"
#include "mongoose.h"

struct mg_context;
namespace simplify { class Repository; }
namespace simplifyd { class Action; }

namespace simplifyd {

struct QueryParam {
    const char *name;
    const char *value;
};

class Server {
public:
    Server(simplify::Repository *repository);
    ~Server();

    void AddRoute(const char *name, Action *action);
    void DeleteRoute(const char *name);

    bool Start(int port, const char *repository_dir, const char *document_root);

private:
    static void *Trampoline(mg_event, mg_connection *, mg_request_info *);
    bool Dispatch(mg_event, mg_connection *, mg_request_info *);

private:
    typedef std::unordered_map<const char *, Action *, CharHashFun, CharEqFun>
        RouteMap;

    mg_context *srv_;
    simplify::Repository *repository_;
    RouteMap routes_;
};

}  // namespace simplifyd

#endif  // SIMPLIFYD_SERVER_HH_
