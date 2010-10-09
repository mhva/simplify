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
#include <string>
#include <system_error>

#include <simplify/error.hh>

#include "error.h"

// TODO: Mark as thread_local once g++ supports this attribute.
static __thread int g_error_code = 0;
static __thread const std::error_category *g_error_category = NULL;
static __thread char g_error_message_buffer[4096];
static __thread char g_error_category_buffer[128];

static void copy_message(const std::string &source, char *buffer, size_t size)
{
    assert(buffer != NULL && size > 0);

    size_t to_copy = std::min(source.size(), size - 1);
    memcpy(buffer, source.data(), to_copy);
    buffer[to_copy] = '\0';
}

void simplify_set_error_code(const std::error_code &error)
{
    g_error_code = error.value();
    g_error_category = &error.category();
}

int simplify_error_code()
{
    if (g_error_category == &simplify::simplify_category())
        return g_error_code;
    else
        return SIMPLIFY_EUNKNOWN;
}

const char *simplify_error_message()
{
    if (g_error_category != NULL)
        copy_message(g_error_category->message(g_error_code),
                     g_error_message_buffer,
                     sizeof(g_error_message_buffer));
    else
        strcpy(g_error_message_buffer, "Success");

    return g_error_message_buffer;
}

const char *simplify_error_category()
{
    if (g_error_category != NULL)
        copy_message(g_error_category->name(),
                     g_error_category_buffer,
                     sizeof(g_error_category_buffer));
    else
        strcpy(g_error_category_buffer, "simplify");

    return g_error_category_buffer;
}
