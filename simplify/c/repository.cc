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

#include <stdlib.h>

#include <simplify/repository.hh>

#include "error.h"
#include "repository.h"

using namespace simplify;

struct simplify_repository {
    Repository *repository;
    Repository::DictionaryList::iterator fetch_pointer;
};


int simplify_repository_identify(const char *path)
{
    DictionaryType dt = Repository::IdentifyDictionary(path);
    return static_cast<int>(dt);
}

simplify_repository_t *simplify_repository_new(const char *repository_path)
{
    Likely<Repository *> likely_repo = Repository::New(repository_path);

    if (likely_repo) {
        simplify_repository_t *sr = static_cast<simplify_repository_t *>(
                malloc(sizeof(simplify_repository_t)));

        sr->repository = *likely_repo;
        return sr;
    } else {
        simplify_set_error_code(likely_repo.error_code());
        return NULL;
    }
}

void simplify_repository_destroy(simplify_repository_t *sr)
{
    delete sr->repository;
    free(sr);
}

simplify_dictionary_t *
simplify_repository_new_dictionary(simplify_repository_t *sr,
                                   const char *name,
                                   const char *path,
                                   int type)
{
    Likely<Dictionary *> likely_dict =
        sr->repository->NewDictionary(name, path, (DictionaryType)type);

    if (likely_dict) {
        return likely_dict;
    } else {
        simplify_set_error_code(likely_dict.error_code());
        return NULL;
    }
}

void simplify_repository_delete_dictionary(simplify_repository_t *sr,
                                           simplify_dictionary_t *sd)
{
    sr->repository->DeleteDictionary(reinterpret_cast<Dictionary *>(sd));
}

void simplify_repository_begin_fetch(simplify_repository_t *sr)
{
    sr->fetch_pointer = sr->repository->Begin();
}

simplify_dictionary_t *simplify_repository_fetch_one(simplify_repository_t *sr)
{
    if (sr->fetch_pointer != sr->repository->End())
        return *sr->fetch_pointer++;
    else
        return NULL;
}
