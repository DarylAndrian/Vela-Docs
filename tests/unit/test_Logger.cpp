// test_Logger.cpp — v0.3.1 (CRIT-05)
#include <catch2/catch_test_macros.hpp>
#include "util/Logger.h"

TEST_CASE("Logger — info() does not throw", "[logger]") {
    vela::Logger::info("hello");
    SUCCEED();
}

TEST_CASE("Logger — warning() does not throw", "[logger]") {
    vela::Logger::warning("warn me");
    SUCCEED();
}
