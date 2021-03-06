#include <mbgl/test/util.hpp>
#include <mbgl/test/fixture_log_observer.hpp>

#include <mbgl/sprite/sprite_parser.hpp>
#include <mbgl/sprite/sprite_image.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/string.hpp>

#include <algorithm>

using namespace mbgl;

namespace {

auto readImage(const std::string& name) {
    return decodeImage(util::read_file(name));
}

} // namespace

TEST(Sprite, SpriteImageCreationInvalid) {
    FixtureLog log;

    const PremultipliedImage image_1x = decodeImage(util::read_file("test/fixtures/annotations/emerald.png"));

    ASSERT_EQ(200u, image_1x.width);
    ASSERT_EQ(299u, image_1x.height);

    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 0, 16, 1, false));    // width == 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 16, 0, 1, false));    // height == 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, -1, 16, 1, false));   // width < 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 16, -1, 1, false));   // height < 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 1, 1, 0, false));     // ratio == 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 1, 1, -1, false));    // ratio < 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 1, 1, 23, false));    // ratio too large
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 2048, 16, 1, false)); // too wide
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 16, 1025, 1, false)); // too tall
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, -1, 0, 16, 16, 1, false));  // srcX < 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, -1, 16, 16, 1, false));  // srcY < 0
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, image_1x.width + 1, 16, 1, false));   // right edge out of bounds
    ASSERT_EQ(nullptr, createSpriteImage(image_1x, 0, 0, 16, image_1x.height + 1, 1, false));  // bottom edge out of bounds

    EXPECT_EQ(13u, log.count({
                      EventSeverity::Error,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't create sprite with invalid metrics",
                  }));
}

TEST(Sprite, SpriteImageCreation1x) {
    const PremultipliedImage image_1x = decodeImage(util::read_file("test/fixtures/annotations/emerald.png"));

    ASSERT_EQ(200u, image_1x.width);
    ASSERT_EQ(299u, image_1x.height);

    { // "museum_icon":{"x":177,"y":187,"width":18,"height":18,"pixelRatio":1,"sdf":false}
        const auto sprite = createSpriteImage(image_1x, 177, 187, 18, 18, 1, false);
        ASSERT_TRUE(sprite.get());
        EXPECT_EQ(18, sprite->getWidth());
        EXPECT_EQ(18, sprite->getHeight());
        EXPECT_EQ(18, sprite->image.width);
        EXPECT_EQ(18, sprite->image.height);
        EXPECT_EQ(1, sprite->pixelRatio);
        EXPECT_EQ(readImage("test/fixtures/annotations/result-spriteimagecreation1x-museum.png"),
                  sprite->image);
    }
}

TEST(Sprite, SpriteImageCreation2x) {
    const PremultipliedImage image_2x = decodeImage(util::read_file("test/fixtures/annotations/emerald@2x.png"));

    // "museum_icon":{"x":354,"y":374,"width":36,"height":36,"pixelRatio":2,"sdf":false}
    const auto sprite = createSpriteImage(image_2x, 354, 374, 36, 36, 2, false);
    ASSERT_TRUE(sprite.get());
    EXPECT_EQ(18, sprite->getWidth());
    EXPECT_EQ(18, sprite->getHeight());
    EXPECT_EQ(36, sprite->image.width);
    EXPECT_EQ(36, sprite->image.height);
    EXPECT_EQ(2, sprite->pixelRatio);
    EXPECT_EQ(readImage("test/fixtures/annotations/result-spriteimagecreation2x.png"),
              sprite->image);
}

TEST(Sprite, SpriteImageCreation1_5x) {
    const PremultipliedImage image_2x = decodeImage(util::read_file("test/fixtures/annotations/emerald@2x.png"));

    // "museum_icon":{"x":354,"y":374,"width":36,"height":36,"pixelRatio":2,"sdf":false}
    const auto sprite = createSpriteImage(image_2x, 354, 374, 36, 36, 1.5, false);
    ASSERT_TRUE(sprite.get());
    EXPECT_EQ(24, sprite->getWidth());
    EXPECT_EQ(24, sprite->getHeight());
    EXPECT_EQ(36, sprite->image.width);
    EXPECT_EQ(36, sprite->image.height);
    EXPECT_EQ(1.5, sprite->pixelRatio);
    EXPECT_EQ(readImage("test/fixtures/annotations/result-spriteimagecreation1_5x-museum.png"),
              sprite->image);

    // "hospital_icon":{"x":314,"y":518,"width":36,"height":36,"pixelRatio":2,"sdf":false}
    const auto sprite2 = createSpriteImage(image_2x, 314, 518, 35, 35, 1.5, false);
    ASSERT_TRUE(sprite2.get());
    EXPECT_EQ(float(35 / 1.5), sprite2->getWidth());
    EXPECT_EQ(float(35 / 1.5), sprite2->getHeight());
    EXPECT_EQ(35, sprite2->image.width);
    EXPECT_EQ(35, sprite2->image.height);
    EXPECT_EQ(1.5, sprite2->pixelRatio);
    EXPECT_EQ(readImage("test/fixtures/annotations/result-spriteimagecreation1_5x-hospital.png"),
              sprite2->image);
}

