#include <string>
#include "mlReview/database/aqms/credit.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace MLReview::Database::AQMS;

TEST_CASE("MLReview::Database::AQMS::Credit", "[database][aqms][credit]")
{
    Credit credit;

    SECTION("Defaults")
    {
        REQUIRE_FALSE(credit.haveIdentifier());
        REQUIRE_FALSE(credit.haveTable());
        REQUIRE_FALSE(credit.haveReference());
        REQUIRE_THROWS_AS(credit.getIdentifier(), std::runtime_error);
        REQUIRE_THROWS_AS(credit.getTable(), std::runtime_error);
        REQUIRE_THROWS_AS(credit.getReference(), std::runtime_error);
    }

    SECTION("Setters and getters")
    {
        credit.setIdentifier(0);
        credit.setTable(Credit::Table::Netmag);
        credit.setReference("RT1");

        REQUIRE(credit.getIdentifier() == 0);
        REQUIRE(credit.getTable() == Credit::Table::Netmag);
        REQUIRE(credit.getReference() == "RT1");
    }

    SECTION("Reference is truncated to 16 characters")
    {
        credit.setReference("ABCDEFGHIJKLMNOPQRS");
        REQUIRE(credit.getReference() == "ABCDEFGHIJKLMNOP");
    }

    SECTION("Whitespace is removed from the reference")
    {
        credit.setReference(" R T1 ");
        REQUIRE(credit.getReference() == "RT1");
    }

    SECTION("Invalid input")
    {
        REQUIRE_THROWS_AS(credit.setIdentifier(-1), std::invalid_argument);
        REQUIRE_THROWS_AS(credit.setReference(""), std::invalid_argument);
        REQUIRE_THROWS_AS(credit.setReference("   "), std::invalid_argument);
    }

    SECTION("Copy, move, and clear")
    {
        credit.setIdentifier(8);
        credit.setTable(Credit::Table::Origin);

        Credit copy{credit};
        REQUIRE(copy.getIdentifier() == 8);
        REQUIRE(copy.getTable() == Credit::Table::Origin);

        Credit moved{std::move(copy)};
        REQUIRE(moved.getIdentifier() == 8);

        credit.clear();
        REQUIRE_FALSE(credit.haveIdentifier());
        REQUIRE_FALSE(credit.haveTable());
        REQUIRE(moved.haveIdentifier());
    }
}
