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

#ifdef SIMPLIFY_POSIX
#include <getopt.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <cassert>
#include <iostream>
#include <string>

#include <simplify/simplify.hh>
#include <simplify/repository.hh>
#include <simplify/epwing/epwing-dictionary.hh>

#include "articleaction.hh"
#include "contextaction.hh"
#include "options.hh"
#include "searchaction.hh"
#include "server.hh"


namespace simplifyd {

static void PrintHelpAndExit()
{
    Options default_options{};

    std::cout << R"#(
Usage: simplifyd [options]

Simplify server daemon.

  -p PORT, --port PORT
      Use PORT as the port number for web server. Default: )#"
        << default_options.GetPort() << R"#(.

  -r CONFIG-PATH, --repository CONFIG-PATH
      Path to repository configuration file.
      Default: )#" << default_options.GetHtmlDir() << R"#(

  -d HTML-DIR, --html-dir HTML-DIR
      Directory containing program's assets.
      Default: )#" << default_options.GetRepositoryConfigPath() << R"#(

  -b, --background
      Detach and run in background. Default: )#"
        << (default_options.GetDaemonize()
            ? "Run in background"
            : "Run in foreground")
        << R"#(.

  -h, --help
      Print this help text and exit.
)#" << std::endl;
    exit(0);
}

static bool ParseCommandLine(int argc, char *argv[], Options &options)
{
    static option g_daemon_options[] = {
        { "port", 1, 0, 'p' },
        { "repository", 1, 0, 'r' },
        { "html-dir", 1, 0, 'd' },
        { "daemonize", 0, 0, 'b' },
        { "help", 0, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    while (true) {
        int argv_index;
        int c = getopt_long(argc, argv,
                            "p:r:d:bh",
                            g_daemon_options,
                            &argv_index);
        if (c == -1)
            break;

        switch (c) {
            case 'p': {
                char *endptr;
                int port = strtol(optarg, &endptr, 10);
                if (*endptr == '\0' && port > 0 && port < 65535) {
                    options.SetPort(port);
                } else {
                    std::cout << "Port number is invalid." << std::endl;
                    return false;
                }
                break;
            }
            case 'r': {
                options.SetRepositoryConfigPath(optarg);
                break;
            }
            case 'd': {
                options.SetHtmlDir(optarg);
                break;
            }
            case 'b': {
                options.SetDaemonize(true);
                break;
            }
            case 'h': {
                PrintHelpAndExit();
                break;
            }
        }
    }

    return true;
}

}  // namespace simplifyd

int main(int argc, char *argv[])
{
    simplify::Initialize();
    simplifyd::Options options;
    if (!simplifyd::ParseCommandLine(argc, argv, options))
        return 1;

    if (options.GetDaemonize()) {
        pid_t pid = fork();

        if (pid == -1) {
            std::cerr << "Failed to fork process: " << strerror(errno);
            return 1;
        } else if (pid != 0) {
            return 0;
        }
    }

    // Increase size of daemon's stack.
    {
        size_t required_size = 32 * 1024 * 1024;
        rlimit limit;
        int status;

        status = getrlimit(RLIMIT_STACK, &limit);
        if (status == 0 && limit.rlim_cur < required_size) {
            limit.rlim_cur = required_size;
            status = setrlimit(RLIMIT_STACK, &limit);
        }

        if (status != 0) {
            std::cerr << "Failed to adjust stack size. Some things "
                      << "may not work properly." << std::endl;
        }
    }

    int return_code;
    auto likely_r = simplify::Repository::New(options.GetRepositoryConfigPath());

    // Start the web server if we've successfully opened repository.
    if (likely_r) {
        simplifyd::Server server(likely_r);
        server.AddRoute("/context", new simplifyd::ContextAction());
        server.AddRoute("/search", new simplifyd::SearchAction());
        server.AddRoute("/article", new simplifyd::ArticleAction());

        return_code = server.Start(options) ? 0 : 1;
    } else {
        std::cout << "Unable to open repository '"
                  << options.GetRepositoryConfigPath() << "': "
                  << likely_r.error_code().message() << "." << std::endl;
        return_code = 1;
    }

    return return_code;
}
