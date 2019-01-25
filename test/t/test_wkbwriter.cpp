#include "catch.hpp"

#include <wkbhpp/wkbwriter.hpp>

#include <array>
#include <string>

constexpr int char_size = 1;
constexpr int uint32_size = 4;
constexpr int double_size = 8;
constexpr int chars_per_byte = 2;
constexpr int point_size = 2 * double_size;

#if __BYTE_ORDER == __LITTLE_ENDIAN

/**
 * Decode a double from a HEX string at a given offset.
 *
 * @param hex HEX string
 * @param offset offset/2 from beginning where the number should be located
 * (offset is multiplied by 2 in the implementation because HEX strings need two characters per byte)
 */
template <typename TNumber, size_t TWidth>
TNumber hex_to_number(const std::string& hex, const size_t offset) {
    // first convert our ASCII HEX representation to bytes
    std::array<unsigned char, TWidth> data;
    for (unsigned char i = 0; i < TWidth; ++i) {
        unsigned char digit = 0;
        for (unsigned char j = 0; j < chars_per_byte; ++j) {
            unsigned char c = hex.at(offset * chars_per_byte + i * chars_per_byte + j);
            if (c >= '0' && c <= '9') {
                c -= 48; // ASCII: 48 == '0'
            } else if (c >= 'A' && c <= 'F') {
                c = c - 65 + 10;
            } else if (c >= 'a' && c <= 'f') {
                c = c - 97 + 10;
            }
            if (j == 0) {
                digit += c * 16;
            } else {
                digit += c;
            }
        }
        data[i] = digit;
    }
    // now read it as double
    TNumber result = *(reinterpret_cast<TNumber*>(data.data()));
    return result;
}

const auto get_double_at = hex_to_number<double, double_size>;

const auto get_uint32_t_at = hex_to_number<uint32_t, uint32_size>;

const auto get_char_at = hex_to_number<char, char_size>;

template <typename TNumber, size_t TWidth>
void test_hex_to_number(const TNumber val, std::string garbage = "") {
    std::string mystr = garbage;
    wkbhpp::str_push<TNumber>(mystr, val);
    std::string hex = wkbhpp::convert_to_hex(mystr);
    const TNumber x = hex_to_number<TNumber, TWidth>(hex, garbage.length());
    REQUIRE(x == val);
}

TEST_CASE("test hex_to_double") {
    test_hex_to_number<double, double_size>(1);
    test_hex_to_number<double, double_size>(-1);
    test_hex_to_number<double, double_size>(15647567671474);
    test_hex_to_number<double, double_size>(15647.567671474);
    test_hex_to_number<double, double_size>(-1564756767.474);
    test_hex_to_number<double, double_size>(100);
    test_hex_to_number<double, double_size>(100, "/xk");
    test_hex_to_number<double, double_size>(3.2);
    test_hex_to_number<double, double_size>(-1564756767.474, "/rk");
    test_hex_to_number<double, double_size>(15647567671474, "/sk");
}

TEST_CASE("test hex_to_uint32_t") {
    test_hex_to_number<uint32_t, uint32_size>(0);
    test_hex_to_number<uint32_t, uint32_size>(0, "hghdf");
    test_hex_to_number<uint32_t, uint32_size>(1);
    test_hex_to_number<uint32_t, uint32_size>(100);
    test_hex_to_number<uint32_t, uint32_size>(100, "/xk");
    test_hex_to_number<uint32_t, uint32_size>(156475674, "/sk");
}

TEST_CASE("test hex_to_char") {
    test_hex_to_number<char, char_size>(0);
    test_hex_to_number<char, char_size>(0, "hghdf");
    test_hex_to_number<char, char_size>(1);
    test_hex_to_number<char, char_size>(100);
    test_hex_to_number<char, char_size>(100, "/xk");
    test_hex_to_number<char, char_size>(250, "/sk");
}

