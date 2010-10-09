//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

#ifndef SIMPLIFYD_MURMURHASH2_HH_
#define SIMPLIFYD_MURMURHASH2_HH_

#include <string.h>
#include <functional>

namespace simplifyd {

unsigned int MurmurHash2(const void *key, int len, unsigned int seed);

struct CharHashFun : public std::unary_function<const char *, size_t> {
    inline size_t operator ()(const char *s) const {
        return MurmurHash2(s, strlen(s), 1495723221);
    }
};

struct CharEqFun
  : public std::binary_function<const char *, const char *, bool> {
    inline bool operator ()(const char *s1, const char *s2) const {
        return strcmp(s1, s2) == 0;
    }
};

}  // namespace simplifyd

#endif  // SIMPLIFYD_MURMURHASH2_HH_
