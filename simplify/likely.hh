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

#ifndef LIBSIMPLIFY_LIKELY_HH_
#define LIBSIMPLIFY_LIKELY_HH_

#include <stdexcept>
#include <system_error>
#include <simplify/error.hh>

namespace simplify {

template<typename T>
class Likely {
public:
    inline Likely() : error_(make_error_code(simplify_error::empty_likely)) {}
    inline Likely(T &&value) : value_(std::move(value)) {}
    inline Likely(const T &value) : value_(value) {}
    inline Likely(const std::error_code &error) : error_(error) {}

    inline bool is_error() const {
      return (bool) error_;
    }

    inline T &value_checked() {
      return value_;
    }

    inline std::error_code &error_code() {
        return error_;
    }

    inline T &operator *() {
        if (!error_)
            return value_;
        else
            throw std::runtime_error("Unchecked error.");
    }

    inline operator T &() {
        return operator *();
    }

    explicit inline operator bool() {
        return !is_error();
    }

private:
    T value_;
    std::error_code error_;
};

template<typename T>
class Likely<T *> {
public:
    inline Likely() : error_(make_error_code(simplify_error::empty_likely)) {}
    inline Likely(T *value) : value_(value) {}
    inline Likely(const std::error_code &error) : error_(error) {}

    inline bool is_error() const {
      return (bool) error_;
    }

    inline T *value_checked() {
      return value_;
    }

    inline std::error_code &error_code() {
        return error_;
    }

    inline operator bool() {
        return !is_error();
    }

    inline T *operator *() {
        if (!error_)
            return value_;
        else
            throw std::runtime_error("Unchecked error.");
    }

    inline operator T *() {
        return operator *();
    }

private:
    T *value_;
    std::error_code error_;
};

}  // namespace simplify

#endif /* LIBSIMPLIFY_LIKELY_HH_ */