std::string add_linestring_points(wkbhpp::WKBWriter& writer) {
    writer.linestring_start();
    writer.linestring_add_location(3.2, 4.2);
    writer.linestring_add_location(3.5, 4.7);
    writer.linestring_add_location(3.6, 4.9);
    return writer.linestring_finish(3);
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(3.2, 4.2)};
    // check byte at offset 0 (endianess) and 4 bytes starting at offset 1 (uint32_t wkgGeometryType)
    REQUIRE(wkb.substr(0, 5 * chars_per_byte) == "0101000000");
    // check first coordinate (byte offset: 5, length: 8)
    REQUIRE(Approx(get_double_at(wkb, 5)) == 3.2);
    // check first coordinate (byte offset: 13, length: 8)
    REQUIRE(Approx(get_double_at(wkb, 13)) == 4.2);
    REQUIRE(wkb.length() == chars_per_byte * (char_size + uint32_size + point_size));
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in EWKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(3.2, 4.2)};
    // check byte at offset 0 (endianess), 4 bytes starting at offset 1 (uint32_t wkbGeometryType),
    // 4 bytes starting at offset 5 (uint32_t srid)
    REQUIRE(wkb.substr(0, 9 * chars_per_byte) == "0101000020E6100000");
    // check first coordinate (byte offset: 9, length: 8)
    REQUIRE(Approx(get_double_at(wkb, 9)) == 3.2);
    // check first coordinate (byte offset: 17, length: 8)
    REQUIRE(Approx(get_double_at(wkb, 17)) == 4.2);
    REQUIRE(wkb.length() == chars_per_byte * (char_size + 2 * uint32_size + point_size));
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in web mercator WKB") {
    wkbhpp::WKBWriter factory{3857, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(356222, 467961)};
    REQUIRE(wkb.substr(0, 5 * chars_per_byte) == "0101000000");
    REQUIRE(Approx(get_double_at(wkb, 5)) == 356222);
    REQUIRE(Approx(get_double_at(wkb, 13)) == 467961);
    REQUIRE(wkb.length() == chars_per_byte * (char_size + uint32_size + point_size));
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in web mercator EWKB") {
    wkbhpp::WKBWriter factory{3857, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(356222, 467961)};
    REQUIRE(wkb.substr(0, 9 * chars_per_byte) == "0101000020110F0000");
    REQUIRE(Approx(get_double_at(wkb, 9)) == 356222);
    REQUIRE(Approx(get_double_at(wkb, 17)) == 467961);
    REQUIRE(wkb.length() == chars_per_byte * (char_size + 2 * uint32_size + point_size));
}

TEST_CASE("WKB geometry factory (byte-order-dependent): linestring in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};
    const std::string wkb{add_linestring_points(factory)};
    REQUIRE(wkb.substr(0, 5 * chars_per_byte) == "0102000000");
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 5) == 3);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 9)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 17)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 25)) == 3.5);
    REQUIRE(Approx(get_double_at(wkb, 33)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 41)) == 3.6);
    REQUIRE(Approx(get_double_at(wkb, 49)) == 4.9);
    REQUIRE(wkb.length() == 57 * chars_per_byte);
}

TEST_CASE("WKB geometry factory (byte-order-dependent): linestring in EPSG:4326 EWKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};
    const std::string wkb{add_linestring_points(factory)};
    // WKB should look more or less like:
    // 0102000020E6100000030000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCCCC1240CDCCCCCCCCCC0C409A99999999991340
    REQUIRE(wkb.length() == chars_per_byte * (char_size + 3 * uint32_size + 3 * point_size));
    REQUIRE(wkb.substr(0, 9 * chars_per_byte) == "0102000020E6100000");
    // endianess
    REQUIRE(get_char_at(wkb, 0) == 1);
    // geometry_type | srid_presence_flag
    uint32_t expected = 2 | 0x20000000;
    REQUIRE(get_uint32_t_at(wkb, 1) == expected);
    // SRID
    REQUIRE(get_uint32_t_at(wkb, 5) == 4326);
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 9) == 3);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 13)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 21)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 29)) == 3.5);
    REQUIRE(Approx(get_double_at(wkb, 37)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 45)) == 3.6);
    REQUIRE(Approx(get_double_at(wkb, 53)) == 4.9);
    REQUIRE(wkb.length() == 61 * chars_per_byte);
}