TEST(Sprite, SpriteParsing) {
    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = util::read_file("test/fixtures/annotations/emerald.json");

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();

    std::set<std::string> names;
    std::transform(images.begin(), images.end(), std::inserter(names, names.begin()),
                   [](const auto& pair) { return pair.first; });

    EXPECT_EQ(std::set<std::string>({ "airfield_icon",
                                      "airport_icon",
                                      "background",
                                      "cemetery_icon",
                                      "college_icon",
                                      "default_1",
                                      "default_2",
                                      "default_3",
                                      "default_4",
                                      "default_5",
                                      "default_6",
                                      "default_marker",
                                      "dlr",
                                      "dlr.london-overground.london-underground.national-rail",
                                      "dlr.london-underground",
                                      "dlr.london-underground.national-rail",
                                      "dlr.national-rail",
                                      "dot",
                                      "embassy_icon",
                                      "fire-station_icon",
                                      "generic-metro",
                                      "generic-rail",
                                      "generic_icon",
                                      "golf_icon",
                                      "government_icon",
                                      "grass_pattern",
                                      "harbor_icon",
                                      "hospital_icon",
                                      "hospital_striped",
                                      "interstate_1",
                                      "interstate_2",
                                      "interstate_3",
                                      "library_icon",
                                      "london-overground",
                                      "london-overground.london-underground",
                                      "london-overground.london-underground.national-rail",
                                      "london-overground.national-rail",
                                      "london-underground",
                                      "london-underground.national-rail",
                                      "marker_icon",
                                      "metro",
                                      "metro.rer",
                                      "monument_icon",
                                      "moscow-metro",
                                      "museum_icon",
                                      "national-rail",
                                      "oneway_motorway",
                                      "oneway_road",
                                      "park_icon",
                                      "police_icon",
                                      "post_icon",
                                      "prison_icon",
                                      "religious-christian_icon",
                                      "religious-jewish_icon",
                                      "religious-muslim_icon",
                                      "rer",
                                      "rer.transilien",
                                      "s-bahn",
                                      "s-bahn.u-bahn",
                                      "sand_noise",
                                      "school_icon",
                                      "school_striped",
                                      "secondary_marker",
                                      "u-bahn",
                                      "us_highway_1",
                                      "us_highway_2",
                                      "us_highway_3",
                                      "us_state_1",
                                      "us_state_2",
                                      "us_state_3",
                                      "washington-metro",
                                      "wiener-linien",
                                      "zoo_icon" }),
              names);

    {
        auto sprite = images.find("generic-metro")->second;
        EXPECT_EQ(18, sprite->getWidth());
        EXPECT_EQ(18, sprite->getHeight());
        EXPECT_EQ(18, sprite->image.width);
        EXPECT_EQ(18, sprite->image.height);
        EXPECT_EQ(1, sprite->pixelRatio);
        EXPECT_EQ(readImage("test/fixtures/annotations/result-spriteparsing.png"), sprite->image);
    }
}

TEST(Sprite, SpriteParsingInvalidJSON) {
    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": " })JSON";

    const auto error = parseSprite(image_1x, json_1x).get<std::exception_ptr>();

    EXPECT_EQ(util::toString(error),
              std::string("Failed to parse JSON: Missing a closing quotation mark in string. at offset 13"));
}

TEST(Sprite, SpriteParsingEmptyImage) {
    FixtureLog log;

    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": {} })JSON";

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();
    EXPECT_EQ(0u, images.size());

    EXPECT_EQ(1u, log.count({
                      EventSeverity::Error,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't create sprite with invalid metrics",
                  }));
}

TEST(Sprite, SpriteParsingSimpleWidthHeight) {
    FixtureLog log;

    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": { "width": 32, "height": 32 } })JSON";

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();
    EXPECT_EQ(1u, images.size());
}

TEST(Sprite, SpriteParsingWidthTooBig) {
    FixtureLog log;

    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": { "width": 65536, "height": 32 } })JSON";

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();
    EXPECT_EQ(0u, images.size());

    EXPECT_EQ(1u, log.count({
                      EventSeverity::Warning,
                      Event::Sprite,
                      int64_t(-1),
                      "Value of 'width' must be an integer between 0 and 65535",
                  }));
    EXPECT_EQ(1u, log.count({
                      EventSeverity::Error,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't create sprite with invalid metrics",
                  }));
}

TEST(Sprite, SpriteParsingNegativeWidth) {
    FixtureLog log;

    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": { "width": -1, "height": 32 } })JSON";

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();
    EXPECT_EQ(0u, images.size());

    EXPECT_EQ(1u, log.count({
                      EventSeverity::Warning,
                      Event::Sprite,
                      int64_t(-1),
                      "Value of 'width' must be an integer between 0 and 65535",
                  }));
    EXPECT_EQ(1u, log.count({
                      EventSeverity::Error,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't create sprite with invalid metrics",
                  }));
}

TEST(Sprite, SpriteParsingNullRatio) {
    FixtureLog log;

    const auto image_1x = util::read_file("test/fixtures/annotations/emerald.png");
    const auto json_1x = R"JSON({ "image": { "width": 32, "height": 32, "pixelRatio": 0 } })JSON";

    const auto images = parseSprite(image_1x, json_1x).get<Sprites>();
    EXPECT_EQ(0u, images.size());

    EXPECT_EQ(1u, log.count({
                      EventSeverity::Error,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't create sprite with invalid metrics",
                  }));
}
