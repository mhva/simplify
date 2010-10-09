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

#ifndef SIMPLIFYD_HTTPRESPONSE_HH_
#define SIMPLIFYD_HTTPRESPONSE_HH_

#include <vector>

namespace simplifyd {

enum class HttpStatusCode {
    Ok = 200
};

enum class HttpVersion {
    Http1_0,
    Http1_1
};

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void SetHttpVersion(HttpVersion version);
    void SetStatus(HttpStatusCode status_code);

    void AddHeader(const char *name, const char *value);
    void OverrideHeader(const char *name, const char *value);

    std::string &GetBody();
    const std::string &GetBody() const;

    std::string ProduceResponse() const;

private:
    struct HttpHeader {
        char *name;
        char *value;
    };

    static HttpHeader *NewHeader(const char *name,
                                 size_t name_length,
                                 const char *value,
                                 size_t value_length);
    static void DeleteHeader(HttpHeader *header);

private:
    HttpVersion http_version_;
    HttpStatusCode status_code_;
    std::vector<HttpHeader *> headers_;
    std::string body_;
};

}  // namespace simplifyd

#endif /* SIMPLIFYD_HTTPRESPONSE_HH_ */
