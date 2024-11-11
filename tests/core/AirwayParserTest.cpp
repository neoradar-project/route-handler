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

        // Helper function to check fix details
        static bool CheckAirwayFix(const AirwayFix &fix,
                                   const std::string &name,
                                   double lat,
                                   double lng,
                                   const std::optional<uint32_t> &minimum_level)
        {
            return fix.name == name &&
                   std::abs(fix.coord.latitude().degrees() - lat) < 0.000001 &&
                   std::abs(fix.coord.longitude().degrees() - lng) < 0.000001 &&
                   fix.minimum_level == minimum_level;
        }
    };

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

        auto airways = network.getAllAirways();
        ASSERT_EQ(airways.size(), 1);

        const auto *airway = network.getAirway("V7");
        ASSERT_NE(airway, nullptr);
        EXPECT_EQ(airway->level, AirwayLevel::BOTH);

        // Check we can traverse in both directions since both are marked Y
        auto path1 = airway->getFixesBetween("05AWE", "ASIBE");
        ASSERT_TRUE(path1.size() == 2);
        EXPECT_TRUE(CheckAirwayFix(path1[0], "05AWE", 43.595222, 142.399147, std::nullopt));
        EXPECT_TRUE(CheckAirwayFix(path1[1], "ASIBE", 43.451106, 142.283592, 7000));
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

        // Should be able to traverse GIVMI to ERNAS
        auto path = airway->getFixesBetween("GIVMI", "ERNAS");
        ASSERT_TRUE(path.size() == 2);
        EXPECT_TRUE(CheckAirwayFix(path[0], "GIVMI", 48.701094, 11.364803, std::nullopt));
        EXPECT_TRUE(CheckAirwayFix(path[1], "ERNAS", 48.844669, 11.219353, 4000));
    }

    TEST_F(AirwayParserTest, NotEstablishedLevel)
    {
        const char *input =
            "ASPAT\t49.196175\t10.725828\t14\tT161\tL\t"
            "N\t"
            "DEBHI\t49.360833\t10.466111\tNESTB\tY\n";

        auto result = AirwayParser::ParseAirwayTxt(input);
        ASSERT_TRUE(result.has_value());
        const auto &network = *result;

        const auto *airway = network.getAirway("T161");
        ASSERT_NE(airway, nullptr);
        EXPECT_EQ(airway->level, AirwayLevel::LOW);

        auto path = airway->getFixesBetween("ASPAT", "DEBHI");
        ASSERT_TRUE(path.size() == 2);
        EXPECT_FALSE(path[1].minimum_level.has_value());
    }

    TEST_F(AirwayParserTest, DirectionalAirway)
    {
        // Example showing a one-way segment
        const char *input =
            "BRAIN\t51.811086\t0.651667\t14\tM197\tB\t"
            "KOBBI\t51.687222\t-0.155000\t09000\tN\t" // Can't go BRAIN->KOBBI
            "GASBA\t51.836111\t0.815556\t09000\tY\n"

            "KOBBI\t51.687222\t-0.155000\t14\tM197\tB\t"
            "IPRIL\t51.674722\t-0.226389\t09000\tN\t"
            "BRAIN\t51.811086\t0.651667\t09000\tY\n"

            "IPRIL\t51.674722\t-0.226389\t14\tM197\tB\t"
            "ICTAM\t51.527047\t-1.163367\t08500\tN\t"
            "KOBBI\t51.687222\t-0.155000\t09000\tY\n";

        auto result = AirwayParser::ParseAirwayTxt(input);
        ASSERT_TRUE(result.has_value());
        const auto &network = *result;

        const auto *airway = network.getAirway("M197");
        ASSERT_NE(airway, nullptr);

        // This direction should fail (BRAIN->KOBBI not allowed)
        EXPECT_THROW(airway->getFixesBetween("BRAIN", "KOBBI"), InvalidAirwayDirectionException);

        // This direction should work (KOBBI->BRAIN allowed)
        auto path1 = airway->getFixesBetween("KOBBI", "BRAIN");
        ASSERT_EQ(path1.size(), 2);

        // BRAIN->GASBA should be allowed
        auto path2 = airway->getFixesBetween("BRAIN", "GASBA");
        ASSERT_EQ(path2.size(), 2);

        EXPECT_THROW(airway->getFixesBetween("BRAIN", "IPRIL"), InvalidAirwayDirectionException);

        auto path3 = airway->getFixesBetween("IPRIL", "BRAIN");
        ASSERT_EQ(path3.size(), 3);
    }

    TEST_F(AirwayParserTest, MalformedInput)
    {
        std::vector<const char *> malformed_inputs = {
            // Too few fields
            "ASPAT\t49.196175\t10.725828\t14\tT161\n",
            // Invalid coordinates
            "ASPAT\tinvalid\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\tinvalid\t14\tT161\tB\tN\n",
            // Missing required fields
            "\t49.196175\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t\t10.725828\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\t\t14\tT161\tB\tN\n",
            "ASPAT\t49.196175\t10.725828\t\t\tB\tN\n",
            "ASPAT\t49.196175\t10.725828\t14\t\tB\tN\n"};

        for (const auto &input : malformed_inputs)
        {
            auto result = AirwayParser::ParseAirwayTxt(input);
            EXPECT_TRUE(!result.has_value() ||
                        result->getAllAirways().empty())
                << "Parser should reject malformed input: " << input;
        }
    }

#ifdef TEST_USE_TESTDATA
    TEST_F(AirwayParserTest, ParseFromFile)
    {
        auto content = ReadTestData("airways.txt");
        auto result = AirwayParser::ParseAirwayTxt(content);

        ASSERT_TRUE(result.has_value());
        const auto &network = *result;
        auto airways = network.getAllAirways();
        ASSERT_FALSE(airways.empty());

        // Test M197 airway directionality
        const auto *m197 = network.getAirway("M197");
        EXPECT_THROW(m197->getFixesBetween("REDFA", "IPRIL"), InvalidAirwayDirectionException);

        const auto *v7 = network.getAirway("V7");
        EXPECT_NO_THROW(v7->getFixesBetween("05AWE", "ASIBE"));
        EXPECT_NO_THROW(v7->getFixesBetween("ASIBE", "05AWE"));
    }
#endif

} // namespace RouteHandlerTests