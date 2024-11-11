#include "AirwayParser.h"
#include <gtest/gtest.h>
#include <fmt/core.h>
#include <fmt/color.h>
#include <string>
#include <fstream>
#include <filesystem>

using namespace RouteParser;

namespace RouteHandlerTests
{
    class AirwayParserTest : public ::testing::Test
    {
    protected:
        std::string ReadTestData(const std::string &filename)
        {
            std::filesystem::path test_dir = std::filesystem::current_path() / "testdata";
            auto filepath = test_dir / filename;

            std::ifstream file(filepath);
            if (!file.is_open())
            {
                throw std::runtime_error(
                    fmt::format("Could not open test file: {}", filepath.string()));
            }

            return std::string(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());
        }

        bool CheckAirwayFix(const AirwayFix &fix,
                            const std::string &name,
                            double lat,
                            double lon,
                            std::optional<uint32_t> level)
        {
            return fix.name == name &&
                   std::abs(fix.coord.latitude().degrees() - lat) < 0.000001 &&
                   std::abs(fix.coord.longitude().degrees() - lon) < 0.000001;
        }

        bool CheckSegment(const AirwaySegmentInfo &segment,
                          const std::string &fromName, double fromLat, double fromLon,
                          const std::string &toName, double toLat, double toLon,
                          uint32_t minLevel, bool canTraverse)
        {
            return CheckAirwayFix(segment.from, fromName, fromLat, fromLon, std::nullopt) &&
                   CheckAirwayFix(segment.to, toName, toLat, toLon, std::nullopt) &&
                   segment.minimum_level == minLevel &&
                   segment.canTraverse == canTraverse;
        }
    };

    TEST_F(AirwayParserTest, DirectionalM197Airway)
    {
        // Test data for M197 showing directional segments
        const char *input =
            "BRAIN\t51.811086\t0.651667\t14\tM197\tB\t"
            "KOBBI\t51.687222\t-0.155000\t09000\tN\t" // Can't go BRAIN->KOBBI
            "GASBA\t51.836111\t0.815556\t09000\tY\n"  // Can go BRAIN->GASBA

            "KOBBI\t51.687222\t-0.155000\t14\tM197\tB\t"
            "IPRIL\t51.674722\t-0.226389\t09000\tN\t"
            "BRAIN\t51.811086\t0.651667\t09000\tY\n" // Can go KOBBI->BRAIN

            "IPRIL\t51.674722\t-0.226389\t14\tM197\tB\t"
            "ICTAM\t51.527047\t-1.163367\t08500\tN\t"
            "KOBBI\t51.687222\t-0.155000\t09000\tY\n"; // Can go IPRIL->KOBBI

        auto result = AirwayParser::ParseAirwayTxt(input);
        ASSERT_TRUE(result.has_value()) << "Failed to parse airway data";

        const auto &network = *result;
        erkir::spherical::Point brainPoint(51.811086, 0.651667);
        const auto *airway = network.getAirwayByLatLon("M197", brainPoint);
        ASSERT_NE(airway, nullptr);

        // Test BRAIN->GASBA (should work)
        EXPECT_NO_THROW({
            auto segments = network.getSegmentsBetween("M197", "BRAIN", "GASBA", brainPoint);
            ASSERT_EQ(segments.size(), 1) << "Expected one segment for BRAIN->GASBA";
            EXPECT_TRUE(CheckSegment(
                segments[0],
                "BRAIN", 51.811086, 0.651667,
                "GASBA", 51.836111, 0.815556,
                9000, true));
        }) << "Should be able to traverse BRAIN->GASBA";

        // Test BRAIN->KOBBI (should fail)
        EXPECT_THROW(
            network.getSegmentsBetween("M197", "BRAIN", "KOBBI", brainPoint),
            InvalidAirwayDirectionException)
            << "Should not be able to traverse BRAIN->KOBBI";

        // Test KOBBI->BRAIN (should work)
        EXPECT_NO_THROW({
            auto segments = network.getSegmentsBetween("M197", "KOBBI", "BRAIN", brainPoint);
            ASSERT_EQ(segments.size(), 1) << "Expected one segment for KOBBI->BRAIN";
            EXPECT_TRUE(CheckSegment(
                segments[0],
                "KOBBI", 51.687222, -0.155000,
                "BRAIN", 51.811086, 0.651667,
                9000, true));
        }) << "Should be able to traverse KOBBI->BRAIN";

        // Test BRAIN->IPRIL (should fail)
        EXPECT_THROW(
            network.getSegmentsBetween("M197", "BRAIN", "IPRIL", brainPoint),
            InvalidAirwayDirectionException)
            << "Should not be able to traverse BRAIN->IPRIL";

        // Verify ordered fixes
        auto orderedFixes = airway->getFixesInOrder();
        std::vector<std::string> expectedOrder = {"IPRIL", "KOBBI", "BRAIN", "GASBA", "ICTAM"};
        ASSERT_EQ(orderedFixes.size(), expectedOrder.size());
        for (size_t i = 0; i < orderedFixes.size(); ++i)
        {
            EXPECT_EQ(orderedFixes[i].name, expectedOrder[i]);
        }
    }

