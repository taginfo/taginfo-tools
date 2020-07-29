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
#include "statistics-handler.hpp"
#include "tagstats-handler.hpp"
#include "util.hpp"

#include <getopt.h>

#include <osmium/io/any_input.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

#ifdef __linux__
static const char* default_index_type = "SparseMmapArray";
#else
static const char* default_index_type = "SparseMemArray";
#endif

GeoDistribution::geo_distribution_type GeoDistribution::c_distribution_all;
int GeoDistribution::c_width;
int GeoDistribution::c_height;

static void print_help() {
    std::cout << "taginfo-stats [OPTIONS] OSMFILE DATABASE\n\n" \
              << "This program is part of taginfo. It calculates statistics on OSM tags\n" \
              << "from OSMFILE and puts them into DATABASE (an SQLite database).\n" \
              << "\nOptions:\n" \
              << "  -H, --help                    Print this help message and exit\n" \
              << "  -i, --index=INDEX_TYPE        Set index type for location index (default: " << default_index_type << ")\n" \
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

    std::string index_type_name = default_index_type;

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
                std::exit(0);
            case 'i':
                index_type_name = optarg;
                break;
            case 'I':
                std::cout << "Available index types:\n";
                std::cout << "  DenseMemArray\n";
                std::cout << "  SparseMemArray\n";
#ifdef __linux__
                std::cout << "  DenseMmapArray\n";
                std::cout << "  SparseMmapArray\n";
#endif
                std::exit(0);
            case 's':
                selection_database_name = optarg;
                break;
            case 'm':
                min_tag_combination_count = atoi(optarg);
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
                width = atoi(optarg);
                break;
            case 'h':
                height = atoi(optarg);
                break;
            default:
                std::exit(1);
        }
    }

    if (argc - optind != 2) {
        std::cerr << "Usage: " << argv[0] << " [OPTIONS] OSMFILE DATABASE\n";
        std::exit(1);
    }

    osmium::util::VerboseOutput vout{true};
    vout << "Starting taginfo-stats...\n";

    GeoDistribution::set_dimensions(width, height);
    osmium::io::File input_file{argv[optind]};
    Sqlite::Database db{argv[optind+1], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE}; // NOLINT(hicpp-signed-bitwise)

    MapToInt map_to_int{left, bottom, right, top, width, height};

    const bool better_resolution = (width * height) >= (1U << 16U);
    LocationIndex location_index{index_type_name, better_resolution};
    TagStatsHandler handler{db, selection_database_name, map_to_int, min_tag_combination_count, vout, location_index};

    osmium::io::Reader reader{input_file};
    osmium::apply(reader, handler);

    handler.write_to_database();
}

