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

#include <sys/time.h>
#include <sys/resource.h>

#include <unistd.h>
#include <getopt.h>

#include <stdlib.h>
#include <string.h>

#include <cassert>
#include <iostream>

#include <simplify/repository.hh>

#include <config.h>

#include "articleaction.hh"
#include "contextaction.hh"
#include "searchaction.hh"
#include "server.hh"


namespace simplifyd {

static option g_daemon_options[] = {
    { "port", 1, 0, 0 },
    { "repository", 1, 0, 0 },
    { "html-dir" },
    { "daemonize", 0, 0, 0 },
    { "help", 0, 0, 0 },
    { 0, 0, 0, 0 }
};

static const char *g_daemon_help =
R"#(
simplifyd [options]
Daemon that serves search and browse requests from Simplify applications.

  -p PORT, --port PORT
      Use PORT as the port number for web server. Default: 8000.

  -r REPOSITORY-PATH, --repository REPOSITORY-PATH
      Root directory of the repository from where dictionaries should
      be loaded. If the specified directory does not exist, program will
      attempt to create it.
      Default: ${HOME}/)#" REPOSITORY_SUFFIX R"#(.

  -d HTML-DIR, --html-dir HTML-DIR
      Root directory of the web server. The web server uses this directory
      to serve web resources (html files, images, css stylesheets, etc.).
      Default: )#" WWW_ROOT R"#(

  -b, --background
      Detach and run in background. Default: Run in foreground.

  -h, --help
      Print this help text.
)#";

static void PrintHelpAndExit()
{
    std::cout << g_daemon_help << std::endl;
}

static bool ParseCommandLine(int argc, char *argv[],
                             int &port,
                             char **repository_path,
                             char **html_dir,
                             bool &daemonize)
{
    port = 8000;
    *repository_path = NULL;
    *html_dir = NULL;
    daemonize = false;

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

                port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || port <= 0 || port > 65535) {
                    std::cout << "Invalid port number specified." << std::endl;
                    return false;
                }
                break;
            }
            case 'r': {
                *repository_path = strdup(optarg);
                break;
            }
            case 'd': {
                *html_dir = strdup(optarg);
            }
            case 'b': {
                daemonize = true;
                break;
            }
            case 'h': {
                PrintHelpAndExit();
                break;
            }
        }
    }

    // Set defaults if nothing were specified on the command line.
    if (!*repository_path) {
        char *home = getenv("HOME");
        size_t size = strlen(home) + strlen(REPOSITORY_SUFFIX) + 2;

        *repository_path = static_cast<char *>(malloc(size));
        sprintf(*repository_path, "%s/%s", home, REPOSITORY_SUFFIX);
    }

    if (!*html_dir)
        *html_dir = strdup(WWW_ROOT);

    return true;
}

}  // namespace simplifyd

int main(int argc, char *argv[])
{
    int port;
    char *repository_path;
    char *html_dir;
    bool daemonize;

    if (!simplifyd::ParseCommandLine(argc, argv,
                                     port,
                                     &repository_path,
                                     &html_dir,
                                     daemonize))
        return 1;

    if (daemonize) {
        pid_t pid = fork();

        if (pid == -1) {
            std::cerr << "Failed to fork process: " << strerror(errno);
            return 1;
        } else if (pid != 0) {
            return 0;
        }
    }

    // Increase the size of stack.
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
    simplify::Likely<simplify::Repository *> likely_r =
        simplify::Repository::New(repository_path);

    // Start the web server if we've successfully opened repository.
    if (likely_r) {
        simplifyd::Server server(likely_r);
        server.AddRoute("/context", new simplifyd::ContextAction());
        server.AddRoute("/search", new simplifyd::SearchAction());
        server.AddRoute("/article", new simplifyd::ArticleAction());

        return_code = server.Start(port, repository_path, html_dir) ? 0 : 1;
    } else {
        std::cout << "Unable to open repository '" << repository_path << "': "
                  << likely_r.error_code().message() << "." << std::endl;
        return_code = 1;
    }

    // Free some memory to keep valgrind happy.
    free(repository_path);
    free(html_dir);
    return return_code;
}
