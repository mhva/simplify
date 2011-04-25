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

#include <cassert>
#include <algorithm>

#include "httpresponse.hh"

namespace simplifyd {

struct HttpHeader {
    char *name;
    char *value;
};

static HttpHeader *NewHeader(const char *name, size_t name_length,
                             const char *value, size_t value_length)
{
    // We pack header into one big memory chunk. Headers aren't very
    // frequently modified so we can improve effectiveness by avoiding
    // some memory allocations.
    struct MemoryLayout {
        HttpHeader header;
        char appendix[];
    } *memory;

    memory = static_cast<MemoryLayout *>(
            malloc(sizeof(MemoryLayout) + name_length + value_length + 2));

    memory->header.name = memory->appendix;
    memory->header.value = memory->appendix + name_length + 1;
    memcpy(memory->header.name, name, name_length + 1);
    memcpy(memory->header.value, value, value_length + 1);

    return &memory->header;
}

static void DeleteHeader(HttpHeader *header)
{
    free(header);
}

static const char *GetStatusCodeText(HttpStatusCode status_code)
{
    switch (status_code) {
        case HttpStatusCode::Ok:
            return "OK";
        default:
            return "Invalid Status Code";
    }
}

static size_t Uitoa10(size_t v, char *buffer)
{
    char *out = buffer;
    do *out++ = '0' + v % 10; while (v /= 10);
    std::reverse(buffer, out);
    return out - buffer;
}

HttpResponse::HttpResponse()
  : http_version_(HttpVersion::Http1_1),
    status_code_(HttpStatusCode::Ok)
{
    headers_.reserve(6);
    body_.reserve(128 * 1024);
}

HttpResponse::~HttpResponse()
{
    // Free memory allocated for headers.
    std::for_each(headers_.begin(), headers_.end(), [](HttpHeader *h) {
        DeleteHeader(h);
    });
}

void HttpResponse::SetHttpVersion(HttpVersion version)
{
    http_version_ = version;
}

void HttpResponse::SetStatus(HttpStatusCode status_code)
{
    status_code_ = status_code;
}

void HttpResponse::AddHeader(const char *name, const char *value)
{
    headers_.push_back(NewHeader(name, strlen(name), value, strlen(value)));
}

void HttpResponse::OverrideHeader(const char *name, const char *value)
{
    // Check if header with the given name already exists.
    auto it = std::find_if(headers_.begin(), headers_.end(),
        [name](HttpHeader *h) {
            return strcmp(h->name, name) == 0;
        }
    );

    // Replace header's value if it already exists, or create a new header
    // otherwise.
    if (it != headers_.end()) {
        size_t value_length = strlen(value);

        if (value_length <= strlen((*it)->value)) {
            // We don't need to reallocate the header because the given
            // header's value can fit in the existing chunk.
            memcpy((*it)->value, value, value_length + 1);
        } else {
            // New value cannot fit into the existing memory chunk.
            DeleteHeader(*it);
            *it = NewHeader(name, strlen(name), value, value_length);
        }
    } else {
        // Header with the given name doesn't exist. Create a new one.
        AddHeader(name, value);
    }
}

std::string &HttpResponse::GetBody()
{
    return body_;
}

const std::string &HttpResponse::GetBody() const
{
    return body_;
}

std::string HttpResponse::ProduceResponse() const
{
    char number_buffer[30];
    size_t number_length;
    std::string response;
    response.reserve(headers_.size() * 32 + body_.size());

    // Format the Status-Line.
    //
    // Append HTTP version.
    switch (http_version_) {
        case HttpVersion::Http1_0:
            response.append("HTTP/1.0 ");
            break;
        case HttpVersion::Http1_1:
            response.append("HTTP/1.1 ");
            break;
    }

    // Append Status-Code.
    number_length = Uitoa10(static_cast<size_t>(status_code_), number_buffer);
    response.append(number_buffer, number_length) \
            .append(1, ' ')
            .append(GetStatusCodeText(status_code_)) \
            .append("\r\n");

    // Format headers.
    //
    // Append user headers.
    for (auto it = headers_.begin(); it != headers_.end(); ++it) {
        response.append((*it)->name).append(": ") \
                .append((*it)->value).append("\r\n");
    }

    // Append the Content-Length header.
    number_length = Uitoa10(body_.size(), number_buffer);
    response.append("Content-Length: ") \
            .append(number_buffer, number_length) \
            .append("\r\n\r\n");

    // Append body.
    response.append(body_);

    return response;
}

}  // namespace simplifyd