TEST_CASE("WKB geometry factory (byte-order-dependent): polygon in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};
    factory.polygon_start();
    factory.polygon_outer_ring_start();
    factory.polygon_add_location(3.2, 4.2);
    factory.polygon_add_location(3.5, 4.7);
    factory.polygon_add_location(3.6, 4.9);
    factory.polygon_add_location(3.2, 4.2);
    factory.polygon_outer_ring_finish();
    const std::string wkb{factory.polygon_finish()};
    // WKB should look more or less like:
    // 010300000001000000040000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCCCC1240CDCCCCCCCCCC0C409A999999999913409A99999999990940CDCCCCCCCCCC1040

    // endianess
    REQUIRE(get_char_at(wkb, 0) == 1);
    // geometry type (Polygon)
    REQUIRE(get_uint32_t_at(wkb, 1) == 3);
    // number of rings
    REQUIRE(get_uint32_t_at(wkb, 5) == 1);

    // LinearRing 1
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 9) == 4);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 13)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 21)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 29)) == 3.5);
    REQUIRE(Approx(get_double_at(wkb, 37)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 45)) == 3.6);
    REQUIRE(Approx(get_double_at(wkb, 53)) == 4.9);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 61)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 69)) == 4.2);

    REQUIRE(wkb.length() == 77 * chars_per_byte);
}

TEST_CASE("WKB geometry factory (byte-order-dependent): polygon with 1 inner ring in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};
    factory.polygon_start();
    factory.polygon_outer_ring_start();
    factory.polygon_add_location(3.2, 4.2);
    factory.polygon_add_location(3.5, 4.7);
    factory.polygon_add_location(3.6, 4.9);
    factory.polygon_add_location(3.2, 4.2);
    factory.polygon_outer_ring_finish();
    factory.polygon_inner_ring_start();
    factory.polygon_add_location(3.3, 4.3);
    factory.polygon_add_location(3.3, 4.4);
    factory.polygon_add_location(3.4, 4.4);
    factory.polygon_add_location(3.4, 4.3);
    factory.polygon_add_location(3.3, 4.3);
    factory.polygon_inner_ring_finish();
    const std::string wkb{factory.polygon_finish()};

    // header of Polygon
    // endianess
    REQUIRE(get_char_at(wkb, 0) == 1);
    // geometry type (Polygon)
    REQUIRE(get_uint32_t_at(wkb, 1) == 3);
    // number of rings
    REQUIRE(get_uint32_t_at(wkb, 5) == 2);

    // LinearRing 1
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 9) == 4);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 13)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 21)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 29)) == 3.5);
    REQUIRE(Approx(get_double_at(wkb, 37)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 45)) == 3.6);
    REQUIRE(Approx(get_double_at(wkb, 53)) == 4.9);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 61)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 69)) == 4.2);

    // LinearRing 2
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 77) == 5);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 81)) == 3.3);
    REQUIRE(Approx(get_double_at(wkb, 89)) == 4.3);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 97)) == 3.3);
    REQUIRE(Approx(get_double_at(wkb, 105)) == 4.4);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 113)) == 3.4);
    REQUIRE(Approx(get_double_at(wkb, 121)) == 4.4);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 129)) == 3.4);
    REQUIRE(Approx(get_double_at(wkb, 137)) == 4.3);
    // point 4
    REQUIRE(Approx(get_double_at(wkb, 145)) == 3.3);
    REQUIRE(Approx(get_double_at(wkb, 153)) == 4.3);

    // total length:
    // header (endianess, geometry type) + number of rings = 9
    // first ring: number of points + 4 points = 68
    // first ring: number of points + 5 points = 84
    // sum: 231
    REQUIRE(wkb.length() == 161 * chars_per_byte);
}

