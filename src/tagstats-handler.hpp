#pragma once

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

#include "geodistribution.hpp"
#include "hash.hpp"
#include "statistics-handler.hpp"
#include "string-store.hpp"

#include <osmium/handler.hpp>
#include <osmium/index/map/dense_mem_array.hpp>
#include <osmium/index/map/dense_mmap_array.hpp>
#include <osmium/index/map/flex_mem.hpp>
#include <osmium/index/map/sparse_mem_array.hpp>
#include <osmium/index/map/sparse_mmap_array.hpp>
#include <osmium/index/nwr_array.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>

#include <absl/container/flat_hash_map.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>

/**
 * Stores the location of nodes. Lookup is by node ID.
 *
 * Locations are stored with reduced resolution, either in 16 bit or 32 bit.
 * The bool better_resolution on the constructor decides which is used.
 */
class LocationIndex {

    template <typename T>
    using map_type = osmium::index::map::Map<osmium::unsigned_object_id_type, T>;

    std::unique_ptr<map_type<uint16_t>> m_location_index_16bit{nullptr};
    std::unique_ptr<map_type<uint32_t>> m_location_index_32bit{nullptr};

    template <typename T>
    static std::unique_ptr<map_type<T>> create_map(const std::string& location_index_type) {
        osmium::index::register_map<osmium::unsigned_object_id_type, T, osmium::index::map::DenseMemArray>("FlexMem");
        osmium::index::register_map<osmium::unsigned_object_id_type, T, osmium::index::map::DenseMemArray>("DenseMemArray");
        osmium::index::register_map<osmium::unsigned_object_id_type, T, osmium::index::map::SparseMemArray>("SparseMemArray");
#ifdef __linux__
        osmium::index::register_map<osmium::unsigned_object_id_type, T, osmium::index::map::DenseMmapArray>("DenseMmapArray");
        osmium::index::register_map<osmium::unsigned_object_id_type, T, osmium::index::map::SparseMmapArray>("SparseMmapArray");
#endif
        const auto& map_factory = osmium::index::MapFactory<osmium::unsigned_object_id_type, T>::instance();
        return map_factory.create_map(location_index_type);
    }

public:

    LocationIndex(const std::string& index_type_name, bool better_resolution) {
        if (better_resolution) {
            m_location_index_32bit = create_map<uint32_t>(index_type_name);
        } else {
            m_location_index_16bit = create_map<uint16_t>(index_type_name);
        }
    }

    void set(osmium::unsigned_object_id_type id, uint32_t value) {
        if (value == std::numeric_limits<uint32_t>::max()) {
            return;
        }
        if (m_location_index_16bit) {
            assert(value <= std::numeric_limits<uint16_t>::max());
            m_location_index_16bit->set(id, static_cast<uint16_t>(value));
        } else {
            m_location_index_32bit->set(id, value);
        }
    }

    uint32_t get(osmium::unsigned_object_id_type id) const {
        return m_location_index_16bit ? static_cast<uint32_t>(m_location_index_16bit->get(id))
                                      : m_location_index_32bit->get(id);
    }

    size_t size() const noexcept {
        return m_location_index_16bit ? m_location_index_16bit->size()
                                      : m_location_index_32bit->size();
    }

    size_t used_memory() const noexcept {
        return m_location_index_16bit ? m_location_index_16bit->used_memory()
                                      : m_location_index_32bit->used_memory();
    }

}; // class LocationIndex


/**
 * Holds some counter for nodes, ways, and relations.
 */
template <typename T>
class Counter {

    osmium::nwr_array<T> m_count{};

public:

    T count(osmium::item_type type) const noexcept {
        return m_count(type);
    }

    void set_count(osmium::item_type type, T value) noexcept {
        m_count(type) = value;
    }

    void incr(osmium::item_type type) noexcept {
        ++m_count(type);
    }

    T nodes() const noexcept {
        return m_count(osmium::item_type::node);
    }

    T ways() const noexcept {
        return m_count(osmium::item_type::way);
    }

    T relations() const noexcept {
        return m_count(osmium::item_type::relation);
    }

    T all() const noexcept {
        return nodes() + ways() + relations();
    }

}; // struct Counter

using Counter32 = Counter<uint32_t>;
using Counter64 = Counter<uint32_t>;

using value_hash_map_type = absl::flat_hash_map<const char*, Counter32, djb2_hash, eqstr>;

using user_hash_map_type = absl::flat_hash_map<osmium::user_id_type, uint32_t>;

using combination_hash_map_type = absl::flat_hash_map<const char*, Counter32, djb2_hash, eqstr>;

/**
 * A KeyStats object holds all statistics for an OSM tag key.
 */
class KeyStats {

    Counter32 m_key;
    Counter32 m_values;
    Counter32 m_cells;

    combination_hash_map_type m_key_combination_hash;

    user_hash_map_type m_user_hash;

    value_hash_map_type m_values_hash;

    GeoDistribution m_distribution;

public:
    const Counter32& key() const noexcept {
        return m_key;
    }

    const Counter32& values() const noexcept {
        return m_values;
    }

    const Counter32& cells() const noexcept {
        return m_cells;
    }

