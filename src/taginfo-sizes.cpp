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
#include "string-store.hpp"
#include "tagstats-handler.hpp"

#include <iostream>

int main() {
    std::cout << "sizeof(Counter32).............................. = " << sizeof(Counter32) << '\n';
    std::cout << "sizeof(GeoDistribution)........................ = " << sizeof(GeoDistribution) << '\n';
    std::cout << "sizeof(KeyStats)............................... = " << sizeof(KeyStats) << '\n';
    std::cout << "sizeof(KeyValueStats).......................... = " << sizeof(KeyValueStats) << '\n';
    std::cout << "sizeof(StringStore)............................ = " << sizeof(StringStore) << '\n';
    std::cout << "sizeof(key_hash_map_type)...................... = " << sizeof(key_hash_map_type) << '\n';
    std::cout << "sizeof(value_hash_map_type).................... = " << sizeof(value_hash_map_type) << '\n';
    std::cout << "sizeof(key_value_hash_map_type)................ = " << sizeof(key_value_hash_map_type) << '\n';
    std::cout << "sizeof(key_combination_hash_map_type).......... = " << sizeof(combination_hash_map_type) << '\n';
    std::cout << "sizeof(key_value_geodistribution_hash_map_type) = " << sizeof(key_value_geodistribution_hash_map_type) << '\n';
    std::cout << "sizeof(user_hash_map_type)..................... = " << sizeof(user_hash_map_type) << '\n';
}

