/*
 * Copyright (c) 2021 SiKol Ltd.
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <array>
#include <string>
using namespace std::string_literals;
using namespace std::string_view_literals;

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <fmt/core.h>

#include "sk/patricia.hxx"

using sk::bit_t;
using sk::patricia_key;
using sk::patricia_set;
using sk::patricia_trie;
using sk::detail::bit_diff;

TEST_CASE("bit_diff")
{
    std::array a{std::byte{0b11111111}, std::byte{0b00000000}};
    std::array b{std::byte{0b00000000}, std::byte{0b11111111}};

    REQUIRE(bit_diff(a, b) == 0);

    a = {std::byte{0b11100000}, std::byte{0b00000000}};
    b = {std::byte{0b11110100}, std::byte{0b00000000}};

    REQUIRE(bit_diff(a, b) == 3);

    a = {std::byte{0b11110100}, std::byte{0b00000010}};
    b = {std::byte{0b11110100}, std::byte{0b00000000}};

    REQUIRE(bit_diff(a, b) == 14);

    a = {std::byte{0b11110100}, std::byte{0b00000011}};
    b = {std::byte{0b11110100}, std::byte{0b00000011}};

    REQUIRE(bit_diff(a, b) == 16);
}

TEST_CASE("patricia_key")
{
    patricia_key a, b;

    REQUIRE(a == b);

    a = patricia_key("foo", 8);
    b = patricia_key("foo", 8);
    REQUIRE(a.size_bytes() == 1);
    REQUIRE(a.size_bits() == 8);
    REQUIRE(a == b);

    a = patricia_key("foo", 8);
    b = patricia_key("foo", 7);
    REQUIRE(b.size_bytes() == 1);
    REQUIRE(b.size_bits() == 7);
    REQUIRE(a != b);

    a = patricia_key("foo", 8);
    b = patricia_key("foo", 8);
    REQUIRE(a == b);

    a = patricia_key("foa", 15);
    b = patricia_key("fob", 15);
    REQUIRE(a.size_bytes() == 2);
    REQUIRE(a.size_bits() == 15);
    REQUIRE(a == b);

    a = patricia_key("foa", 16);
    b = patricia_key("fob", 16);
    REQUIRE(a.size_bytes() == 2);
    REQUIRE(a.size_bits() == 16);
    REQUIRE(a == b);

    a = patricia_key("foa", 17);
    b = patricia_key("fob", 17);
    REQUIRE(a.size_bytes() == 3);
    REQUIRE(a.size_bits() == 17);
    REQUIRE(a == b);

    std::byte ba{0b10110100};
    std::byte bb{0b10100100};

    a = patricia_key(std::span(&ba, 1), 3);
    b = patricia_key(std::span(&bb, 1), 3);
    REQUIRE(a.size_bytes() == 1);
    REQUIRE(a.size_bits() == 3);
    REQUIRE(a == b);

    a = patricia_key(std::span(&ba, 1), 4);
    b = patricia_key(std::span(&bb, 1), 4);
    REQUIRE(a != b);
}

TEST_CASE("patricia bit masks")
{
    std::byte b1{0b11110000};
    std::byte b2{0b11111000};

    {
        patricia_key k1(std::span(&b1, 1), 3);
        patricia_key k2(std::span(&b2, 1), 3);

        patricia_trie<int> trie;
        REQUIRE(trie.insert(k1, 42));

        auto r = trie.find(k1);
        REQUIRE(r);
        REQUIRE(*r == 42);

        r = trie.find(k2);
        REQUIRE(r);
        REQUIRE(*r == 42);
    }

    {
        patricia_key k1(std::span(&b1, 1), 5);
        patricia_key k2(std::span(&b2, 1), 5);

        patricia_trie<int> trie;
        REQUIRE(trie.insert(k1, 42));

        auto r = trie.find(k1);
        REQUIRE(r);
        REQUIRE(*r == 42);

        r = trie.find(k2);
        REQUIRE(!r);
    }

    {
        patricia_key k1(std::span(&b1, 1), 4);
        patricia_key k2(std::span(&b1, 1), 5);

        patricia_trie<int> trie;
        REQUIRE(trie.insert(k1, 42));

        auto r = trie.find(k1);
        REQUIRE(r);
        REQUIRE(*r == 42);

        r = trie.find(k2);
        REQUIRE(!r);
    }
}

TEST_CASE("patricia byte inserts")
{
    patricia_trie<int> trie;

    std::byte b1{0b11110000};
    std::byte b2{0b11111000};
    std::byte b3{0b11101000};
    std::byte b4{0b11111100};

    auto b = trie.insert(std::span(&b1, 1), 42);
    REQUIRE(b);

    b = trie.insert(std::span(&b2, 1), 666);
    REQUIRE(b);

    auto r = trie.find(std::span(&b1, 1));
    REQUIRE(r);
    REQUIRE(*r == 42);

    r = trie.find(std::span(&b2, 1));
    REQUIRE(r);
    REQUIRE(*r == 666);

    r = trie.find(std::span(&b3, 1));
    REQUIRE(!r);

    r = trie.find(std::span(&b4, 1));
    REQUIRE(!r);

    b = trie.remove(std::span(&b2, 1));
    REQUIRE(b);

    b = trie.remove(std::span(&b2, 1));
    REQUIRE(!b);
}

TEST_CASE("patricia_trie basic inserts")
{
    std::vector<std::string_view> test_strings{
        "test"sv,
        "testep"sv,
        "tester"sv,
        "testzz"sv,
        //"tesp"sv,
        //"tezn"sv,
        "team"sv,
        "tam"sv,
        "foo"sv,
        "bar"sv,
    };

    std::ranges::sort(test_strings);

    do {
        patricia_trie<std::string> trie;

        std::string info;
        for (auto &&s : test_strings)
            info += "->" + std::string(s);

        INFO(info);

        for (auto &&s : test_strings) {
            auto b = trie.insert(s, std::string(s));
            REQUIRE(b);

            for (auto &&s_ : test_strings) {
                auto r = trie.find(s_);
                REQUIRE(r);
                REQUIRE(*r == s_);
                if (s_ == s)
                    break;
            }
        }

        for (std::size_t i = 0; i < test_strings.size(); ++i) {
            auto b = trie.remove(test_strings[i]);
            REQUIRE(b);

            for (std::size_t j = 0; j < test_strings.size(); ++j) {
                auto r = trie.find(test_strings[j]);
                if (j <= i) {
                    REQUIRE(!r);
                } else {
                    REQUIRE(r);
                    REQUIRE(*r == test_strings[j]);
                }
            }
        }
    } while (std::ranges::next_permutation(test_strings).found);
}

TEST_CASE("patricia_key_maker<std::uint16_t>")
{
    sk::patricia_key_maker<std::uint16_t> km;
    auto k = km(0x1234);
    REQUIRE(k.key[0] == std::byte{0x12});
    REQUIRE(k.key[1] == std::byte{0x34});
}

TEST_CASE("patricia_key_maker<std::int16_t>")
{
    sk::patricia_key_maker<std::int16_t> km;
    auto k = km(-0x1234);
    REQUIRE(k.key[0] == std::byte{0x6d});
    REQUIRE(k.key[1] == std::byte{0xcc});
}

TEST_CASE("patricia_key_maker<std::uint32_t>")
{
    sk::patricia_key_maker<std::uint32_t> km;
    auto k = km(0x12345678);
    REQUIRE(k.key[0] == std::byte{0x12});
    REQUIRE(k.key[1] == std::byte{0x34});
    REQUIRE(k.key[2] == std::byte{0x56});
    REQUIRE(k.key[3] == std::byte{0x78});
}

TEST_CASE("patricia_key_maker<std::int32_t>")
{
    sk::patricia_key_maker<std::int32_t> km;
    auto k = km(-0x12345678);
    REQUIRE(k.key[0] == std::byte{0x6d});
    REQUIRE(k.key[1] == std::byte{0xcb});
    REQUIRE(k.key[2] == std::byte{0xa9});
    REQUIRE(k.key[3] == std::byte{0x88});
}

TEST_CASE("patricia_key_maker<std::uint64_t>")
{
    sk::patricia_key_maker<std::uint64_t> km;
    auto k = km(0x1234567811223344ULL);
    REQUIRE(k.key[0] == std::byte{0x12});
    REQUIRE(k.key[1] == std::byte{0x34});
    REQUIRE(k.key[2] == std::byte{0x56});
    REQUIRE(k.key[3] == std::byte{0x78});
    REQUIRE(k.key[4] == std::byte{0x11});
    REQUIRE(k.key[5] == std::byte{0x22});
    REQUIRE(k.key[6] == std::byte{0x33});
    REQUIRE(k.key[7] == std::byte{0x44});
}

TEST_CASE("patricia_key_maker<std::int64_t>")
{
    sk::patricia_key_maker<std::int64_t> km;
    auto k = km(-0x1234567811223344LL);
    REQUIRE(k.key[0] == std::byte{0x6d});
    REQUIRE(k.key[1] == std::byte{0xcb});
    REQUIRE(k.key[2] == std::byte{0xa9});
    REQUIRE(k.key[3] == std::byte{0x87});
    REQUIRE(k.key[4] == std::byte{0xee});
    REQUIRE(k.key[5] == std::byte{0xdd});
    REQUIRE(k.key[6] == std::byte{0xcc});
    REQUIRE(k.key[7] == std::byte{0xbc});
}

TEST_CASE("patricia_set<std::string>")
{
    patricia_set<std::string> s;

    REQUIRE(s.empty());

    REQUIRE(s.insert("foo").second);
    REQUIRE(s.insert("foobar").second);
    REQUIRE(s.insert("bar").second);

    REQUIRE(!s.empty());

    REQUIRE(s.contains("foo"));
    REQUIRE(s.contains("foobar"));
    REQUIRE(s.contains("bar"));
}

TEST_CASE("patricia_set<int>")
{
    patricia_set<int> s;

    REQUIRE(s.empty());

    REQUIRE(s.insert(1).second);
    REQUIRE(s.insert(42).second);
    REQUIRE(s.insert(666).second);

    REQUIRE(!s.empty());

    REQUIRE(s.contains(1));
    REQUIRE(s.contains(42));
    REQUIRE(s.contains(666));
}

TEST_CASE("patricia_map<std::string, int>")
{
    sk::patricia_map<std::string, int> m;

    REQUIRE(m.empty());

    m["foo"] = 1;
    m["foobar"] = 42;
    m["bar"] = 666;

    REQUIRE(!m.empty());

    REQUIRE(m["foo"] == 1);
    REQUIRE(m["foobar"] == 42);
    REQUIRE(m["bar"] == 666);
}

TEST_CASE("patricia_set<std::string>::iterator")
{
    sk::patricia_set<std::string> set;

    std::vector<std::string> values{"foo", "bar", "foobar", "quux"};

    for (auto &&value : values) {
        auto r = set.insert(value);
        REQUIRE(r.second);
        REQUIRE(*r.first == value);
    }

    std::ranges::sort(values);

    auto it = set.begin(), end = set.end();

    for (auto &&value : values) {
        REQUIRE(it != end);
        INFO(*it);
        REQUIRE(*it == value);
        ++it;
    }

    REQUIRE(it == end);
}

TEST_CASE("patricia_set<int>::iterator")
{
    sk::patricia_set<int> set;

    std::vector<int> values{std::numeric_limits<int>::min(),
                            -1938482,
                            -42,
                            -1,
                            0,
                            1,
                            20,
                            512,
                            65535,
                            std::numeric_limits<int>::max()};

    for (auto &&value : values) {
        INFO(value);
        auto r = set.insert(value);
        REQUIRE(r.second);
        REQUIRE(*r.first == value);
    }

    std::ranges::sort(values);

    auto it = set.begin(), end = set.end();

    for (auto &&value : values) {
        REQUIRE(it != end);
        INFO(*it);
        REQUIRE(*it == value);
        ++it;
    }

    REQUIRE(it == end);
}

TEST_CASE("patricia_map::iterator")
{
    sk::patricia_map<int, std::string> map;

    map.insert(std::pair{666, "foobar"});
    map.insert(std::pair{42, "bar"});
    map.insert(std::pair{1024, "quux"});
    map.insert(std::pair{1, "foo"});

    auto it = map.begin(), end = map.end();

    REQUIRE(it->first == 1);
    REQUIRE(it->second == "foo");
    ++it;
    REQUIRE(it != end);

    REQUIRE(it->first == 42);
    REQUIRE(it->second == "bar");
    ++it;
    REQUIRE(it != end);

    REQUIRE(it->first == 666);
    REQUIRE(it->second == "foobar");
    ++it;
    REQUIRE(it != end);

    REQUIRE(it->first == 1024);
    REQUIRE(it->second == "quux");
    ++it;
    REQUIRE(it == end);
}

auto random_set_test(unsigned int seed) -> bool
{
    std::default_random_engine engine(seed);

    std::uniform_int_distribution<unsigned int> rand_nitems(1, 500);
    std::uniform_int_distribution<unsigned int> rand_itemlen(20, 50);
    std::uniform_int_distribution<unsigned> rand_byte(0, 255);

    std::set<std::vector<std::byte>> items;

    // How many items will we put in the set?
    auto nitems = rand_nitems(engine);
    for (unsigned i = 0; i < nitems; ++i) {
        // How long will this item be?
        unsigned len = rand_itemlen(engine);

        std::vector<std::byte> item(len);
        std::ranges::generate(
            item, [&]() -> std::byte { return std::byte(rand_byte(engine)); });

        items.insert(std::move(item));
    }

    sk::patricia_set<std::vector<std::byte>> set;

    for (auto &&item : items) {
        auto i = set.insert(item);
        REQUIRE(i.second);
    }

    for (auto &&item : items) {
        auto i = set.find(item);
        REQUIRE(i != set.end());
        REQUIRE(*i == item);
    }

    for (auto &&item : items) {
        REQUIRE(set.erase(item) == 1);
    }

    REQUIRE(set.empty());

    return true;
}

TEST_CASE("patricia_set random test")
{
    // This test is non-deterministic, so it may fail on one run and then
    // succeed on the next.  The intention is that if the test fails, the
    // failing seed can be added as a fixed test case.

    std::random_device r;

    for (unsigned i = 0; i < 500; ++i) {
        auto seed = r();
        INFO(seed);
        REQUIRE(random_set_test(seed));
    }
}

auto random_map_test(unsigned int seed) -> bool
{
    std::default_random_engine engine(seed);

    std::uniform_int_distribution<unsigned int> rand_nitems(1, 500);
    std::uniform_int_distribution<unsigned int> rand_itemlen(20, 50);
    std::uniform_int_distribution<unsigned> rand_byte(0, 255);

    std::set<std::vector<std::byte>> items;

    // How many items will we put in the set?
    auto nitems = rand_nitems(engine);
    for (unsigned i = 0; i < nitems; ++i) {
        // How long will this item be?
        unsigned len = rand_itemlen(engine);

        std::vector<std::byte> item(len);
        std::ranges::generate(
            item, [&]() -> std::byte { return std::byte(rand_byte(engine)); });

        items.insert(std::move(item));
    }

    sk::patricia_map<std::vector<std::byte>, std::size_t> map;

    for (auto &&item : items) {
        auto i = map.insert(std::pair{item, item.size()});
        REQUIRE(i.second);
    }

    for (auto &&item : items) {
        auto i = map.find(item);
        REQUIRE(i != map.end());
        REQUIRE(i->first == item);
        REQUIRE(i->second == item.size());
    }

    for (auto &&item : items) {
        REQUIRE(map.erase(item) == 1);
    }

    REQUIRE(map.empty());

    return true;
}

TEST_CASE("patricia_map random test")
{
    // This test is non-deterministic, so it may fail on one run and then
    // succeed on the next.  The intention is that if the test fails, the
    // failing seed can be added as a fixed test case.

    std::random_device r;

    for (unsigned i = 0; i < 500; ++i) {
        auto seed = r();
        INFO(seed);
        REQUIRE(random_map_test(seed));
    }
}

TEST_CASE("patricia_set const functions")
{
    patricia_set<int> set;

    set.insert(42);
    set.insert(666);

    patricia_set<int> const &cset(set);
    auto it = cset.begin();
    REQUIRE(it != cset.end());
    REQUIRE(*it++ == 42);
    REQUIRE(*it++ == 666);
    REQUIRE(it == cset.end());

    it = cset.cbegin();
    REQUIRE(it != cset.cend());
    REQUIRE(*it++ == 42);
    REQUIRE(*it++ == 666);
    REQUIRE(it == cset.cend());

    REQUIRE(!cset.empty());
    REQUIRE(cset.find(42) != cset.end());
}

TEST_CASE("patricia_set range for") {
    patricia_set<int> set;

    set.insert(1);
    set.insert(2);
    set.insert(3);
    int i = 0;
    for (auto &&item: set)
        REQUIRE(item == ++i);
    REQUIRE(i == 3);
}

TEST_CASE("patricia_set empty string") {
    patricia_set<std::string> s;

    s.insert("");
    REQUIRE(*s.find(std::string()) == "");
}

TEST_CASE("patricia_set const range for") {
    patricia_set<int> set;

    set.insert(1);
    set.insert(2);
    set.insert(3);

    patricia_set<int> const &cset(set);
    int i = 0;
    for (auto &&item: cset)
        REQUIRE(item == ++i);
    REQUIRE(i == 3);
}

TEST_CASE("patricia_set typedefs")
{
    REQUIRE(std::same_as<int, patricia_set<int>::value_type>);
    REQUIRE(std::same_as<int, patricia_set<int>::key_type>);
    REQUIRE(std::same_as<std::size_t, patricia_set<int>::size_type>);
    REQUIRE(std::same_as<std::ptrdiff_t, patricia_set<int>::difference_type>);
    REQUIRE(std::same_as<int &, patricia_set<int>::reference>);
    REQUIRE(std::same_as<int const &, patricia_set<int>::const_reference>);
    REQUIRE(std::same_as<int *, patricia_set<int>::pointer>);
    REQUIRE(std::same_as<int const *, patricia_set<int>::const_pointer>);
    REQUIRE(
        std::same_as<sk::patricia_set_iterator<int, std::allocator<int>, false>,
                     patricia_set<int>::iterator>);
    REQUIRE(
        std::same_as<sk::patricia_set_iterator<int, std::allocator<int>, true>,
                     patricia_set<int>::const_iterator>);
}

TEST_CASE("patricia_trie typedefs")
{
    REQUIRE(std::same_as<int, patricia_trie<int>::value_type>);
    REQUIRE(std::same_as<patricia_key, patricia_trie<int>::key_type>);
    REQUIRE(std::same_as<std::size_t, patricia_trie<int>::size_type>);
    REQUIRE(std::same_as<std::ptrdiff_t, patricia_trie<int>::difference_type>);
    REQUIRE(std::same_as<int &, patricia_trie<int>::reference>);
    REQUIRE(std::same_as<int const &, patricia_trie<int>::const_reference>);
    REQUIRE(std::same_as<int *, patricia_trie<int>::pointer>);
    REQUIRE(std::same_as<int const *, patricia_trie<int>::const_pointer>);
    REQUIRE(
        std::same_as<sk::patricia_iterator<int, std::allocator<int>, false>,
        patricia_trie<int>::iterator>);
    REQUIRE(
        std::same_as<sk::patricia_iterator<int, std::allocator<int>, true>,
        patricia_trie<int>::const_iterator>);
}
