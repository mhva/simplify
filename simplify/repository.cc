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

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <string>

#include <nowide/fstream.hpp>
#include <nlohmann/json.hpp>

#include "epwing/epwing-dictionary.hh"
#include "error.hh"
#include "utils.hh"

#include "repository.hh"

namespace simplify {

static void to_json(nlohmann::json &json, DictionaryType t) {
    switch (t) {
    case DictionaryType::Epwing:
        json = "epwing";
        break;
    default:
        assert(!"Trying to serialize an unknown dictionary type");
        json = "unknown";
        break;
    }
}

static void to_json(nlohmann::json &json, Dictionary &d) {
    switch (d.GetType()) {
    case DictionaryType::Epwing: {
        auto epwing = reinterpret_cast<EpwingDictionary *>(&d);
        json = nlohmann::json{
            {"name", d.GetName()},
            {"type", d.GetType()},
            {"state", epwing}
        };
        break;
    }
    default:
        assert(!"Trying to serialize unsupported dictionary type");
    }
}

static Likely<Dictionary *> EpwingDictionaryFromConfig(const nlohmann::json &j)
{
    using json = nlohmann::json;

    auto path = j["path"].get_ref<const json::string_t &>();
    auto name = j["name"].get_ref<const json::string_t &>();
    Likely<EpwingDictionary *> maybe_dict;

    if (auto state = j["state"]; state.is_object()) {
        maybe_dict = EpwingDictionary::NewWithState(
            name.c_str(),
            path.c_str(),
            state
        );
    } else {
        // No state sub-key => create bare-bones instance.
        maybe_dict = EpwingDictionary::New(name.c_str(), path.c_str(), nullptr);
    }

    if (maybe_dict) {
        return *maybe_dict;
    } else {
        return maybe_dict.error_code();
    }
}

static Likely<Repository::DictionaryList>
    RestoreDictionaryList(std::ifstream &stream)
{
    using json = nlohmann::json;
    Repository::DictionaryList result;

    try {
        json root;
        stream >> root;

        auto dicts = root["dicts"].get_ref<const json::array_t &>();
        for (auto &dict : dicts) {
            auto type = dict["type"].get_ref<const json::string_t &>();
            Likely<Dictionary *> maybe_dict;

            if (StreqCaseFold(type, "epwing")) {
                maybe_dict = EpwingDictionaryFromConfig(dict);
            } else {
                // FIXME: report invalid dictionary type.
                continue;
            }

            if (!maybe_dict) {
                // FIXME: inappropriate error code.
                return make_error_code(simplify_error::bad_configuration);
            }
            result.push_back(std::shared_ptr<Dictionary>(*maybe_dict));
        }
    } catch (...) {
        return make_error_code(simplify_error::bad_configuration);
    }
    return std::move(result);
}

Repository::Repository(const char *config_path, DictionaryList &&dicts)
    : config_path_(config_path)
    , dicts_(std::move(dicts))
{
}

Repository::~Repository()
{
}

std::error_code Repository::SaveState()
{
    nlohmann::json root;
    for (auto &dict : this->dicts_) {
        root["dicts"].push_back(*dict);
    }

    nowide::ofstream stream(config_path_);
    if (!stream) {
        // FIXME: bad error code.
        return simplify::bad_configuration;
    }

    try {
        stream << std::setw(4) << root;
    } catch (...) {
        // FIXME: bad error code.
        return simplify::bad_configuration;
    }

    return make_error_code(simplify_error::success);
}

void Repository::AddDictionary(std::unique_ptr<Dictionary> d)
{
    // It's important to pass repository instance to dictionary for it to be
    // able to notify repository about state changes.
    d->SetRepository(shared_from_this());

    dicts_.push_back(std::move(d));
    SaveState();
}

void Repository::RemoveDictionary(size_t pos)
{
    RemoveDictionary(dicts_.begin() + pos);
}

void Repository::RemoveDictionary(DictionaryList::iterator it)
{
    (*it)->SetRepository(nullptr);

    dicts_.erase(it);
    SaveState();
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

std::shared_ptr<Dictionary> Repository::GetDictionary(size_t pos)
{
    if (pos < dicts_.size())
      return dicts_[pos];
    else
      return nullptr;
}

std::shared_ptr<const Dictionary> Repository::GetDictionary(size_t pos) const
{
    if (pos < dicts_.size())
      return dicts_[pos];
    else
      return nullptr;
}

Likely<std::shared_ptr<Repository>> Repository::New(const char *config_path)
{
    // Open and parse config. Use nowide to handle Windows business.
    auto stream = nowide::ifstream(config_path);
    if (!stream) {
        auto *r = new Repository(config_path, DictionaryList{});
        return std::shared_ptr<Repository>(r);
    }

    auto maybe_list = RestoreDictionaryList(stream);
    if (maybe_list) {
        auto r = new Repository(config_path, std::move(*maybe_list));
        auto sr = std::shared_ptr<Repository>(r);

        // Pass new repository instance to each dictionary.
        for (auto &dict : r->dicts_) {
            dict->SetRepository(sr);
        }
        return sr;
    }
    return maybe_list.error_code();
}

}  // namespace simplify