    void set_cells_count(osmium::item_type type, uint32_t count) noexcept {
        m_cells.set_count(type, count);
    }

    const combination_hash_map_type& key_combination_hash() const noexcept {
        return m_key_combination_hash;
    }

    const user_hash_map_type& user_hash() const noexcept {
        return m_user_hash;
    }

    const value_hash_map_type& values_hash() const noexcept {
        return m_values_hash;
    }

    GeoDistribution& distribution() noexcept {
        return m_distribution;
    }

    const GeoDistribution& distribution() const noexcept {
        return m_distribution;
    }

    void update(const char* value, const osmium::OSMObject& object, StringStore& string_store) {
        const auto type = object.type();

        m_key.incr(type);

        const auto values_iterator = m_values_hash.find(value);
        if (values_iterator == m_values_hash.end()) {
            Counter32 counter;
            counter.incr(type);
            m_values_hash.insert(std::pair<const char*, Counter32>(string_store.add(value), counter));
            m_values.incr(type);
        } else {
            values_iterator->second.incr(type);
            if (values_iterator->second.count(type) == 1) {
                m_values.incr(type);
            }
        }

        m_user_hash[object.uid()]++;
    }

    void add_key_combination(const char* other_key, osmium::item_type type) {
        m_key_combination_hash[other_key].incr(type);
    }

}; // class KeyStats

using key_hash_map_type = absl::flat_hash_map<const char*, KeyStats, djb2_hash, eqstr>;

/**
 * A KeyValueStats object holds some statistics for an OSM tag (key/value pair).
 */
class KeyValueStats {

    combination_hash_map_type m_key_value_combination_hash;

public:

    const combination_hash_map_type& key_value_combination_hash() const noexcept {
        return m_key_value_combination_hash;
    }

    void add_key_combination(const char* other_key, osmium::item_type type) {
        m_key_value_combination_hash[other_key].incr(type);
    }

}; // class KeyValueStats

using key_value_hash_map_type = absl::flat_hash_map<const char*, KeyValueStats, djb2_hash, eqstr>;
using key_value_geodistribution_hash_map_type = absl::flat_hash_map<std::pair<const char*, const char*>, GeoDistribution, djb2_hash, eqstr>;

class RelationTypeStats {

    uint64_t m_count = 0;
    Counter64 m_members;

    absl::flat_hash_map<std::string, Counter32> m_role_counts;

public:

    uint64_t count() const noexcept {
        return m_count;
    }

    Counter64 members() const noexcept {
        return m_members;
    }

    const absl::flat_hash_map<std::string, Counter32>& role_counts() const noexcept {
        return m_role_counts;
    }

    void add(const osmium::Relation& relation) {
        ++m_count;

        for (const auto& member : relation.members()) {
            m_role_counts[member.role()].incr(member.type());
            m_members.incr(member.type());
        }
    }

}; // class RelationTypeStats


/**
 * Osmium handler that creates statistics for Taginfo.
 */
class TagStatsHandler : public osmium::handler::Handler {

    osmium::util::VerboseOutput& m_vout;

    /**
     * Tag combination not appearing at least this often are not written
     * to database.
     */
    unsigned int m_min_tag_combination_count;

    time_t m_timer;

    key_hash_map_type m_tags_stat;

    key_value_hash_map_type m_key_value_stats;

    key_value_geodistribution_hash_map_type m_key_value_geodistribution;

    absl::flat_hash_map<std::string, RelationTypeStats> m_relation_type_stats;

    osmium::Timestamp m_max_timestamp{};

    // this must be much bigger than the largest string we want to store
    static const int string_store_size = 1024 * 1024 * 10;
    StringStore m_string_store;

    Sqlite::Database& m_database;

    StatisticsHandler m_statistics_handler;

    MapToInt& m_map_to_int;

    LocationIndex& m_location_index;

    osmium::item_type m_last_type = osmium::item_type::node;

    void timer_info(const char* msg);

    void update_key_combination_hash(osmium::item_type type,
                                      osmium::TagList::const_iterator it1,
                                      osmium::TagList::const_iterator end);

    void update_key_value_combination_hash2(osmium::item_type type,
                                             osmium::TagList::const_iterator it,
                                             osmium::TagList::const_iterator end,
                                             key_value_hash_map_type::iterator kvi1,
                                             const std::string& key_value1);

    void update_key_value_combination_hash(osmium::item_type type,
                                            osmium::TagList::const_iterator it,
                                            osmium::TagList::const_iterator end);

    void print_and_clear_key_distribution_images(osmium::item_type type);

    void print_and_clear_tag_distribution_images(osmium::item_type type);

    void print_actual_memory_usage();

    KeyStats& get_stat(const char* key);
    void collect_tag_stats(const osmium::OSMObject& object);

public:

    TagStatsHandler(Sqlite::Database& database,
                    const std::string& selection_database_name,
                    MapToInt& map_to_int,
                    unsigned int min_tag_combination_count,
                    osmium::util::VerboseOutput& vout,
                    LocationIndex& location_index);

    void node(const osmium::Node& node);

    void way(const osmium::Way& way);

    void relation(const osmium::Relation& relation);

    void before_ways();

    void before_relations();

    void write_to_database();

}; // class TagStatsHandler

