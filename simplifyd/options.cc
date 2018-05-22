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

#include <cstdio>
#include <cstdlib>

#include <filesystem>
#include <stdexcept>

#include "options.hh"

namespace simplifyd
{

Options::Options()
    : port_(8000),
      html_dir_(SIMPLIFY_WWWROOT),
      daemonize_(false)
{
    std::filesystem::path config_dir_path;

#if defined(SIMPLIFY_DARWIN)
    if (home != nullptr) {
        config_dir_path = getenv("HOME");
        config_dir_path.append("Library").append("Simplify");
    }
#elif defined(SIMPLIFY_WINDOWS)
    // FIXME: use win32 API to retrieve LOCALAPPDATA path, getenv() is not
    // unicode-aware.
    if (getenv("LOCALAPPDATA") != nullptr) {
        config_dir_path = getenv("LOCALAPPDATA");
        config_dir_path.append("Simplify");
    }
#else
    if (getenv("XDG_CONFIG_HOME") != nullptr) {
        config_dir_path = getenv("XDG_CONFIG_HOME");
        config_dir_path.append("simplify");
    } else if (getenv("HOME") != nullptr) {
        config_dir_path = getenv("HOME");
        config_dir_path.append(".config");
        config_dir_path.append("simplify");
    }
#endif

    if (config_dir_path.empty()) {
        if (getenv("HOME") != nullptr) {
            config_dir_path = getenv("HOME");
            config_dir_path.append(".simplify");
        } else {
            throw std::runtime_error("Unable to determine config path");
        }
    }

    std::filesystem::path repository_path = config_dir_path;
    repository_path.append("repository.js");

    repository_config_ = repository_path.string();
}

Options::~Options()
{
}

void Options::SetPort(int port)
{
    port_ = port;
}

void Options::SetRepositoryConfigPath(const char *path)
{
    repository_config_ = path;
}

void Options::SetHtmlDir(const char *path)
{
    html_dir_ = path;
}

void Options::SetDaemonize(bool daemonize)
{
    daemonize_ = daemonize;
}

int Options::GetPort() const
{
    return port_;
}

const char *Options::GetConfigDir() const
{
    return config_dir_.c_str();
}

const char *Options::GetRepositoryConfigPath() const
{
    return repository_config_.c_str();
}

const char *Options::GetHtmlDir() const
{
    return html_dir_.c_str();
}

bool Options::GetDaemonize() const
{
    return daemonize_;
}

}  // namespace simplifyd
