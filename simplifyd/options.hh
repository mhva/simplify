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

#ifndef SIMPLIFYD_OPTIONS_HH_
#define SIMPLIFYD_OPTIONS_HH_

namespace simplifyd {

class Options
{
public:
    Options();
    ~Options();

    void SetPort(int port);
    void SetRepositoryDir(const char *path);
    void SetDaemonize(bool daemonize);
    void SetHtmlDir(const char *path);

    int GetPort() const;
    const char *GetRepositoryDir() const;
    const char *GetHtmlDir() const;
    bool GetDaemonize() const;

private:
    int port_;
    char *repository_dir_;
    char *html_dir_;
    bool daemonize_;
};

}  // namespace simplifyd

#endif /* SIMPLIFYD_OPTIONS_HH_ */
