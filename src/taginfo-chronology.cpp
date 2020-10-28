/*

  Copyright (C) 2012-2020 Jochen Topf <jochen@topf.org>.

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

#include "version.hpp"

#include <sqlite.hpp>

#include <osmium/diff_handler.hpp>
#include <osmium/diff_visitor.hpp>
#include <osmium/index/nwr_array.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/diff_object.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>

#include <absl/container/flat_hash_map.h>

#include <getopt.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

static constexpr const std::size_t seconds_in_a_day = 60 * 60 * 24;

static void print_help() {
    std::cout << "taginfo-chronology [OPTIONS] OSMFILE DATABASE\n\n" \
              << "This program is part of taginfo. It calculates statistics on OSM tags\n" \
              << "from the OSM history file OSMFILE and puts them into DATABASE (an SQLite database).\n" \
              << "\nOptions:\n" \
              << "  -H, --help                    Print this help message and exit\n" \
              << "  -s, --selection-db=DATABASE   Name of selection database\n";
}

class chronology_store {

    // Start from 2005-01-01 (that's 35 years + 9 leap days from 1970-01-01)
    static constexpr const std::size_t offset_days = 365 * 35 + 9;

    osmium::nwr_array<std::vector<int32_t>> m_changes;

    // Number of days we store from today back to 2005-01-01
    std::size_t count = (std::time(nullptr) / seconds_in_a_day) + 1 - offset_days;

    int32_t nodes(std::size_t n) const noexcept {
        const auto& v = m_changes(osmium::item_type::node);
        return v.empty() ? 0 : v[n];
    }

    int32_t ways(std::size_t n) const noexcept {
        const auto& v = m_changes(osmium::item_type::way);
        return v.empty() ? 0 : v[n];
    }

    int32_t relations(std::size_t n) const noexcept {
        const auto& v = m_changes(osmium::item_type::relation);
        return v.empty() ? 0 : v[n];
    }

    bool any(std::size_t n) const noexcept {
        return nodes(n) != 0 || ways(n) != 0 || relations(n) != 0;
    }

public:

    void update(osmium::item_type type, std::size_t day, int32_t change) {
        auto& v = m_changes(type);
        if (v.empty()) {
            v.resize(count);
        }
        const auto idx = day - offset_days;
        v[idx] += change;
    }

    void write(Sqlite::Statement& stmt, const std::string& key) const {
        std::vector<int32_t> out;

        for (std::size_t i = 0; i < count; ++i) {
            if (any(i)) {
                out.push_back(static_cast<int32_t>(i + offset_days));
                out.push_back(nodes(i));
                out.push_back(ways(i));
                out.push_back(relations(i));
            }
        }

        stmt.bind_text(key)
            .bind_blob(out.data(), static_cast<int>(out.size() * sizeof(int32_t)))
            .execute();
    }

    void write(Sqlite::Statement& stmt, const std::pair<std::string, std::string>& tag) const {
        std::vector<int32_t> out;

        for (std::size_t i = 0; i < count; ++i) {
            if (any(i)) {
                out.push_back(static_cast<int32_t>(i + offset_days));
                out.push_back(nodes(i));
                out.push_back(ways(i));
                out.push_back(relations(i));
            }
        }

        stmt.bind_text(tag.first)
            .bind_text(tag.second)
            .bind_blob(out.data(), static_cast<int>(out.size() * sizeof(int32_t)))
            .execute();
    }

}; // class chronology_store

class Handler : osmium::diff_handler::DiffHandler {

    osmium::util::VerboseOutput& m_vout;
    absl::flat_hash_map<std::string, chronology_store> m_keys;
    absl::flat_hash_map<std::pair<std::string, std::string>, chronology_store> m_tags;

public:

    Handler(osmium::util::VerboseOutput& vout, const std::string &selection_database_name) :
        m_vout(vout) {
        if (!selection_database_name.empty()) {
            m_vout << "Opening selection database: " << selection_database_name << '\n';
            Sqlite::Database sdb{selection_database_name, SQLITE_OPEN_READONLY};

            Sqlite::Statement select{sdb, "SELECT key, value FROM frequent_tags;"};
            int n = 0;
            while (select.read()) {
                const auto key   = select.get_text_ptr(0);
                const auto value = select.get_text_ptr(1);
                m_tags.emplace(std::pair<std::string, std::string>{key, value}, chronology_store{});
                ++n;
            }
            m_vout << "  got " << n << " tags\n";
        }
    }

    void object(const osmium::DiffObject& object) {
        if (object.curr().deleted()) {
            return;
        }

        const auto sday = object.start_time().seconds_since_epoch() / seconds_in_a_day;
        const auto eday = object.end_time().seconds_since_epoch() / seconds_in_a_day;

        if (sday == eday) {
            return;
        }

        const auto type = object.type();

        for (const auto& tag: object.curr().tags()) {
            auto& store = m_keys[tag.key()];
            store.update(type, sday, 1);
            if (!object.last()) {
                store.update(type, eday, -1);
            }

            if (!m_tags.empty()) {
                auto it = m_tags.find(std::pair<std::string, std::string>{tag.key(), tag.value()});
                if (it != m_tags.end()) {
                    it->second.update(type, sday, 1);
                    if (!object.last()) {
                        it->second.update(type, eday, -1);
                    }
                }
            }
        }
    }

    void node(const osmium::DiffNode& node) {
        object(node);
    }

    void way(const osmium::DiffWay& way) {
        object(way);
    }

    void relation(const osmium::DiffRelation& rel) {
        object(rel);
    }

    void write(Sqlite::Database& db) const {
        {
            Sqlite::Statement statement_insert{db,
                "INSERT INTO keys_chronology (key, data) VALUES (?, ?);"};

            for (const auto& hist : m_keys) {
                hist.second.write(statement_insert, hist.first);
            }
        }
        if (!m_tags.empty()) {
            Sqlite::Statement statement_insert{db,
                "INSERT INTO tags_chronology (key, value, data) VALUES (?, ?, ?);"};

            for (const auto& hist : m_tags) {
                hist.second.write(statement_insert, hist.first);
            }
        }
    }

}; // class Handler

int main(int argc, char* argv[]) {
    static const option long_options[] = {
        {"help",         no_argument,       nullptr, 'H'},
        {"selection-db", required_argument, nullptr, 's'},
        {nullptr, 0, nullptr, 0}
    };

    std::string selection_database_name;

    while (true) {
        int c = getopt_long(argc, argv, "Hs:", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'H':
                print_help();
                return 0;
            case 's':
                selection_database_name = optarg;
                break;
            default:
                return 1;
        }
    }

    if (argc - optind != 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE DATABASE\n";
        return 1;
    }

    try {
        osmium::util::VerboseOutput vout{true};
        vout << "Starting taginfo-chronology...\n";
        vout << "  " << get_taginfo_tools_version() << '\n';
        vout << "  " << get_libosmium_version() << '\n';

        osmium::io::File input_file{argv[optind]};
        Sqlite::Database db{argv[optind + 1], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE}; // NOLINT(hicpp-signed-bitwise)
        db.exec("PRAGMA journal_mode = OFF;");
        db.exec("PRAGMA synchronous  = OFF;");

        osmium::io::Reader reader{input_file};
        if (! reader.header().has_multiple_object_versions()) {
            std::cerr << "Input file is not an OSM history file!\n";
            return 2;
        }

        Handler handler{vout, selection_database_name};

        vout << "Processing input file...\n";
        osmium::apply_diff(reader, handler);

        vout << "Writing database...\n";
        handler.write(db);

        osmium::MemoryUsage mcheck;
        vout << "\n"
            << "Actual memory usage:\n"
            << "  current: " << mcheck.current() << "MB\n"
            << "  peak:    " << mcheck.peak() << "MB\n";

        vout << "Done.\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    }

    return 0;
}

