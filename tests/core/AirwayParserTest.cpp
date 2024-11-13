#include "AirwayParser.h"
#include <gtest/gtest.h>
#include <fstream>
#include "Navdata.h"

using namespace RouteParser;

namespace RouteHandlerTests
{
    class AirwayNetworkTest : public ::testing::Test
    {
    protected:
        bool CheckAirwayFix(const Waypoint &fix,
                            const std::string &name,
                            double lat,
                            double lon)
        {
            return fix.getIdentifier() == name &&
                   std::abs(fix.getPosition().latitude().degrees() - lat) < 0.000001 &&
                   std::abs(fix.getPosition().longitude().degrees() - lon) < 0.000001;
        }

        bool CheckSegment(const AirwaySegmentInfo &segment,
                          const std::string &fromName, double fromLat, double fromLon,
                          const std::string &toName, double toLat, double toLon,
                          uint32_t minLevel, bool canTraverse)
        {
            return CheckAirwayFix(segment.from, fromName, fromLat, fromLon) &&
                   CheckAirwayFix(segment.to, toName, toLat, toLon) &&
                   segment.minimum_level == minLevel &&
                   segment.canTraverse == canTraverse;
        }

        std::string ReadFile(const char *filename)
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                return "";
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    };

    TEST_F(AirwayNetworkTest, AddAirwaySegment)
    {
        const char *input =
            "ASPAT\t49.196175\t10.725828\t14\tT161\tL\t"
            "N\t"
            "DEBHI\t49.360833\t10.466111\t18000\tY\n";

        auto network = AirwayParser::ParseAirwayTxt(input);

        auto airway = network->getAirway("T161");
        ASSERT_NE(airway, nullptr);
        EXPECT_EQ(airway->level, AirwayLevel::LOW);

        auto segments = airway->getSegmentsBetween("ASPAT", "DEBHI");
        ASSERT_EQ(segments.size(), 1);
        EXPECT_TRUE(CheckSegment(segments[0], "ASPAT", 49.196175, 10.725828,
                                 "DEBHI", 49.360833, 10.466111, 18000, true));
    }

    TEST_F(AirwayNetworkTest, GetAirwaysByNameAndFix)
    {
        const char *input =
            "BANSU\t14.286111\t108.159722\t14\tW11\tL\t"
            "PLK\t14.006039\t108.024406\t10000\tY\t"
            "TALAP\t14.415000\t108.221111\t10000\tY\n";

        auto network = AirwayParser::ParseAirwayTxt(input);
        auto airways = network->getAirwaysByNameAndFix("W11", "BANSU");
        ASSERT_EQ(airways.size(), 1);
    }

    TEST_F(AirwayNetworkTest, GetFixesBetween)
    {
        const char *input =
            "ASPAT\t49.196175\t10.725828\t14\tT161\tL\t"
            "N\t"
            "DEBHI\t49.360833\t10.466111\t18000\tY\n"

            "DEBHI\t49.360833\t10.466111\t14\tT161\tL\t"
            "ASPAT\t49.196175\t10.725828\t18000\tY\t"
            "URSAL\t49.525000\t10.206944\t18000\tY\n";

        auto network = AirwayParser::ParseAirwayTxt(input);

        auto fixes = network->getFixesBetween("T161", "ASPAT", "URSAL", {49.360833, 10.466111});
        ASSERT_EQ(fixes.size(), 3);
        EXPECT_TRUE(CheckAirwayFix(fixes[0], "ASPAT", 49.196175, 10.725828));
        EXPECT_TRUE(CheckAirwayFix(fixes[1], "DEBHI", 49.360833, 10.466111));
        EXPECT_TRUE(CheckAirwayFix(fixes[2], "URSAL", 49.525000, 10.206944));
    }

