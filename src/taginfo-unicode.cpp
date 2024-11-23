/*

  Copyright (C) 2012-2024 Jochen Topf <jochen@topf.org>.

  This file is part of Taginfo Tools.

  Taginfo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Taginfo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Taginfo.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <sqlite.hpp>

#include <unicode/schriter.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

static const char* category_to_string(int8_t category) noexcept {
    switch (category) {
        // letters
        case  1: return "Lu"; // uppercase letter
        case  2: return "Ll"; // lowercase letter
        case  3: return "Lt"; // titlecase letter
        case  4: return "Lm"; // modifier letter
        case  5: return "Lo"; // other letter
        // marks
        case  6: return "Mn"; // non-spacing mark
        case  7: return "Me"; // enclosing mark
        case  8: return "Mc"; // combining spacing mark
        // numbers
        case  9: return "Nd"; // decimal digit number
        case 10: return "Nl"; // letter number
        case 11: return "No"; // other number
        // separators
        case 12: return "Zs"; // space separator
        case 13: return "Zl"; // line separator
        case 14: return "Zp"; // paragraph separator
        // control characters etc.
        case 15: return "Cc"; // control char
        case 16: return "Cf"; // format char
        case 17: return "Co"; // private use char
        case 18: return "Cs"; // surrogate
        // punctuations
        case 19: return "Pd"; // dash punctuation
        case 20: return "Ps"; // start punctuation
        case 21: return "Pe"; // end punctuation
        case 22: return "Pc"; // connector punctuation
        case 23: return "Po"; // other punctuation
        // symbols
        case 24: return "Sm"; // math symbol
        case 25: return "Sc"; // currency symbol
        case 26: return "Sk"; // modifier symbol
        case 27: return "So"; // other symbol
        // punctuations cont.
        case 28: return "Pi"; // initial punctuation
        case 29: return "Pf"; // final punctuation
        default:
            return "UNKNOWN";
    }
}

static bool is_plain(const char* t) noexcept {
    for (; *t; ++t) {
        if (!std::isalnum(*t) && *t != '_' && *t != ':' && *t != ' ' && *t != '.' && *t != '-') {
            return false;
        }
    }
    return true;
}

static bool is_unusual(const icu::UnicodeString& us) {
    for (icu::StringCharacterIterator it{us}; it.hasNext(); it.next()) {
        const UChar32 codepoint = it.current32();
        const int8_t chartype = u_charType(codepoint);
        if (! u_isprint(codepoint)) {
            return true;
        }
        if (u_charDirection(codepoint) != 0) {
            return true;
        }
        if (chartype !=  1 && // UPPERCASE_LETTER
            chartype !=  2 && // LOWERCASE_LETTER
            chartype !=  9 && // DECIMAL_DIGIT_NUMBER
            chartype != 12 && // SPACE_SEPARATOR
            chartype != 19 && // DASH_PUNCTUATION
            chartype != 22 && // CONNECTOR_PUNCTUATION
            chartype != 23) { // OTHER_PUNCTUATION
            return true;
        }
    }
    return false;
}

static void get_unicode_info(const char* text, Sqlite::Statement& insert) {
    if (is_plain(text)) {
        return;
    }

    const auto us = icu::UnicodeString::fromUTF8(text);
    if (!is_unusual(us)) {
        return;
    }

    int num = 0;
    for (icu::StringCharacterIterator it{us}; it.hasNext(); it.next(), ++num) {
        const UChar32 codepoint = it.current32();

        const int8_t chartype = u_charType(codepoint);

        std::array<char, 100> buffer{};
        UErrorCode errorCode = U_ZERO_ERROR;
        u_charName(codepoint, U_UNICODE_CHAR_NAME, buffer.begin(), buffer.size(), &errorCode);

        const UCharDirection direction = u_charDirection(codepoint);
        const int32_t block = u_getIntPropertyValue(codepoint, UCHAR_BLOCK);

        const icu::UnicodeString ustr{codepoint};
        std::string str;
        ustr.toUTF8String(str);

        std::array<char, 10> uplus{};
        snprintf(uplus.begin(), uplus.size(), "U+%04x", codepoint);

        insert.
            bind_text(text).
            bind_int(num).
            bind_text(str).
            bind_text(uplus.cbegin()).
            bind_int(block).
            bind_text(category_to_string(chartype)).
            bind_int(direction).
            bind_text(buffer.cbegin()).
            execute();
    }
}

static void find_unicode_info(const char* begin, const char* end, Sqlite::Statement& insert) {
    for (; begin != end; begin += std::strlen(begin) + 1) {
        get_unicode_info(begin, insert);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "taginfo-unicode DATABASE\n";
        return 1;
    }

    try {
        std::string data;

        Sqlite::Database db{argv[1], SQLITE_OPEN_READWRITE};
        Sqlite::Statement select{db, "SELECT key FROM keys WHERE characters IS NULL OR characters NOT IN ('plain', 'colon') ORDER BY key"};
        while (select.read()) {
            data += select.get_text_ptr(0);
            data += '\0';
        }


        Sqlite::Statement insert{db, "INSERT INTO key_characters (key, num, utf8, codepoint, block, category, direction, name) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"};
        db.begin_transaction();
        find_unicode_info(data.c_str(), data.c_str() + data.size(), insert);
        db.commit();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    }

    return 0;
}

