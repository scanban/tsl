#include <catch.hpp>
#include <tsl/tsl.h>

using namespace Catch;

TEST_CASE("basic functionality tests", "[basic]") {
    const std::vector<int> source { 6, 1, 5, 2, 4, 3 };

    SECTION("vector source, vector destination") {
        const auto& result = tsl::stream(
                tsl::source(source),
                tsl::to_vector<int>()
        );
        REQUIRE_THAT(result.value(), Equals(source));
    }
}