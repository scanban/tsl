#include <catch.hpp>
#include <string>
#include <tsl/tsl.h>
using namespace Catch;

namespace {
    template <typename T, typename Compare = std::less<T>>
            std::vector<T> sorted(const std::vector<T>& v) {
        auto result = v;
        std::sort(std::begin(result), std::end(result), Compare());
        return result;
    };
}

TEST_CASE("basic functionality tests", "[basic]") {
    const std::vector<int> source { 6, 1, 5, 2, 4, 3 };
    const std::vector<std::string> string_source { "s6", "s1", "s5", "s2" };

    SECTION("vector source, vector destination") {
        const auto& result = tsl::stream(
                tsl::source(source),
                tsl::to_vector<int>()
        );
        REQUIRE_THAT(result.value(), Equals(source));
    }

    SECTION("move: vector source, vector destination") {
        auto test = string_source;
        const auto& result = tsl::stream(
                tsl::source(std::move(test)),
                tsl::to_vector<std::string>()
        );
        REQUIRE_THAT(result.value(), Equals(string_source));
        REQUIRE(std::all_of(test.cbegin(), test.cend(), [](const auto& v){ return v.empty();})); // NOLINT
    }

    SECTION("sort: vector source, vector destination") {
        const auto& result = tsl::stream(
                tsl::source(source),
                tsl::sort<int>(),
                tsl::to_vector<int>()
        );
        REQUIRE_THAT(result.value(), Equals(sorted(source)));
    }
}