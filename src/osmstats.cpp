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

#include "statistics-handler.hpp"

#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>

#include <exception>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " OSMFILE DATABASE\n";
        return 1;
    }

    try {
        osmium::io::File input_file{argv[1]};

        Sqlite::Database db{argv[2], SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE}; // NOLINT(hicpp-signed-bitwise)
        db.exec("CREATE TABLE stats (key TEXT, value INT64);");

        StatisticsHandler handler{db};
        osmium::io::Reader reader{input_file};
        osmium::apply(reader, handler);

        handler.write_to_database();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    }

    return 0;
}

