#pragma once

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

#include <sqlite.hpp>

#include <osmium/handler.hpp>
#include <osmium/osm.hpp>

#include <array>
#include <cstdint>

/**
 * Osmium handler that collects basic statistics from OSM data and
 * writes it to a Sqlite database.
 */
class StatisticsHandler : public osmium::handler::Handler {

public:

    explicit StatisticsHandler(Sqlite::Database& database) :
        Handler(),
        m_database(database) {
    }

    void node(const osmium::Node& node) noexcept {
        update_common_stats(node);
        m_stats.nodes++;
        if (m_tag_count == 0) {
            m_stats.nodes_without_tags++;
        }
        if (m_id > m_stats.max_node_id) {
            m_stats.max_node_id = m_id;
        }
        m_stats.node_tags += m_tag_count;
        if (m_tag_count > m_stats.max_tags_on_node) {
            m_stats.max_tags_on_node = m_tag_count;
        }
        if (m_version > static_cast<int64_t>(m_stats.max_node_version)) {
            m_stats.max_node_version = m_version;
        }
        m_stats.sum_node_version += m_version;
    }

    void way(const osmium::Way& way) noexcept {
        update_common_stats(way);
        m_stats.ways++;
        if (m_tag_count == 0) {
            m_stats.ways_without_tags++;
        }
        if (way.is_closed()) {
            m_stats.closed_ways++;
        }
        if (m_id > m_stats.max_way_id) {
            m_stats.max_way_id = m_id;
        }
        m_stats.way_tags += m_tag_count;

        const auto way_nodes_size = way.nodes().size();
        m_stats.way_nodes += way_nodes_size;
        if (m_tag_count > m_stats.max_tags_on_way) {
            m_stats.max_tags_on_way = m_tag_count;
        }
        if (way_nodes_size > m_stats.max_nodes_on_way) {
            m_stats.max_nodes_on_way = way_nodes_size;
        }
        if (m_version > static_cast<int64_t>(m_stats.max_way_version)) {
            m_stats.max_way_version = m_version;
        }
        m_stats.sum_way_version += m_version;

        osmium::object_id_type ref = 0;
        for (const auto& wn : way.nodes()) {
            const osmium::object_id_type diff = wn.ref() - ref;
            if (diff == 1) {
                ++m_stats.way_nodes_consecutive;
            } else if (diff <= 127) { // 2^7-1
                ++m_stats.way_nodes_within_127;
            } else if (diff <= 32767) { // 2^15-1
                ++m_stats.way_nodes_within_32767;
            }
            ref = wn.ref();
        }
    }

    void relation(const osmium::Relation& relation) noexcept {
        update_common_stats(relation);
        m_stats.relations++;
        if (m_tag_count == 0) {
            m_stats.relations_without_tags++;
        }
        if (m_id > m_stats.max_relation_id) {
            m_stats.max_relation_id = m_id;
        }
        m_stats.relation_tags += m_tag_count;
        const auto member_count = relation.members().size();
        m_stats.relation_members += member_count;
        if (m_tag_count > m_stats.max_tags_on_relation) {
            m_stats.max_tags_on_relation = m_tag_count;
        }
        if (member_count > m_stats.max_members_on_relation) {
            m_stats.max_members_on_relation = member_count;
        }
        if (m_version > static_cast<int64_t>(m_stats.max_relation_version)) {
            m_stats.max_relation_version = m_version;
        }
        m_stats.sum_relation_version += m_version;

        for (const auto& member : relation.members()) {
            switch (member.type()) {
                case osmium::item_type::node:
                    ++m_stats.relation_member_nodes;
                    break;
                case osmium::item_type::way:
                    ++m_stats.relation_member_ways;
                    break;
                case osmium::item_type::relation:
                    ++m_stats.relation_member_relations;
                    break;
                default:
                    break;
            }
        }
        const char* type = relation.tags().get_value_by_key("type");
        if (!type) {
            m_stats.relations_without_type++;
        }
    }

    static constexpr const std::size_t num_stats = 36;