    TEST_F(AirwayParserTest, EmptyInput)
    {
        auto result = AirwayParser::ParseAirwayTxt("");
        EXPECT_TRUE(result.has_value());
        const auto &network = *result;
        EXPECT_EQ(network.getAllAirways().size(), 0);
    }

    TEST_F(AirwayParserTest, SingleValidAirway)
    {
        const char *input =
            "05AWE\t43.595222\t142.399147\t14\tV7\tB\t"
            "AWE\t43.667264\t142.456847\t05000\tY\t"
            "ASIBE\t43.451106\t142.283592\t07000\tY\n";

        auto result = AirwayParser::ParseAirwayTxt(input);
        ASSERT_TRUE(result.has_value());
        const auto &network = *result;

        const auto *airway = network.getAirwayByLatLon("V7", erkir::spherical::Point(43.595222, 142.399147));
        ASSERT_NE(airway, nullptr);
        EXPECT_EQ(airway->level, AirwayLevel::BOTH);

        // Verify ordered fixes
        auto orderedFixes = airway->getFixesInOrder();
        std::vector<std::string> expectedOrder = {"05AWE", "AWE", "ASIBE"};
        ASSERT_EQ(orderedFixes.size(), expectedOrder.size());
        for (size_t i = 0; i < orderedFixes.size(); ++i)
        {
            EXPECT_EQ(orderedFixes[i].name, expectedOrder[i]);
        }

        // // Check traversal
        // auto segments = airway->getSegmentsBetween("05AWE", "ASIBE");
        // ASSERT_EQ(segments.size(), 2) << "Expected two segments for full path";
        // EXPECT_TRUE(CheckSegment(
        //     segments[0],
        //     "05AWE", 43.595222, 142.399147,
        //     "AWE", 43.667264, 142.456847,
        //     5000, true));
    }

    TEST_F(AirwayParserTest, NoNeighbours)
    {
        const char *input =
            "GIVMI\t48.701094\t11.364803\t14\tY101\tB\t"
            "N\t"
            "ERNAS\t48.844669\t11.219353\t04000\tY\n";

        auto result = AirwayParser::ParseAirwayTxt(input);
        ASSERT_TRUE(result.has_value());
        const auto &network = *result;

        const auto *airway = network.getAirway("Y101");
        ASSERT_NE(airway, nullptr);

        auto orderedFixes = airway->getFixesInOrder();
        ASSERT_EQ(orderedFixes.size(), 2);
        EXPECT_EQ(orderedFixes[0].name, "GIVMI");
        EXPECT_EQ(orderedFixes[1].name, "ERNAS");
    }

    TEST_F(AirwayParserTest, MalformedInput)
    {
        std::vector<const char *> malformed_inputs = {
            "ASPAT\t49.196175\t10.725828\t14\tT161\n",
            "ASPAT\tinvalid\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\tinvalid\t14\tT161\tB\tN\n",
            "\t49.196175\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\t\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\t10.725828\t\t\tB\tN\n",
            "ASPAT\t49.196175\t10.725828\t14\t\tB\tN\n"};

        for (const auto &input : malformed_inputs)
        {
            auto result = AirwayParser::ParseAirwayTxt(input);
            EXPECT_TRUE(!result.has_value() || result->getAllAirways().empty())
                << "Parser should reject malformed input: " << input;
        }
    }

    TEST_F(AirwayParserTest, ParseFromFile)
    {
        auto content = ReadTestData("airways.txt");
        auto result = AirwayParser::ParseAirwayTxt(content);

        ASSERT_TRUE(result.has_value()) << "Failed to parse airways file";
        const auto &network = *result;
        auto airways = network.getAllAirways();
        ASSERT_FALSE(airways.empty()) << "No airways were parsed from file";

        // // Test M197 airway directionality
        // {
        //     erkir::spherical::Point m197Point(51.811086, 0.651667); // BRAIN's location
        //     EXPECT_THROW(
        //         network.getSegmentsBetween("M197", "REDFA", "IPRIL", m197Point),
        //         InvalidAirwayDirectionException);

        //     EXPECT_NO_THROW(
        //         network.getSegmentsBetween("M197", "IPRIL", "REDFA", m197Point))
        //         << "Should not be able to traverse M197 from REDFA to IPRIL";
        // }

        {
            erkir::spherical::Point waypoint(14.006039, 108.024406);
            const auto *w11 = network.getAirwayByLatLon("W11", waypoint);
            ASSERT_NE(w11, nullptr) << "Failed to find W11 airway near specified point";

            auto result = w11->getSegmentsBetween("PLK", "CQ");

            ASSERT_EQ(result.size(), 3) << "Expected two segments for PLK->CQ";

            // Print all fixes in W11 for debugging
            std::cout << "W11 fixes: ";
            for (const auto &fix : w11->getFixesInOrder())
            {
                std::cout << fix.name << " ";
            }
            std::cout << std::endl;
        }

        {
            erkir::spherical::Point hamburg(53.6304, 9.9883);

            const auto *airway = network.getAirwayByLatLon("L23", hamburg);

            auto result = airway->getFixesInOrder();

            for (const auto &fix : result)
            {
                std::cout << fix.name << " ";
            }
        }
    }

} // namespace RouteHandlerTests