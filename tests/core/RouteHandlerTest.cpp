#include <gtest/gtest.h>
#include "RouteHandler.h"

using namespace RouteParser;

namespace RouteHandlerTests
{
    class RouteHandlerTest : public ::testing::Test
    {
    protected:
        RouteHandler handler;

        void SetUp() override
        {
            // Any setup needed before each test
        }
    };

    TEST_F(RouteHandlerTest, GetParser)
    {
        auto parser = handler.GetParser();
        ASSERT_NE(parser, nullptr);
    }
}