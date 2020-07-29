#pragma once

#include <cstring>
#include <string>
#include <utility>

/**
 * Hash function used in hash maps that works well with tag
 * key/value strings.
 */
struct djb2_hash {

    std::size_t calc(std::size_t hash, const char *str) const noexcept {
        std::size_t c;

        while ((c = *str++)) {
            hash = ((hash << 5U) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }

    std::size_t operator()(const char *str) const noexcept {
        return calc(5381U, str);
    }

    std::size_t operator()(std::pair<const char *, const char*> p) const {
        auto hash = calc(5381U, p.first);
        hash = calc(hash, "=");
        hash = calc(hash, p.second);
        return hash;
    }

};

/**
 * String comparison used in hash maps.
 */
struct eqstr {
    bool operator()(const char* s1, const char* s2) const noexcept {
        return (s1 == s2) || (s1 && s2 && std::strcmp(s1, s2) == 0);
    }

    bool operator()(std::pair<const char*, const char*> p1,
                    std::pair<const char*, const char*> p2) const noexcept {
        return operator()(p1.first, p2.first) && operator()(p1.second, p2.second);
    }
};

struct strless {
    bool operator()(const char* s1, const char* s2) const noexcept {
        return std::strcmp(s1, s2) < 0;
    }
};

