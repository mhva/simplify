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

#include <string.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>

#include <simplify/repository.hh>

#include "action.hh"
#include "httpquery.hh"
#include "httpresponse.hh"
#include "server.hh"


namespace simplifyd {

static Server *g_server_instance = NULL;

Server::Server(simplify::Repository *repository) :
    srv_(NULL),
    repository_(repository),
    routes_(100)
{
}

Server::~Server()
{
    std::for_each(routes_.begin(), routes_.end(), [](RouteMap::value_type &v) {
        delete v.second;
    });
    delete repository_;
}

void Server::AddRoute(const char *name, Action *action)
{
    auto it = routes_.find(name);
    if (it == routes_.end()) {
        routes_.insert(std::make_pair(name, action));
    } else {
        // Replace the action if the route did already exist.
        delete (*it).second;
        (*it).second = action;
    }
}

void Server::DeleteRoute(const char *name)
{
    auto it = routes_.find(name);
    if (it != routes_.end()) {
        delete (*it).second;
        routes_.erase(it);
    }
}

bool Server::Start(int port,
                   const char *repository_dir,
                   const char *document_root)
{
    // Ensure that only one server is running.
    assert(g_server_instance == NULL);
    char str_port[21];

    sprintf(str_port, "%u", port);

    const char *options[] = {
        "listening_ports", str_port,
        "document_root", document_root,
        NULL
    };

    // Start receiving requests.
    g_server_instance = this;
    srv_ = mg_start(&Server::Trampoline, options);
    g_server_instance = NULL;

    if (srv_)
        return true;
    else
        return false;
}

void *Server::Trampoline(mg_event event,
                         mg_connection *conn,
                         mg_request_info *req)
{
    return g_server_instance->Dispatch(event, conn, req) ? conn : NULL;
}

bool Server::Dispatch(mg_event event,
                      mg_connection *conn,
                      mg_request_info *req)
{
    if (event != MG_NEW_REQUEST)
        return false;


    // Find an appropriate action for the given path and let an associated
    // Action object handle the event.
    auto it = routes_.find(req->uri);
    if (it != routes_.end()) {
        HttpQuery query(*req);
        HttpResponse response;

        (*it).second->Handle(*repository_, query, response);

        std::string text = response.ProduceResponse();
        mg_write(conn, text.data(), text.size());
        return true;
    } else {
        return false;
    }
}

}  // namespace simplifyd
