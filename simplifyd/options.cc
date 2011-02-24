/*
   Copyright (C) 2011 Anton Mihalyov <anton@bytepaper.com>

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
#include <cstdio>
#include <cstdlib>
#include "options.hh"

namespace simplifyd
{

static char *GetDefaultRepositoryDir()
{
    const char *suffix = "repository";
    char *home = getenv("HOME");
    if (home == NULL)
        return NULL;

    char *dir = static_cast<char *>(malloc(strlen(home) +
                                           strlen(SIMPLIFY_CONFIG_PATH) +
                                           sizeof(suffix) + 3));
    sprintf(dir, "%s/%s/%s", home, SIMPLIFY_CONFIG_PATH, suffix);
    return dir;
}

static char *GetDefaultHtmlDir()
{
    return strdup(SIMPLIFY_WWWROOT);
}

Options::Options()
    : port_(8000),
      repository_dir_(GetDefaultRepositoryDir()),
      html_dir_(GetDefaultHtmlDir()),
      daemonize_(false)
{
}

Options::~Options()
{
    free(repository_dir_);
    free(html_dir_);
}

void Options::SetPort(int port)
{
    port_ = port;
}

void Options::SetRepositoryDir(const char *path)
{
    free(repository_dir_);
    repository_dir_ = strdup(path);
}

void Options::SetHtmlDir(const char *path)
{
    free(html_dir_);
    html_dir_ = strdup(path);
}

void Options::SetDaemonize(bool daemonize)
{
    daemonize_ = daemonize;
}

int Options::GetPort() const
{
    return port_;
}

const char *Options::GetRepositoryDir() const
{
    return repository_dir_;
}

const char *Options::GetHtmlDir() const
{
    return html_dir_;
}

bool Options::GetDaemonize() const
{
    return daemonize_;
}

}  // namespace simplifyd
