# Taginfo Tools

These are some tools needed for creating statistics from a planet or other OSM
file. These are used as part of [taginfo](https://github.com/taginfo/taginfo).

They used to "live" in the
[main taginfo repository](https://github.com/taginfo/taginfo).

[![Travis Build Status](https://travis-ci.org/taginfo/taginfo-tools.svg?branch=master)](https://travis-ci.org/github/taginfo/taginfo-tools)

## Prerequisites

* [libgd](https://www.libgd.org/)
* [libicu](https://icu-project.org/)
* [libosmium](https://osmcode.org/libosmium) (>= 2.14.2)
* [libsqlite3](https://www.sqlite.org/)
* [protozero](https://github.com/mapbox/protozero)
* [bz2lib](https://www.bzip.org/)
* [zlib](https://www.zlib.net/)
* [Expat](https://libexpat.github.io/)
* [Sparse hash map](https://github.com/sparsehash/sparsehash)

Ob Debian/Ubuntu install these like this:

```
apt install \
    libbz2-dev \
    libexpat1-dev \
    libgd-dev \
    libicu-dev \
    libosmium2-dev \
    libprotozero-dev \
    libsparsehash-dev \
    libsqlite3-dev \
    zlib1g-dev
```

Use [Debian Backports](https://backports.debian.org/) or install `libosmium`
and `protozero` manually if their versions are too old in your distribution.

## Building

Build as usual with CMake. For instance like this:

```
mkdir build && cd build
cmake ..
make
```

## Programs

* `osmstats` - Create statistics from a planet or other OSM file (not used in taginfo)
* `taginfo-similarity` - Find similarities between OSM tags
* `taginfo-sizes` - Debugging tool to print out sizes of important structs from `taginfo-stats`
* `taginfo-stats` - Create statistics from a planet or other OSM file
* `taginfo-unicode` - Categorizes OSM tags based on unicode character types

## Contact

There is a mailing list for developers and people running their own instances
of taginfo:
[taginfo-dev](https://lists.openstreetmap.org/listinfo/taginfo-dev)

## Author

Jochen Topf (jochen@topf.org) - https://jochentopf.com/

