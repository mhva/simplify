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

#include <utility>

#include "repository.hh"
#include "dictionary.hh"

namespace simplify {

Dictionary::Dictionary(const char *name) : name_(name) {}
Dictionary::Dictionary(std::string name) : name_(std::move(name)) {}
Dictionary::~Dictionary() {}

const char *Dictionary::GetName() const
{
    return name_.c_str();
}

void Dictionary::SetName(const std::string &name)
{
    name_ = name;

    // Dictionary's name is part of permanent state.
    SaveState();
}

std::shared_ptr<Repository> Dictionary::GetRepository()
{
    return repo_.lock();
}

std::shared_ptr<const Repository> Dictionary::GetRepository() const
{
    return repo_.lock();
}

void Dictionary::SetRepository(std::shared_ptr<Repository> repo)
{
    repo_ = repo;
}

std::error_code Dictionary::SaveState()
{
    if (auto repo = GetRepository(); repo != nullptr)
        return repo->SaveState();
    return simplify_error::success;
}

}  // namespace simplify
