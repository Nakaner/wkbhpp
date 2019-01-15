#include "catch.hpp"

#include <wkbhpp/wkbwriter.hpp>

#include <string>

#if __BYTE_ORDER == __LITTLE_ENDIAN

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
    REQUIRE(wkb == "01010000009A99999999990940CDCCCCCCCCCC1040");
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in EWKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(3.2, 4.2)};
    REQUIRE(wkb == "0101000020E61000009A99999999990940CDCCCCCCCCCC1040");
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in web mercator WKB") {
    wkbhpp::WKBWriter factory{3857, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(356222, 467961)};
    REQUIRE(wkb.substr(0, 10) == "0101000000"); // little endian, point type
    REQUIRE(wkb.substr(10 + 2, 16 - 2) == "000000F8BD1541"); // x coordinate (without first (least significant) byte)
    REQUIRE(wkb.substr(26 + 2, 16 - 2) == "000000E48F1C41"); // y coordinate (without first (least significant) byte)
}

TEST_CASE("WKB geometry factory (byte-order-dependent), point in web mercator EWKB") {
    wkbhpp::WKBWriter factory{3857, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};

    const std::string wkb{factory.make_point(356222, 467961)};
    REQUIRE(wkb.substr(0, 10) == "0101000020"); // little endian, point type (extended)
    REQUIRE(wkb.substr(10, 8) == "110F0000"); // SRID 3857
    REQUIRE(wkb.substr(18 + 2, 16 - 2) == "000000F8BD1541"); // x coordinate (without first (least significant) byte)
    REQUIRE(wkb.substr(34 + 2, 16 - 2) == "000000E48F1C41"); // y coordinate (without first (least significant) byte)
}

TEST_CASE("WKB geometry factory (byte-order-dependent): linestring in WKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::wkb, wkbhpp::out_type::hex};
    const std::string wkb{add_linestring_points(factory)};
    REQUIRE(wkb == "0102000000030000009A99999999990940CDCCCCCCCCCC10400000000000000C40CDCCCCCCCC" \
            "CC1240CDCCCCCCCCCC0C409A99999999991340");
}

TEST_CASE("WKB geometry factory (byte-order-dependent): linestring in EPSG:4326 EWKB") {
    wkbhpp::WKBWriter factory{4326, wkbhpp::wkb_type::ewkb, wkbhpp::out_type::hex};
    const std::string wkb{add_linestring_points(factory)};
    REQUIRE(wkb == "0102000020E6100000030000009A99999999990940CDCCCCCCCCCC10400000000000000C40CD" \
            "CCCCCCCCCC1240CDCCCCCCCCCC0C409A99999999991340");
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
    REQUIRE(wkb == "010300000001000000040000009A99999999990940CDCCCCCCCCCC10400000000000000C40CD" \
            "CCCCCCCCCC1240CDCCCCCCCCCC0C409A999999999913409A99999999990940CDCCCCCCCCCC1040");
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
    REQUIRE(wkb == "010300000002000000040000009A99999999990940CDCCCCCCCCCC10400000000000000C40CD" \
            "CCCCCCCCCC1240CDCCCCCCCCCC0C409A999999999913409A99999999990940CDCCCCCCCCCC104005000" \
            "0006666666666660A4033333333333311406666666666660A409A999999999911403333333333330B40" \
            "9A999999999911403333333333330B4033333333333311406666666666660A403333333333331140");
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
    REQUIRE(wkb == "010600000002000000010300000001000000040000009A99999999990940CDCCCCCCCCCC1040" \
            "0000000000000C40CDCCCCCCCCCC124000000000000008409A999999999913409A99999999990940CDC" \
            "CCCCCCCCC1040010300000002000000040000006666666666662A40CDCCCCCCCCCC1040000000000000" \
            "2B40CDCCCCCCCCCC12400000000000002A409A999999999913406666666666662A40CDCCCCCCCCCC104" \
            "0040000000000000000802A4000000000000011409A99999999192A4066666666666613406666666666" \
            "E62A409A999999999912400000000000802A400000000000001140");
}

#endif