    void write_to_database() {
        // if you change anything in this array, also change the corresponding struct below
        static constexpr const std::array<const char*, num_stats> stat_names = {
            "nodes",
            "nodes_without_tags",
            "node_tags",
            "max_node_id",
            "max_tags_on_node",
            "ways",
            "ways_without_tags",
            "way_tags",
            "way_nodes",
            "way_nodes_consecutive",
            "way_nodes_within_127",
            "way_nodes_within_32767",
            "max_way_id",
            "max_tags_on_way",
            "max_nodes_on_way",
            "closed_ways",
            "relations",
            "relations_without_tags",
            "relations_without_type",
            "relation_tags",
            "relation_members",
            "relation_member_nodes",
            "relation_member_ways",
            "relation_member_relations",
            "max_relation_id",
            "max_tags_on_relation",
            "max_members_on_relation",
            "max_user_id",
            "anon_user_objects",
            "max_node_version",
            "max_way_version",
            "max_relation_version",
            "sum_node_version",
            "sum_way_version",
            "sum_relation_version",
            "max_changeset_id"
        };

        Sqlite::Statement statement_insert_into_main_stats{m_database, "INSERT INTO stats (key, value) VALUES (?, ?);"};
        m_database.begin_transaction();

        for (std::size_t i = 0; i < stat_names.size(); ++i) {
            statement_insert_into_main_stats
                .bind_text(stat_names[i])
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                .bind_int64(static_cast<int64_t>(reinterpret_cast<uint64_t*>(&m_stats)[i]))
                .execute();
        }
        statement_insert_into_main_stats
            .bind_text("nodes_with_tags")
            .bind_int64(static_cast<int64_t>(m_stats.nodes - m_stats.nodes_without_tags))
            .execute();

        m_database.commit();
    }

private:

    // if you change anything in this struct, also change the corresponding array above
    struct statistics {
        uint64_t nodes = 0;
        uint64_t nodes_without_tags = 0;
        uint64_t node_tags = 0;
        uint64_t max_node_id = 0;
        uint64_t max_tags_on_node = 0;
        uint64_t ways = 0;
        uint64_t ways_without_tags = 0;
        uint64_t way_tags = 0;
        uint64_t way_nodes = 0;
        uint64_t way_nodes_consecutive = 0;
        uint64_t way_nodes_within_127 = 0;
        uint64_t way_nodes_within_32767 = 0;
        uint64_t max_way_id = 0;
        uint64_t max_tags_on_way = 0;
        uint64_t max_nodes_on_way = 0;
        uint64_t closed_ways = 0;
        uint64_t relations = 0;
        uint64_t relations_without_tags = 0;
        uint64_t relations_without_type = 0;
        uint64_t relation_tags = 0;
        uint64_t relation_members = 0;
        uint64_t relation_member_nodes = 0;
        uint64_t relation_member_ways = 0;
        uint64_t relation_member_relations = 0;
        uint64_t max_relation_id = 0;
        uint64_t max_tags_on_relation = 0;
        uint64_t max_members_on_relation = 0;
        uint64_t max_user_id = 0;
        uint64_t anon_user_objects = 0;
        uint64_t max_node_version = 0;
        uint64_t max_way_version = 0;
        uint64_t max_relation_version = 0;
        uint64_t sum_node_version = 0;
        uint64_t sum_way_version = 0;
        uint64_t sum_relation_version = 0;
        uint64_t max_changeset_id = 0;
    } m_stats;

    static_assert(sizeof(statistics) == sizeof(uint64_t) * num_stats,
                  "Size mismatch between struct statistics and stat_names array");

    Sqlite::Database& m_database;

    osmium::unsigned_object_id_type m_id = 0;
    std::size_t m_tag_count = 0;
    osmium::object_version_type m_version = 0;

    void update_common_stats(const osmium::OSMObject& object) noexcept {
        m_id        = object.positive_id();
        m_version   = object.version();
        m_tag_count = object.tags().size();

        const auto uid = object.uid();
        if (uid == 0) {
            m_stats.anon_user_objects++;
        }
        if (uid > static_cast<int64_t>(m_stats.max_user_id)) {
            m_stats.max_user_id = uid;
        }

        const auto changeset = object.changeset();
        if (changeset > static_cast<int64_t>(m_stats.max_changeset_id)) {
            m_stats.max_changeset_id = changeset;
        }
    }

}; // class StatisticsHandler

