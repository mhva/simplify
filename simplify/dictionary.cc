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

#include "dictionary.hh"

namespace simplify {

Dictionary::Dictionary(Config *conf) : conf_(conf)
{
}

Dictionary::~Dictionary()
{
    delete conf_;
}

const char *Dictionary::GetName() const
{
    return (*conf_)["Dictionary"].ReadString("name", "");
}

void Dictionary::SetName(const std::string &name)
{
    (*conf_)["Dictionary"].WriteString("name", name);
}

Config &Dictionary::GetConfig()
{
    return *conf_;
}

const Config &Dictionary::GetConfig() const
{
    return *conf_;
}

}  // namespace simplify
