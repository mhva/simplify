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
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <dirent.h>
#else
# include <Windows.h>
#endif

#include <string.h>

#include <algorithm>
#include <string>

#include "config.hh"
#include "epwing-dictionary.hh"
#include "error.hh"

#include "repository.hh"

namespace simplify {

static Likely<Dictionary *> DictionaryFromConfig(Config *conf)
{
    int dict_type;
    if (!(*conf)["Dictionary"].ReadInt32("type", &dict_type))
        return make_error_code(simplify_error::bad_configuration);

    switch (static_cast<DictionaryType>(dict_type)) {
        case DictionaryType::Epwing: {
            Likely<EpwingDictionary *> likely_dict =
                EpwingDictionary::New(conf);

            if (likely_dict)
                return (Dictionary *)likely_dict;
            else
                return likely_dict.error_code();
        }
        default: {
            return make_error_code(simplify_error::index_out_of_range);
        }
    }
}

static Likely<Dictionary *> DictionaryFromDirectory(const std::string &dict_dir)
{
    Likely<Config *> likely_conf =
        Config::New((dict_dir + "/config").c_str());

    if (likely_conf) {
        Likely<Dictionary *> likely_dict = DictionaryFromConfig(likely_conf);

        if (!likely_dict)
            delete *likely_conf;

        return likely_dict;
    } else {
        return likely_conf.error_code();
    }
}

Repository::Repository(const char *repository_path)
    : repository_path_(repository_path)
{
}

Repository::~Repository()
{
    std::for_each(dicts_.begin(), dicts_.end(), [](Dictionary *d) {delete d;});
}

Likely<Dictionary *> Repository::NewDictionary(const char *name,
                                               const char *path,
                                               DictionaryType type)
{
    if (strlen(name) == 0)
        return make_error_code(simplify_error::empty_string);

    std::string dict_dir = repository_path_ + "/" + name;

#if defined(SIMPLIFY_POSIX)
    // Ensure that we are not going to overwrite an existing dictionary by
    // checking whether configuration file exists or not.
    if (access((dict_dir + "/config").c_str(), F_OK) != 0) {
        if (errno != ENOENT)
            return make_error_code(static_cast<std::errc>(errno));
    } else {
        return make_error_code(simplify_error::already_exists);
    }

    // Create a directory for the new dictionary. Do not fail if said
    // directory already exists.
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    if (mkdir(dict_dir.c_str(), mode) != 0 && errno != EEXIST)
        return make_error_code(static_cast<std::errc>(errno));

#else
# error "Repository::NewDictionary() is not implemented."
#endif

    // Create configuration object for dictionary.
    Config *conf;
    Likely<Config *> likely_conf = Config::New(dict_dir + "/config");

    if (likely_conf)
        conf = likely_conf;
    else
        return likely_conf.error_code();

    // Save the dictionary type for later use by the DictionaryFromDirectory()
    // function.
    (*conf)["Dictionary"].WriteInt32("type", static_cast<int>(type));

    // Finally, instantiate the dictionary.
    Dictionary *dict;
    Likely<Dictionary *> likely_dict = DictionaryFromConfig(conf);

    if (!likely_dict) {
        delete conf;
        return likely_dict.error_code();
    }

    dict = likely_dict;
    dict->SetName(name);

    // Flush configuration to the config file.
    std::error_code error =conf->Flush();
    if (!error) {
        return dict;
    } else {
        // Configuration object is owned by dictionary object so it will be
        // deleted by its destructor.
        delete dict;
        return error;
    }
}

void Repository::DeleteDictionary(Dictionary *dict)
{
    // Ensure that the dictionary belongs to this repository.
    auto pos = std::find(dicts_.begin(), dicts_.end(), dict);
    if (pos == dicts_.end())
        return;

    std::string config_file = dict->GetConfig().GetFileName();
    std::string dict_dir = dict->GetConfig().GetEnclosingDirectory();

    dicts_.erase(pos);
    delete dict;

    // Remove configuration file and enclosing directory if it's empty.
#ifdef SIMPLIFY_POSIX
    unlink(config_file.c_str());
    rmdir(dict_dir.c_str());
#else
# error "Repository::DeleteDictionary() is not implemented.";
#endif
}

Repository::DictionaryList::iterator Repository::Begin()
{
    return dicts_.begin();
}

Repository::DictionaryList::const_iterator Repository::Begin() const
{
    return dicts_.begin();
}

Repository::DictionaryList::iterator Repository::End()
{
    return dicts_.end();
}

Repository::DictionaryList::const_iterator Repository::End() const
{
    return dicts_.end();
}

size_t Repository::GetDictionaryCount() const
{
    return dicts_.size();
}

Dictionary *Repository::GetDictionaryByIndex(size_t index)
{
    if (index < dicts_.size())
      return dicts_[index];
    else
      return NULL;
}

const Dictionary *Repository::GetDictionaryByIndex(size_t index) const
{
    if (index < dicts_.size())
      return dicts_[index];
    else
      return NULL;
}

DictionaryType Repository::IdentifyDictionary(const char *path)
{
    return DictionaryType::Epwing;
}

Likely<Repository *> Repository::New(const char *repository_path)
{
#ifdef SIMPLIFY_POSIX
    DIR *dir = opendir(repository_path);

    if (dir == NULL)
        return make_error_code(static_cast<std::errc>(errno));

    dirent *dir_entry;
    Repository *repository = new Repository(repository_path);

    // Each directory in the repository supposed to be a dictionary entry.
    // Read each directory and try to restore a dictionary.
    while ((dir_entry = readdir(dir))) {
        // Ensure that configuration file is present in each directory in the
        // repository.
        std::string dict_dir;
        dict_dir.append(repository_path).append(1, '/') \
                .append(dir_entry->d_name);

        // Ignore current directory if we can't reach configuration file or
        // it doesn't have appropriate permissions.
        if (access((dict_dir + "/config").c_str(), R_OK) != 0)
            continue;

        Likely<Dictionary *> likely_dict = DictionaryFromDirectory(dict_dir);
        if (likely_dict)
            repository->dicts_.push_back(likely_dict);
    }

    closedir(dir);
    return repository;
#else
# error "Repository::New() is not implemented";
#endif
}

}  // namespace simplify
