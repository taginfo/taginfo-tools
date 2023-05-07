/*

  Copyright (C) 2012-2022 Jochen Topf <jochen@topf.org>.

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

#include "util.hpp"
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

// Due to database format changes on that date, the OSM history data dump
// does not contain object versions before 2007-10-07. So we simply start
// our statistics on that date. This is the offset from 1970-01-01.
static constexpr const std::size_t offset_days = 13793;

// Number of days we store from today back to 2007-10-07
static std::size_t const count = (std::time(nullptr) / seconds_in_a_day) + 1 - offset_days;

static void print_help() {
    std::cout << "taginfo-chronology [OPTIONS] OSMFILE DATABASE\n\n" \
              << "This program is part of taginfo. It calculates statistics on OSM tags\n" \
              << "from the OSM history file OSMFILE and puts them into DATABASE (an SQLite database).\n" \
              << "\nOptions:\n" \
              << "  -H, --help                    Print this help message and exit\n" \
              << "  -s, --selection-db=DATABASE   Name of selection database\n";
}

class chronology_store {

    osmium::nwr_array<std::vector<int32_t>> m_changes;

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

    std::size_t bytes_used() const noexcept {
        return sizeof(int32_t) * m_changes(osmium::item_type::node).size() +
               sizeof(int32_t) * m_changes(osmium::item_type::way).size() +
               sizeof(int32_t) * m_changes(osmium::item_type::relation).size();
    }

    void update(osmium::item_type type, std::size_t day, int32_t change) {
        auto& v = m_changes(type);
        if (v.empty()) {
            v.resize(count);
        }

        if (day <= offset_days) {
            v[0] += change;
        } else {
            const auto idx = day - offset_days;
            v[idx] += change;
        }
    }

    void write(Sqlite::Statement& stmt, const std::string& key) const {
        std::vector<int32_t> out;

        int first_use = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (any(i)) {
                if (first_use == 0) {
                    first_use = i + offset_days;
                }
                out.push_back(static_cast<int32_t>(i + offset_days));
                out.push_back(nodes(i));
                out.push_back(ways(i));
                out.push_back(relations(i));
            }
        }

        stmt.bind_text(key)
            .bind_blob(out.data(), static_cast<int>(out.size() * sizeof(int32_t)))
            .bind_int(first_use * 60 * 60 * 24)
            .execute();
    }

    void write(Sqlite::Statement& stmt, const std::pair<std::string, std::string>& tag) const {
        std::vector<int32_t> out;

        int first_use = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (any(i)) {
                if (first_use == 0) {
                    first_use = i + offset_days;
                }
                out.push_back(static_cast<int32_t>(i + offset_days));
                out.push_back(nodes(i));
                out.push_back(ways(i));
                out.push_back(relations(i));
            }
        }

        stmt.bind_text(tag.first)
            .bind_text(tag.second)
            .bind_blob(out.data(), static_cast<int>(out.size() * sizeof(int32_t)))
            .bind_int(first_use * 60 * 60 * 24)
            .execute();
    }

}; // class chronology_store

class Handler : osmium::diff_handler::DiffHandler {

    osmium::util::VerboseOutput& m_vout;
    absl::flat_hash_map<std::string, chronology_store> m_keys;
    absl::flat_hash_map<std::pair<std::string, std::string>, chronology_store> m_tags;

    osmium::Timestamp m_max_timestamp{};
    std::size_t m_count_nodes = 0;
    std::size_t m_count_ways = 0;
    std::size_t m_count_relations = 0;
    std::size_t m_count_visible_nodes = 0;
    std::size_t m_count_visible_ways = 0;
    std::size_t m_count_visible_relations = 0;

    void object(const osmium::DiffObject& object) {
        if (m_max_timestamp < object.curr().timestamp()) {
            m_max_timestamp = object.curr().timestamp();
        }

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

public:

    Handler(osmium::util::VerboseOutput& vout, const std::string &selection_database_name) :
        m_vout(vout) {
        if (!selection_database_name.empty()) {
            m_vout << "Opening selection database: " << selection_database_name << '\n';
            Sqlite::Database sdb{selection_database_name, SQLITE_OPEN_READONLY};

            Sqlite::Statement select{sdb, "SELECT key, value FROM frequent_tags;"};
            int n = 0;
            while (select.read()) {
                const auto *const key   = select.get_text_ptr(0);
                const auto *const value = select.get_text_ptr(1);
                m_tags.emplace(std::pair<std::string, std::string>{key, value}, chronology_store{});
                ++n;
            }
            m_vout << "  got " << n << " tags\n";
        }
    }

    void node(const osmium::DiffNode& node) {
        ++m_count_nodes;
        if (node.curr().visible()) {
            ++m_count_visible_nodes;
        }
        object(node);
    }

    void way(const osmium::DiffWay& way) {
        ++m_count_ways;
        if (way.curr().visible()) {
            ++m_count_visible_ways;
        }
        object(way);
    }

    void relation(const osmium::DiffRelation& relation) {
        ++m_count_relations;
        if (relation.curr().visible()) {
            ++m_count_visible_relations;
        }
        object(relation);
    }

    void write(Sqlite::Database& db) const {
        {
            Sqlite::Statement statement_update_meta{db, "UPDATE source SET data_until=?"};
            statement_update_meta.bind_text(time_string(m_max_timestamp)).execute();
        }
        {
            Sqlite::Statement stmt{db, "INSERT INTO stats (key, value) VALUES (?, ?)"};
            stmt.bind_text("chronology_num_nodes").bind_int64(m_count_nodes).execute();
            stmt.bind_text("chronology_num_visible_nodes").bind_int64(m_count_visible_nodes).execute();
            stmt.bind_text("chronology_num_ways").bind_int64(m_count_ways).execute();
            stmt.bind_text("chronology_num_visible_ways").bind_int64(m_count_visible_ways).execute();
            stmt.bind_text("chronology_num_relations").bind_int64(m_count_relations).execute();
            stmt.bind_text("chronology_num_visible_relations").bind_int64(m_count_visible_relations).execute();
        }
        {
            std::size_t bytes_keys = 0;

            Sqlite::Statement statement_insert{db,
                "INSERT INTO keys_chronology (key, data, first_use) VALUES (?, ?, ?);"};

            for (const auto& hist : m_keys) {
                bytes_keys += hist.second.bytes_used();
                hist.second.write(statement_insert, hist.first);
            }

            m_vout << "Key counters needed " << (bytes_keys / (1024*1024)) << " MBytes\n";
        }

        std::size_t bytes_tags = 0;
        if (!m_tags.empty()) {
            Sqlite::Statement statement_insert{db,
                "INSERT INTO tags_chronology (key, value, data, first_use) VALUES (?, ?, ?, ?);"};

            for (const auto& hist : m_tags) {
                bytes_tags += hist.second.bytes_used();
                hist.second.write(statement_insert, hist.first);
            }
        }

        m_vout << "Tag counters needed " << (bytes_tags / (1024*1024)) << " MBytes\n";
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

