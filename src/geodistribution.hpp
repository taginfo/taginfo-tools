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

#include <osmium/osm/location.hpp>
#include <osmium/osm/types.hpp>

#include <gd.h>

#include <cassert>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

/**
 * Functor class defining the call operator as a function that limits a
 * osmium::Location to a bounding box, reduces the resolution
 * of the coordinates and returns an integer.
 *
 * If the position is outside the bounding box, the max value for this type
 * is returned.
 */
class MapToInt {

    double m_minx;
    double m_miny;
    double m_maxx;
    double m_maxy;

    unsigned int m_width;
    unsigned int m_height;

    double m_dx;
    double m_dy;

public:

    MapToInt(double minx, double miny, double maxx, double maxy, unsigned int width, unsigned int height) :
        m_minx(minx), m_miny(miny), m_maxx(maxx), m_maxy(maxy),
        m_width(width), m_height(height),
        m_dx(maxx - minx), m_dy(maxy - miny) {
        assert(size() < std::numeric_limits<uint32_t>::max());
    }

    uint32_t operator()(const osmium::Location& p) const noexcept {
        if (!p.valid() || p.lon_without_check() < m_minx
                       || p.lat_without_check() < m_miny
                       || p.lon_without_check() >= m_maxx
                       || p.lat_without_check() >= m_maxy) {
            // if the position is out of bounds we return MAXINT
            return std::numeric_limits<uint32_t>::max();
        }

        auto x = static_cast<int>((p.lon_without_check() - m_minx) / m_dx * m_width);
        auto y = static_cast<int>((m_maxy - p.lat_without_check()) / m_dy * m_height);

        if (x < 0) {
            x = 0;
        } else if (static_cast<unsigned int>(x) >= m_width) {
            x = static_cast<int>(m_width) - 1;
        }
        if (y < 0) {
            y = 0;
        } else if (static_cast<unsigned int>(y) >= m_height) {
            y = static_cast<int>(m_height) - 1;
        }

        return static_cast<unsigned int>(y) * m_width + static_cast<unsigned int>(x);
    }

    unsigned int width() const noexcept {
        return m_width;
    }

    unsigned int height() const noexcept {
        return m_height;
    }

    unsigned int size() const noexcept {
        return m_width * m_height;
    }

}; // class MapToInt

/**
 * Stores the geographical distribution of something in a space efficient way.
 */
class GeoDistribution {

    using geo_distribution_type = std::vector<bool>;

    /**
     * Contains a pointer to a bitset that gives us the distribution.
     * If only one grid cell is used so far, this pointer is nullptr. Only
     * if more than one grid cell is used, we dynamically create an
     * object for this.
     */
    std::unique_ptr<geo_distribution_type> m_distribution = nullptr;

    /**
     * Number of set grid cells.
     */
    unsigned int m_cells = 0;

    /// If there is only one grid cell location, this is where its kept.
    uint32_t m_location = 0;

    /// Overall distribution
    static geo_distribution_type c_distribution_all;

    static unsigned int c_width;
    static unsigned int c_height;

public:

    GeoDistribution() = default;

    void clear() {
        m_distribution.reset(nullptr);
        m_cells = 0;
        m_location = 0;
    }

    static void set_dimensions(unsigned int width, unsigned  int height) {
        c_width = width;
        c_height = height;
        c_distribution_all.resize(c_width * c_height);
    }

    /**
     * Add the given coordinate to the distribution store.
     */
    void add_coordinate(uint32_t n) {
        if (n == std::numeric_limits<uint32_t>::max()) {
            // ignore positions that are out of bounds
            return;
        }
        if (m_cells == 0) {
            m_location = n;
            ++m_cells;
            c_distribution_all[n] = true;
        } else if (m_cells == 1 && m_location != n) {
            m_distribution = std::make_unique<geo_distribution_type>(c_width * c_height);
            m_distribution->operator[](m_location) = true;
            c_distribution_all[m_location] = true;
            m_distribution->operator[](n) = true;
            c_distribution_all[n] = true;
            ++m_cells;
        } else if (m_cells == 1 && m_location == n) {
            // nothing to do
        } else if (! m_distribution->operator[](n)) {
            ++m_cells;
            m_distribution->operator[](n) = true;
            c_distribution_all[n] = true;
        }
    }

    class Image {

        gdImagePtr m_image;
        int m_color;

    public:

        Image(unsigned int width, unsigned int height) :
            m_image(gdImageCreate(static_cast<int>(width), static_cast<int>(height))) {
            gdImageColorTransparent(m_image, gdImageColorAllocate(m_image, 0, 0, 0));
            m_color = gdImageColorAllocate(m_image, 180, 0, 0);
        }

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;

        ~Image() {
            gdImageDestroy(m_image);
        }

        void set_pixel(unsigned int x, unsigned int y) noexcept {
            gdImageSetPixel(m_image, static_cast<int>(x), static_cast<int>(y), m_color);
        }

        gdImagePtr data() const noexcept {
            return m_image;
        }

    }; // class Image

    class Png {

        int m_size = 0;
        void* m_data;

    public:

        explicit Png(Image& image) :
            m_data(gdImagePngPtr(image.data(), &m_size)) {
        }

        Png(const Png&) = delete;
        Png& operator=(const Png&) = delete;

        Png(Png&&) = default;
        Png& operator=(Png&&) = default;

        ~Png() {
            gdFree(m_data);
        }

        int size() const noexcept {
            return m_size;
        }

        void* data() const noexcept {
            return m_data;
        }

    }; // class Png

    Png create_png() const {
        Image image{c_width, c_height};

        if (m_cells == 1) {
            const auto y = m_location / c_width;
            const auto x = m_location - (y * c_width);
            image.set_pixel(x, y);
        } else if (m_cells >= 2) {
            std::size_t n = 0;
            for (unsigned int y = 0; y < c_height; ++y) {
                for (unsigned int x = 0; x < c_width; ++x) {
                    if (m_distribution->operator[](n)) {
                        image.set_pixel(x, y);
                    }
                    ++n;
                }
            }
        }

        return Png{image};
    }

    static Png create_empty_png() {
        Image image{c_width, c_height};
        return Png{image};
    }

    /**
     * Return the number of cells set.
     */
    unsigned int cells() const noexcept {
        return m_cells;
    }

    /**
     * Return the number of cells that are set in at least one GeoDistribution
     * object.
     */
    static unsigned int count_all_set_cells() {
        unsigned int c = 0;
        for (unsigned int n = 0; n < c_width * c_height; ++n) {
            if (c_distribution_all[n]) {
                ++c;
            }
        }
        return c;
    }

}; // class GeoDistribution

