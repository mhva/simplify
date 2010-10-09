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

#ifndef LIBSIMPLIFY_CONFIG_HH_
#define LIBSIMPLIFY_CONFIG_HH_

#include <string>
#include <system_error>
#include <vector>

#include <simplify/likely.hh>

namespace simplify {

class Config {
public:
    class ConfigSection {
    public:
        /**
         * Returns name of this config section.
         */
        std::string GetSectionName() const;

        /**
         * Checks whether the config section is dirty or not.
         *
         * To flush changes use the @Config::Flush() method.
         *
         * \return Returns true if the config section contains unflushed
         * changes; otherwise false.
         */
        bool IsDirty() const;

        /**
         * Checks whenether the given @key exists in the configuration section.
         *
         * \return Returns true if the key exists; otherwise false.
         */
        bool IsKeyExists(const char *key) const;

        /**
         * Reads a string associated with the given @key and stores a pointer
         * to it into @ptr.
         *
         * \return Returns true if the string was successfully read; false is
         *  returned when the given key does not exist.
         */
        bool ReadString(const char *key, const char **ptr) const;

        /**
         * Reads a string associated with the given @key and stores its contents
         * into @buf.
         *
         * \return Returns true if the string was successfully read; false is
         *  returned when the given key does not exist.
         */
        bool ReadString(const char *key, std::string &buf) const;

        /**
         * Reads an integer associated with the given @key and stores it into
         * the buffer pointed by @buf.
         *
         * \return Returns true if the integer was successfully read; false is
         *  returned when the given key does not exist.
         */
        bool ReadInt32(const char *key, int *buf) const;

        /**
         * Reads a boolean value associated with the given @key and stores it
         * into the buffer pointed by @buf.
         *
         * \return Returns true if the boolean was successfully read; false is
         *  returned when the given key does not exist.
         */
        bool ReadBoolean(const char *key, bool *buf) const;

        /**
         * Reads a string associated with the given @key, in the case when
         * the key does not exist, function uses @fallback as return value.
         *
         * \return If the key exists, returns a string value associated with
         *  the given key; if the key does not exist, the value of @fallback is
         *  returned instead.
         */
        const char *ReadString(const char *key, const char *fallback) const;

        /**
         * Reads an integer associated with the given @key, in the case when
         * the key does not exist, function uses @fallback as return value.
         *
         * \return If the key exists, returns an integer value associated with
         *  the given key; if the key does not exist, the value of @fallback is
         *  returned instead.
         */
        int ReadInt32(const char *key, int fallback) const;

        /**
         * Reads an boolean value associated with the given @key, in the case
         * when the key does not exist, function uses @fallback as return value.
         *
         * \return If the key exists, returns a boolean value associated with
         *  the given key; if the key does not exist, the value of @fallback is
         *  returned instead.
         */
        bool ReadBoolean(const char *key, bool fallback) const;

        ConfigSection &WriteString(const char *key, const std::string &val);
        ConfigSection &WriteString(const char *key, const char *val, size_t sz);
        ConfigSection &WriteInt32(const char *key, int val);
        ConfigSection &WriteBoolean(const char *key, bool val);

    private:
        typedef std::vector<std::pair<std::string, std::string>> Options;
        friend class ::simplify::Config;

        ConfigSection(const std::string &section_name);
        ~ConfigSection();

        Options::iterator LocateOption(const char *);
        Options::const_iterator LocateOption(const char *) const;

        void SetDirty(bool dirty);

    private:
        Options options_;
        std::string section_name_;
        bool dirty_;
    };

public:
    Config(const char *filename);
    ~Config();

    /**
     * Checks whether the configuration section with specified name exists
     * or not.
     */
    bool IsSectionExists(const char *section_name) const;

    /**
     * Returns filename of the configuration file.
     */
    std::string GetFileName() const;

    /**
     * Returns path to enclosing directory.
     */
    std::string GetEnclosingDirectory() const;

    /**
     * Flushes configuration to the configuration to file.
     */
    std::error_code Flush();

    /**
     * Returns a configuration section with specified name.
     *
     * NOTE: If the section doesn't exist, a reference to the newly
     *  created section will be returned.
     */
    ConfigSection &GetSection(const char *section_name);

    /**
     * Returns a configuration section with specified name.
     *
     * Alias to the @GetSection() method.
     */
    ConfigSection &operator [](const char *section_name);

    static Likely<Config *> New(const char *config_filename);
    static Likely<Config *> New(const std::string &config_filename);

private:
    typedef std::vector<ConfigSection *> Sections;

    Sections::iterator LocateSection(const char *);
    Sections::const_iterator LocateSection(const char *) const;

private:
    std::string filename_;
    Sections sections_;
};

}  // namespace simplify

#endif  // LIBSIMPLIFY_CONFIG_HH_
