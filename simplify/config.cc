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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <typeinfo>

#ifdef POSIX
# include <unistd.h>
#endif

#include "error.hh"
#include "likely.hh"
#include "utils.hh"

#include "config.h"
#include "config.hh"

namespace simplify {

/**
 * Escapes special characters in source string and stores the result in buffer.
 */
static std::string EscapeString(const std::string &string)
{
    static char hex[] = "0123456789abcdef";
    std::string buffer;

    buffer.reserve(string.size() * 1.2f);

    for (size_t i = 0; i < string.size(); ++i) {
        unsigned char c = string[i];

        if (isalnum(c) || isblank(c) || ispunct(c)) {
            if (c != '\\')
                buffer.append(1, c);
            else
                buffer.append("\\\\");
        } else {
            char enc[] = { '\\', hex[(c & ~0xf) >> 4], hex[c & 0xf] };
            buffer.append(enc, sizeof(enc));
        }
    }

    return buffer;
}

/**
 * Un-escapes a string and stores result into buffer.
 *
 * Possible errors:
 *  simplify_errors::invalid_syntax Source string has malformed escape sequence.
 */
static bool UnescapeString(const char *source,
                           size_t source_length,
                           std::string &buffer,
                           std::error_code &error)
{
    error.clear();
    buffer.clear();

    size_t offset = 0;
    std::string result;

    buffer.reserve(source_length);

    // Lambda to map a single hex digit to an integer.
    static auto to_int = [](unsigned char c) -> int {
        if (isdigit(c))
            return tolower(c) - '0';
        else /* if (isalpha(c)) */
            return 10 + (tolower(c) - 'a');
    };

    while (offset < source_length) {
        // Just append the character to result if it's not an escape sequence.
        if (source[offset] != '\\') {
            buffer.append(1, source[offset]);
            ++offset;
            continue;
        }

        // Fail if we've stumbled upon an escape sequence but it's incomplete.
        if (offset + 1 >= source_length) {
            error = make_error_code(simplify_error::invalid_syntax);
            return false;
        }

        // Check if the escape sequence represents '\'.
        if (source[offset + 1] == '\\') {
            buffer.append(1, '\\');
            offset += 2;
            continue;
        }

        // Ensure that the escape sequence has 2 hex digits in it.
        if (offset + 2 >= source_length || !isxdigit(source[offset + 1]) ||
            !isxdigit(source[offset + 2])) {
            error = make_error_code(simplify_error::invalid_syntax);
            return false;
        }

        char c = to_int(source[offset + 1]) * 16 + to_int(source[offset + 2]);
        buffer.append(1, c);

        offset += 3;
    }

    return true;
}

Config::ConfigSection::ConfigSection(const std::string &section_name)
    : section_name_(section_name),
      dirty_(false)
{
}

Config::ConfigSection::~ConfigSection()
{
}

std::string Config::ConfigSection::GetSectionName() const
{
    return section_name_;
}

bool Config::ConfigSection::IsDirty() const
{
    return dirty_;
}

bool Config::ConfigSection::ReadString(const char *key, const char **ptr) const
{
    auto it = LocateOption(key);

    if (it != options_.end()) {
        *ptr = (*it).second.c_str();
        return true;
    } else {
        return false;
    }
}

bool Config::ConfigSection::ReadString(const char *key, std::string &buf) const
{
    auto it = LocateOption(key);

    if (it != options_.end()) {
        buf = (*it).second;
        return true;
    } else {
        return false;
    }
}

bool Config::ConfigSection::ReadInt32(const char *key, int *buf) const
{
    const char *str;

    if (ReadString(key, &str)) {
        *buf = atoi(str);
        return true;
    } else {
        return false;
    }
}

bool Config::ConfigSection::ReadBoolean(const char *key, bool *buf) const
{
    const char *str;

    if (!ReadString(key, &str))
        return false;

    if (strcmp(str, "true") == 0) {
        *buf = true;
        return true;
    } else if (strcmp(str, "false")) {
        *buf = false;
        return true;
    } else {
        return false;
    }
}

const char *Config::ConfigSection::ReadString(const char *key,
                                              const char *fallback) const
{
    const char *str;
    return ReadString(key, &str) ? str : fallback;
}

int Config::ConfigSection::ReadInt32(const char *key, int fallback) const
{
    int i32;
    return ReadInt32(key, &i32) ? i32 : fallback;
}

bool Config::ConfigSection::ReadBoolean(const char *key, bool fallback) const
{
    bool b;
    return ReadBoolean(key, &b) ? b : fallback;
}

Config::ConfigSection &Config::ConfigSection::WriteString(const char *key,
                                                         const std::string &val)
{
    auto it = LocateOption(key);

    dirty_ = true;

    if (it != options_.end()) {
        (*it).second = val;
        return *this;
    } else {
        options_.push_back(std::make_pair(key, val));
        return *this;
    }
}

Config::ConfigSection &Config::ConfigSection::WriteString(const char *key,
                                                          const char *val,
                                                          size_t sz)
{
    return WriteString(key, std::string(val, sz));
}

Config::ConfigSection &Config::ConfigSection::WriteInt32(const char *key,
                                                         int i32)
{
    std::ostringstream oss;
    oss << i32;
    return WriteString(key, oss.str());
}

Config::ConfigSection &Config::ConfigSection::WriteBoolean(const char *key,
                                                           bool val)
{
    return WriteString(key, val ? "true" : "false");
}

Config::ConfigSection::Options::iterator
Config::ConfigSection::LocateOption(const char *k)
{
    for (auto it = options_.begin(); it != options_.end(); ++it) {
        if ((*it).first == k)
            return it;
    }

    return options_.end();
}

Config::ConfigSection::Options::const_iterator
Config::ConfigSection::LocateOption(const char *k) const
{
    for (auto it = options_.begin(); it != options_.end(); ++it) {
        if ((*it).first == k)
            return it;
    }

    return options_.end();
}

void Config::ConfigSection::SetDirty(bool dirty)
{
    dirty_ = dirty;
}

Config::Config(const char *filename) : filename_(filename)
{
}

Config::~Config()
{
    Flush();

    std::for_each(sections_.begin(), sections_.end(),
                  [](ConfigSection *s) { delete s; });
}

std::string Config::GetFileName() const
{
    return filename_;
}

std::string Config::GetEnclosingDirectory() const
{
#if defined(POSIX)
    size_t slash_offset = filename_.rfind('/');
    assert(slash_offset != std::string::npos);

    return filename_.substr(0, slash_offset);
#else
#endif
}

bool Config::IsSectionExists(const char *section_name) const
{
    return LocateSection(section_name) != sections_.end();
}

std::error_code Config::Flush()
{
    std::string tmp_filename;

#if defined(POSIX)
    // On POSIX systems we write to backup file to later use atomic replace
    // after we've finised writing to guarantee that the config file is always
    // consistent.
    tmp_filename.append(".~").append(filename_);
#else
    // Write directly to the config file on Windows. Atomic replace is
    // not supported on Windows XP and earlier.
    tmp_filename = filename_;
#endif

    // Check if there are any dirty config sections.
    static auto is_dirty = [](ConfigSection *s) { return s->IsDirty(); };
    auto pos = std::find_if(sections_.begin(), sections_.end(), is_dirty);

    // If there are no dirty config sections we are done.
    if (pos == sections_.end())
        return std::error_code();

    FILE *file = fopen(tmp_filename.c_str(), "wb");
    if (!file)
        return make_error_code(static_cast<std::errc>(errno));

    // Write config sections to disk.
    for (auto sect = sections_.begin(); sect != sections_.end(); ++sect) {
        if (fprintf(file, "[%s]\n", (*sect)->GetSectionName().c_str()) <= 0)
            goto error_cleanup;

        auto &options = (*sect)->options_;

        for (auto opt = options.begin(); opt != options.end(); ++opt) {
            const char *key = (*opt).first.c_str();
            std::string val = EscapeString((*opt).second);

            if (fprintf(file, "%s=%s\n", key, val.c_str()) <= 0)
                goto error_cleanup;
        }

        // Put a newline before the next section header.
        if (sect + 1 != sections_.end()) {
            if (fputc('\n', file) == EOF)
                goto error_cleanup;
        }
    }

    if (fflush(file) == EOF)
        goto error_cleanup;

#ifdef POSIX
    // Do an atomic replace to guarantee consistency.

    // XXX: On Mac OS X fsync() doesn't block until all data has been flushed
    // on disk so there's still a possibility of data loss on this platform.
    // Should we call fcntl(..., F_FULLFSYNC) instead?
    {
        int fd = fileno(file);

        if (fsync(fd) != 0 || fclose(file) != 0 ||
            rename(tmp_filename.c_str(), filename_.c_str()))
            goto error_cleanup;
    }
#else
    if (fclose(file) != 0)
        goto error_cleanup;
#endif

    // Mark all sections as clean since we've successfully flushed config to
    // disk.
    static auto undirtify = [](ConfigSection *s) { s->SetDirty(false); };
    std::for_each(sections_.begin(), sections_.end(), undirtify);

    return std::error_code();

error_cleanup:
    fclose(file);
    return make_error_code(static_cast<std::errc>(errno));
}

Config::ConfigSection &Config::GetSection(const char *section_name)
{
    auto it = LocateSection(section_name);

    if (it != sections_.end()) {
        return **it;
    } else {
        sections_.push_back(new ConfigSection(section_name));
        return *sections_.back();
    }
}

Config::ConfigSection &Config::operator [](const char *section_name)
{
    return GetSection(section_name);
}

Likely<Config *> Config::New(const char *config_filename)
{
    std::error_code error;

    auto file_data = ReadFile(config_filename, error);
    if (file_data.first == NULL)
        return error;

    Config *conf = new Config(config_filename);
    ConfigSection *current_section = NULL;

    char *buffer = file_data.first;
    size_t length = file_data.second;
    size_t line_start = 0;
    size_t line_end = 0;

    while (line_end < length) {
        // Find end of the current line.
        while (line_end < length && buffer[line_end] != '\n')
            ++line_end;

        // Check if the current line is empty.
        bool is_empty = true;
        for (size_t i = line_start; i < line_end; ++i) {
            if (!isblank(buffer[i])) {
                is_empty = false;
                break;
            }
        }

        char *assign_ptr;

        if (is_empty || buffer[line_start] == '#') {
            // Ignore line since it's empty or contains a full-line comment.
        } else if (buffer[line_start] == '[' && buffer[line_end - 1] == ']') {
            // Start a new config section if we've encountered a section header.
            if (current_section != NULL && !current_section->options_.empty()) {
                conf->sections_.push_back(current_section);
            } else {
                // Delete section since it's empty and we won't insert into the
                // configuration object.
                delete current_section;
            }

            size_t name_start = line_start + 1;
            size_t name_length = line_end - line_start - 2;
            current_section =
               new ConfigSection(std::string(buffer + name_start, name_length));
        } else if ((assign_ptr = static_cast<char *>(
                        memchr(buffer + line_start, '=', line_end - line_start))
                    ) != NULL) {
            // Encountered a key-value pair.
            size_t assign_offset = assign_ptr - (buffer + line_start);

            // Ensure that assignment does not start outside a section and
            // the key name is not empty (empty values are allowed).
            if (current_section == NULL || line_start == assign_offset) {
                error = make_error_code(simplify_error::invalid_syntax);
                goto error_cleanup;
            }

            ConfigSection::Options::value_type keyval;
            const char *key = buffer + line_start;
            const char *val = buffer + line_start + assign_offset + 1;
            size_t key_length = assign_offset;
            size_t val_length = line_end - line_start - assign_offset - 1;

            keyval.first.assign(key, key_length);

            // Try to un-escape value.
            if (!UnescapeString(val, val_length, keyval.second, error)) {
                error = make_error_code(simplify_error::invalid_syntax);
                goto error_cleanup;
            }

            current_section->options_.push_back(keyval);
        } else {
            error = make_error_code(simplify_error::invalid_syntax);
            goto error_cleanup;
        }

        // Switch to next line.
        line_start = line_end = line_end + 1;
    }

    if (current_section != NULL && !current_section->options_.empty())
        conf->sections_.push_back(current_section);

    free(buffer);
    return conf;

error_cleanup:
    delete current_section;
    delete conf;
    free(buffer);
    return error;
}

Likely<Config *> Config::New(const std::string &config_filename)
{
    return Config::New(config_filename.c_str());
}

Config::Sections::iterator Config::LocateSection(const char *s)
{
    for (auto it = sections_.begin(); it != sections_.end(); ++it) {
        if ((*it)->GetSectionName() == s)
            return it;
    }

    return sections_.end();
}

Config::Sections::const_iterator Config::LocateSection(const char *s) const
{
    for (auto it = sections_.begin(); it != sections_.end(); ++it) {
        if ((*it)->GetSectionName() == s)
            return it;
    }

    return sections_.end();
}

}  // namespace simplify