    TEST_F(AirwayNetworkTest, ValidateRoute)
    {
        const char *input =
            "IDESI\t51.897706\t1.885578\t14\tY6\tB\t"
            "BANEM\t52.335556\t1.505278\t16500\tY\t"
            "TOSVA\t51.677056\t2.073983\t10500\tN\n"

            "TOSVA\t51.677056\t2.073983\t14\tY6\tB\t"
            "IDESI\t51.897706\t1.885578\t10500\tY\t"
            "SUMUM\t51.637281\t2.107706\t10500\tN\n"

            "BANEM\t52.335556\t1.505278\t14\tY6\tB\t"
            "N\t"
            "IDESI\t51.897706\t1.885578\t16500\tN\n"

            "SUMUM\t51.637281\t2.107706\t14\tY6\tB\t"
            "TOSVA\t51.677056\t2.073983\t10500\tY\t"
            "N\n";

        auto network = AirwayParser::ParseAirwayTxt(input);

        network->finalizeAirways();

        // Test with sufficient level
        {
            auto result = network->validateAirwayTraversal("SUMUM", "Y6", "IDESI", 11000, {51.677056, 2.073983});
            EXPECT_TRUE(result.isValid);
            EXPECT_TRUE(result.errors.empty());
            ASSERT_EQ(result.segments.size(), 2);
            EXPECT_TRUE(CheckSegment(result.segments[0], "SUMUM", 51.637281, 2.107706,
                                     "TOSVA", 51.677056, 2.073983, 10500, true));
            EXPECT_TRUE(CheckSegment(result.segments[1], "TOSVA", 51.677056, 2.073983,
                                     "IDESI", 51.897706, 1.885578, 10500, true));
        }

        // Test with insufficient level
        {
            auto result = network->validateAirwayTraversal("SUMUM", "Y6", "IDESI", 10000, {51.677056, 2.073983});
            EXPECT_FALSE(result.isValid);
            ASSERT_FALSE(result.errors.empty());
            EXPECT_EQ(result.errors[0].type, INSUFFICIENT_FLIGHT_LEVEL);
            EXPECT_TRUE(result.errors[0].message.find("10500") != std::string::npos);
        }

        // Test invalid direction
        {
            auto result = network->validateAirwayTraversal("IDESI", "Y6", "SUMUM", 20000, {51.677056, 2.073983});
            EXPECT_FALSE(result.isValid);
            ASSERT_FALSE(result.errors.empty());
            EXPECT_EQ(result.errors[0].type, INVALID_AIRWAY_DIRECTION);
        }
    }

#ifdef TEST_USE_TESTDATA

    TEST_F(AirwayNetworkTest, ParseAirwayTxt)
    {
        auto network = AirwayParser::ParseAirwayTxt(ReadFile("testdata/airways.txt"));
        ASSERT_TRUE(network.has_value());

        auto result = network->validateRoute("SUMUM Y6 IDESI P7 LOGAN");
        EXPECT_TRUE(result.isValid);
        ASSERT_EQ(result.segments.size(), 3);
        EXPECT_TRUE(CheckSegment(result.segments[0], "SUMUM", 51.637281, 2.107706,
                                 "TOSVA", 51.677056, 2.073983, 10500, true));
        EXPECT_TRUE(CheckSegment(result.segments[1], "TOSVA", 51.677056, 2.073983,
                                 "IDESI", 51.897706, 1.885578, 10500, true));

        result = network->validateRoute("BANEM Y6 SUMUM");
        EXPECT_FALSE(result.isValid);
        ASSERT_EQ(result.errors.size(), 1);
        EXPECT_EQ(result.errors[0].type, INVALID_AIRWAY_DIRECTION);
        EXPECT_EQ(result.errors[0].message, "Cannot traverse airway Y6 from BANEM to SUMUM");

        {
            result = network->validateRoute("BARAD M747 SUBUT T916 LEYLA N161 SARIN A368 FKG W192 ESDEX W191 DNC W565 XIXAN B330 KWE W181 DUDIT A599 POU R473 SIERA");

            if (!result.errors.empty())
            {
                for (const auto &error : result.errors)
                {
                    std::cout << error.message << std::endl;
                }
            }

            for (const auto &segment : result.segments)
            {
                std::cout << " " << segment.to.getIdentifier();
            }
        }
    }

#endif
}