TEST_CASE("WKB geometry factory (byte-order-dependent): multipolygon in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};
    factory.multipolygon_start();
    factory.multipolygon_polygon_start();
    factory.multipolygon_outer_ring_start();
    factory.multipolygon_add_location(3.2, 4.2);
    factory.multipolygon_add_location(3.5, 4.7);
    factory.multipolygon_add_location(3.0, 4.9);
    factory.multipolygon_add_location(3.2, 4.2);
    factory.multipolygon_outer_ring_finish();
    factory.multipolygon_polygon_finish();
    factory.multipolygon_polygon_start();
    factory.multipolygon_outer_ring_start();
    factory.multipolygon_add_location(13.2, 4.2);
    factory.multipolygon_add_location(13.5, 4.7);
    factory.multipolygon_add_location(13.0, 4.9);
    factory.multipolygon_add_location(13.2, 4.2);
    factory.multipolygon_outer_ring_finish();
    factory.multipolygon_inner_ring_start();
    factory.multipolygon_add_location(13.25, 4.25);
    factory.multipolygon_add_location(13.05, 4.85);
    factory.multipolygon_add_location(13.45, 4.65);
    factory.multipolygon_add_location(13.25, 4.25);
    factory.multipolygon_inner_ring_finish();
    factory.multipolygon_polygon_finish();
    const std::string wkb{factory.multipolygon_finish()};

    // header of MultiPolygon
    // endianess
    REQUIRE(get_char_at(wkb, 0) == 1);
    // geometry type (MultiPolygon)
    REQUIRE(get_uint32_t_at(wkb, 1) == 6);
    // number of polygons
    REQUIRE(get_uint32_t_at(wkb, 5) == 2);

    // LinearRing 1
    // endianess
    REQUIRE(get_char_at(wkb, 9) == 1);
    // geometry type (Polygon)
    REQUIRE(get_uint32_t_at(wkb, 10) == 3);
    // number of rings
    REQUIRE(get_uint32_t_at(wkb, 14) == 1);
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 18) == 4);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 22)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 30)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 38)) == 3.5);
    REQUIRE(Approx(get_double_at(wkb, 46)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 54)) == 3.0);
    REQUIRE(Approx(get_double_at(wkb, 62)) == 4.9);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 70)) == 3.2);
    REQUIRE(Approx(get_double_at(wkb, 78)) == 4.2);

    // LinearRing 2
    // endianess
    REQUIRE(get_char_at(wkb, 86) == 1);
    // geometry type (Polygon)
    REQUIRE(get_uint32_t_at(wkb, 87) == 3);
    // number of rings
    REQUIRE(get_uint32_t_at(wkb, 91) == 2);
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 95) == 4);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 99)) == 13.2);
    REQUIRE(Approx(get_double_at(wkb, 107)) == 4.2);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 115)) == 13.5);
    REQUIRE(Approx(get_double_at(wkb, 123)) == 4.7);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 131)) == 13.0);
    REQUIRE(Approx(get_double_at(wkb, 139)) == 4.9);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 147)) == 13.2);
    REQUIRE(Approx(get_double_at(wkb, 155)) == 4.2);
    // number of points
    REQUIRE(get_uint32_t_at(wkb, 163) == 4);
    // point 0
    REQUIRE(Approx(get_double_at(wkb, 167)) == 13.25);
    REQUIRE(Approx(get_double_at(wkb, 175)) == 4.25);
    // point 1
    REQUIRE(Approx(get_double_at(wkb, 183)) == 13.05);
    REQUIRE(Approx(get_double_at(wkb, 191)) == 4.85);
    // point 2
    REQUIRE(Approx(get_double_at(wkb, 199)) == 13.45);
    REQUIRE(Approx(get_double_at(wkb, 207)) == 4.65);
    // point 3
    REQUIRE(Approx(get_double_at(wkb, 215)) == 13.25);
    REQUIRE(Approx(get_double_at(wkb, 223)) == 4.25);

    // total length:
    // header (endianess, geometry type) + number of polygons = 9
    // first polygon: header + number of rings + number of points + 4 points = 77
    // second polygon: header + number of rings = 9
    // second polygon, first ring: number of points + 4 points = 68
    // second polygon, second ring: number of points + 4 points = 68
    // sum: 231
    REQUIRE(wkb.length() == 231 * chars_per_byte);
}

#endif
