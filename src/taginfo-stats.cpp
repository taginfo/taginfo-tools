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

#include "geodistribution.hpp"
#include "tagstats-handler.hpp"
#include "util.hpp"
#include "version.hpp"

#include <sqlite.hpp>

#include <osmium/diff_handler.hpp>
#include <osmium/diff_visitor.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#include <getopt.h>

#include <string>

GeoDistribution::geo_distribution_type GeoDistribution::c_distribution_all;
unsigned int GeoDistribution::c_width;
unsigned int GeoDistribution::c_height;

static void print_help() {
    std::cout << "taginfo-stats [OPTIONS] OSMFILE DATABASE\n\n" \
              << "This program is part of taginfo. It calculates statistics on OSM tags\n" \
              << "from OSMFILE and puts them into DATABASE (an SQLite database).\n" \
              << "\nOptions:\n" \
              << "  -H, --help                    Print this help message and exit\n" \
              << "  -i, --index=INDEX_TYPE        Set index type for location index (default: FlexMem)\n" \
              << "  -I, --show-index-types        Show available index types for location index\n" \
              << "  -m, --min-tag-combination-count=N  Tag combinations not appearing this often\n" \
              << "                                     are not written to database\n" \
              << "  -s, --selection-db=DATABASE   Name of selection database\n" \
              << "  -t, --top=NUMBER              Top of bounding box for distribution images\n" \
              << "  -r, --right=NUMBER            Right of bounding box for distribution images\n" \
              << "  -b, --bottom=NUMBER           Bottom of bounding box for distribution images\n" \
              << "  -l, --left=NUMBER             Left of bounding box for distribution images\n" \
              << "  -w, --width=NUMBER            Width of distribution images (default: 360)\n" \
              << "  -h, --height=NUMBER           Height of distribution images (default: 180)\n" \
              << "\nDefault for bounding box is: (-180, -90, 180, 90).\n";
}

class LastVersionHandler : public osmium::diff_handler::DiffHandler {

    TagStatsHandler& m_handler;

public:

    LastVersionHandler(TagStatsHandler& handler) noexcept : m_handler(handler) {
    }

    void node(const osmium::DiffNode& node) {
        if (node.last() && node.curr().visible()) {
            m_handler.node(node.curr());
        }
    }

    void way(const osmium::DiffWay& way) {
        if (way.last() && way.curr().visible()) {
            m_handler.way(way.curr());
        }
    }

    void relation(const osmium::DiffRelation& relation) {
        if (relation.last() && relation.curr().visible()) {
            m_handler.relation(relation.curr());
        }
    }

}; // LastVersionHandler

int main(int argc, char* argv[]) {
    static const option long_options[] = {
        {"help",                      no_argument,       nullptr, 'H'},
        {"index",                     required_argument, nullptr, 'i'},
        {"show-index-types",          no_argument,       nullptr, 'I'},
        {"min-tag-combination-count", required_argument, nullptr, 'm'},
        {"selection-db",              required_argument, nullptr, 's'},
        {"top",                       required_argument, nullptr, 't'},
        {"right",                     required_argument, nullptr, 'r'},
        {"bottom",                    required_argument, nullptr, 'b'},
        {"left",                      required_argument, nullptr, 'l'},
        {"width",                     required_argument, nullptr, 'w'},
        {"height",                    required_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    unsigned int min_tag_combination_count = 1000;

    std::string selection_database_name;

    std::string index_type_name{"FlexMem"};

    double top    =   90.0;
    double right  =  180.0;
    double bottom =  -90.0;
    double left   = -180.0;

    unsigned int width  = 360;
    unsigned int height = 180;

    while (true) {
        int c = getopt_long(argc, argv, "Hi:Im:s:t:r:b:l:w:h:", long_options, nullptr);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'H':
                print_help();
                return 0;
            case 'i':
                index_type_name = optarg;
                break;
            case 'I':
                std::cout << "Available index types:\n";
                std::cout << "  FlexMem (default)\n";
                std::cout << "  DenseMemArray\n";
                std::cout << "  SparseMemArray\n";
#ifdef __linux__
                std::cout << "  DenseMmapArray\n";
                std::cout << "  SparseMmapArray\n";
#endif
                return 0;
            case 's':
                selection_database_name = optarg;
                break;
            case 'm':
                min_tag_combination_count = get_uint(optarg);
                break;
            case 't':
                top = get_coordinate(optarg, 90.0);
                break;
            case 'r':
                right = get_coordinate(optarg, 180.0);
                break;
            case 'b':
                bottom = get_coordinate(optarg, 90.0);
                break;
            case 'l':
                left = get_coordinate(optarg, 180.0);
                break;
            case 'w':
                width = get_uint(optarg);
                break;
            case 'h':
                height = get_uint(optarg);
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
        vout << "Starting taginfo-stats...\n";
        vout << "  " << get_taginfo_tools_version() << '\n';
        vout << "  " << get_libosmium_version() << '\n';

        GeoDistribution::set_dimensions(width, height);
        osmium::io::File input_file{argv[optind]};
        Sqlite::Database db{argv[optind + 1], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE}; // NOLINT(hicpp-signed-bitwise)

        MapToInt map_to_int{left, bottom, right, top, width, height};

        const bool better_resolution = (width * height) >= (1U << 16U);
        LocationIndex location_index{index_type_name, better_resolution};

        osmium::io::Reader reader{input_file};
        const bool is_history = reader.header().has_multiple_object_versions();

        if (is_history) {
            vout << "Input file is an OSM history file\n";
        } else {
            vout << "Input file is an OSM data file\n";
        }

        TagStatsHandler tagstats_handler{db, selection_database_name, map_to_int, min_tag_combination_count, vout, location_index};
        LastVersionHandler handler{tagstats_handler};

        osmium::apply_diff(reader, handler);

        tagstats_handler.write_to_database();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    }

    return 0;
}

