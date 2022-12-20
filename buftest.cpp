#include "buffer.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <gtest/gtest.h>

const char* seed_chars = "abcdefghijklmnopqrstuvwxyz";
const uint32_t seed_len = 26;
const uint32_t tb_size = 256;
uint8_t test_buf[tb_size];
std::string test_str;

void setup()
{
	for (uint32_t i = 0; i < tb_size; ++i) {
		test_buf[i] = i;
		test_str.push_back(seed_chars[i % seed_len]);
	}
}

TEST(buffer, arraybuf_test)
{
	arraybuf ab(test_buf, tb_size);

	ASSERT_EQ(tb_size, ab.length());
	uint32_t iter = 0;
	for (uint8_t val : ab) {
		ASSERT_EQ(iter, val);
		ASSERT_EQ(iter, ab[iter]);
		++iter;
	}
}

TEST(buffer, strbuf_test)
{
	strbuf sb(test_str);

	ASSERT_EQ(tb_size, sb.length());

	uint32_t iter = 0;
	for (uint8_t val : sb) {
		ASSERT_EQ(seed_chars[iter % seed_len], val);
		ASSERT_EQ(seed_chars[iter % seed_len], sb[iter]);
		++iter;
	}
}

TEST(buffer, cmp_test)
{
	uint8_t needle_data[4] = { 5, 6, 7, 8 };
	uint8_t needle_start[4] = { 0, 1, 2, 3 };
	uint8_t needle_end[4] = { 252, 253, 254, 255 };
	arraybuf ab(test_buf, tb_size);

	{
		arraybuf needle(needle_data, 4);
		ASSERT_FALSE(ab.cmp(needle));
		ASSERT_TRUE(ab.cmp(needle, 5));
		ASSERT_FALSE(ab.cmp(needle, 1000));
		ASSERT_FALSE(ab.cmp(needle, 253));
	}

	{
		arraybuf needle(needle_start, 4);
		ASSERT_FALSE(ab.cmp(needle)); // Length mismatch
		ASSERT_TRUE(ab.cmp(needle, 0));
		ASSERT_FALSE(ab.cmp(needle, 1));
	}

	{
		arraybuf needle(needle_end, 4);
		ASSERT_FALSE(ab.cmp(needle));
		ASSERT_TRUE(ab.cmp(needle, 252));
	}
}

TEST(buffer, find_test)
{
	uint8_t corpus[] = { 0x6f, 0x00, 0x1e, 0xef, 0x2b, 0x94, 0x00, 0x00,
	                     0x00, 0x04, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x00,
						 0x07, 0x2b, 0x95, 0x00, 0x00, 0x00, 0x00, 0x49,
						 0x6c, 0x6c, 0x69, 0x73, 0x61, 0x20, 0x4b, 0x65,
						 0x70, 0x70, 0x65, 0x49, 0x61, 0x00, 0x01, 0x9f };

	arraybuf ab(corpus, sizeof(corpus));

	{
		std::string item("list");
		strbuf needle(item);
		auto res = ab.find_all(needle);
		ASSERT_EQ((uint64_t)1, res.size());
		ASSERT_EQ((uint32_t)10, res.front());
	}

	{
		std::string item("dorf");
		strbuf needle(item);
		auto res = ab.find_all(needle);
		ASSERT_EQ((uint32_t)0, res.size());
	}

	{
		uint8_t ndata[] = { 0x00, 0x00 };
		arraybuf needle(ndata, 2);
		auto res = ab.find_all(needle);
		ASSERT_EQ((uint32_t)6, res.size());
		ASSERT_EQ((uint32_t)6, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)7, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)14, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)19, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)20, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)21, res.front());
		res.pop_front();
	}

	{
		arraybuf needle({0x00, 0x01, 0x9f});
		auto res = ab.find_all(needle);
		ASSERT_EQ((uint32_t)1, res.size());
		ASSERT_EQ((uint32_t)37, res.front());
	}

	{
		arraybuf needle({0x73});
		auto res = ab.find_all(needle);
		ASSERT_EQ((uint32_t)2, res.size());
		uint32_t val = res.front();
		ASSERT_EQ((uint32_t)12, val);
		res.pop_front();
		val = res.front();
		ASSERT_EQ((uint32_t)27, val);
	}

	{
		auto res = ab.find_all(ab);
		ASSERT_EQ((uint32_t)1, res.size());
		ASSERT_EQ((uint32_t)0, res.front());
	}
}

TEST(buffer, find_test_targeted)
{
	const uint32_t len = 256;
	const char* seed_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::vector<uint8_t> vec(len);

	for (uint32_t i = 0; i < len; ++i) {
		vec[i] = seed_chars[i % 52];
	}

	{
		arraybuf ab(vec);
		uint32_t result = ab.find_first(arraybuf{'Z', 'a', 'b'});

		ASSERT_EQ((uint32_t)25, result);
	}

	for (uint32_t i = 0; i < len; ++i) {
		vec[i] = 'a';
	}

	{
		arraybuf ab(vec);
		ASSERT_EQ(len, ab.length());
		ASSERT_EQ((uint8_t)'a', ab[0]);

		auto result = ab.find_all(arraybuf{'a'});
		ASSERT_EQ(len, result.size());
	}
}

TEST(buffer, num2buf_be_tests)
{
	std::unique_ptr<buffer> buf = nullptr;

	buf = buffer_conversion::number_string_to_buffer("A", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("a4", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa4, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("a4F", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0xa, (*buf)[0]);
	ASSERT_EQ((uint8_t)0x4f, (*buf)[1]);

	buf = buffer_conversion::number_string_to_buffer("a4f0", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0xa4, (*buf)[0]);
	ASSERT_EQ((uint8_t)0xf0, (*buf)[1]);

	buf = buffer_conversion::number_string_to_buffer("a4f05aff4d0e110a9", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)9, buf->length());
	ASSERT_EQ((uint8_t)0x0a, (*buf)[0]);
	ASSERT_EQ((uint8_t)0x4f, (*buf)[1]);
	ASSERT_EQ((uint8_t)0x05, (*buf)[2]);
	ASSERT_EQ((uint8_t)0xaf, (*buf)[3]);
	ASSERT_EQ((uint8_t)0xf4, (*buf)[4]);
	ASSERT_EQ((uint8_t)0xd0, (*buf)[5]);
	ASSERT_EQ((uint8_t)0xe1, (*buf)[6]);
	ASSERT_EQ((uint8_t)0x10, (*buf)[7]);
	ASSERT_EQ((uint8_t)0xa9, (*buf)[8]);
}

TEST(buffer, num2buf_le_tests)
{
	std::unique_ptr<buffer> buf = nullptr;

	buf = buffer_conversion::number_string_to_buffer("a", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("a4", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa4, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("a4f", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0x0a, (*buf)[1]);
	ASSERT_EQ((uint8_t)0x4f, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("4F0a", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0x0a, (*buf)[0]);
	ASSERT_EQ((uint8_t)0x4f, (*buf)[1]);

	buf = buffer_conversion::number_string_to_buffer("a4f0", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0xa4, (*buf)[1]);
	ASSERT_EQ((uint8_t)0xf0, (*buf)[0]);

	buf = buffer_conversion::number_string_to_buffer("a4f05aff4d0E110a9", false, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)9, buf->length());
	ASSERT_EQ((uint8_t)0xa9, (*buf)[0]);
	ASSERT_EQ((uint8_t)0x10, (*buf)[1]);
	ASSERT_EQ((uint8_t)0xe1, (*buf)[2]);
	ASSERT_EQ((uint8_t)0xd0, (*buf)[3]);
	ASSERT_EQ((uint8_t)0xf4, (*buf)[4]);
	ASSERT_EQ((uint8_t)0xaf, (*buf)[5]);
	ASSERT_EQ((uint8_t)0x05, (*buf)[6]);
	ASSERT_EQ((uint8_t)0x4f, (*buf)[7]);
	ASSERT_EQ((uint8_t)0x0a, (*buf)[8]);
}

TEST(buffer_needle, match_first)
{
	uint8_t corpus[] = { 0x6f, 0x00, 0x1e, 0xef, 0x2b, 0x94, 0x00, 0x00,
	                     0x00, 0x04, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x00,
						 0x07, 0x2b, 0x95, 0x00, 0x00, 0x00, 0x00, 0x49,
						 0x6c, 0x6c, 0x69, 0x73, 0x61, 0x20, 0x4b, 0x65,
						 0x70, 0x70, 0x65, 0x49, 0x61, 0x00, 0x01, 0x9f };

	arraybuf ab(corpus, sizeof(corpus));

	{
		buffer_needle bn({0x00, 0x01, 0x02});
		uint32_t offset = bn.first_match(ab, 0);
		ASSERT_EQ((uint32_t)UINT32_MAX, offset);
	}

	{
		buffer_needle bn({0x00, 0x01, 0x9f});
		uint32_t offset = bn.first_match(ab, 0);
		ASSERT_EQ((uint32_t)37, offset);

		offset = bn.first_match(ab, 38);
		ASSERT_EQ((uint32_t)UINT32_MAX, offset);
	}

	{
		buffer_needle bn({0x00, 0x00});
		uint32_t offset = bn.first_match(ab, 0);
		ASSERT_EQ((uint32_t)6, offset);

		offset = bn.first_match(ab, 6);
		ASSERT_EQ((uint32_t)6, offset);

		offset = bn.first_match(ab, 7);
		ASSERT_EQ((uint32_t)7, offset);

		offset = bn.first_match(ab, 8);
		ASSERT_EQ((uint32_t)14, offset);
	}
}

TEST(buffer_needle, match_all)
{
	uint8_t corpus[] = { 0x6f, 0x00, 0x1e, 0xef, 0x2b, 0x94, 0x00, 0x00,
	                     0x00, 0x04, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x00,
						 0x07, 0x2b, 0x95, 0x00, 0x00, 0x00, 0x00, 0x49,
						 0x6c, 0x6c, 0x69, 0x73, 0x61, 0x20, 0x4b, 0x65,
						 0x70, 0x70, 0x65, 0x49, 0x61, 0x00, 0x01, 0x9f };

	arraybuf ab(corpus, sizeof(corpus));

	{
		buffer_needle bn({0x00, 0x01, 0x02});
		auto offlist = bn.match(ab, 0);
		ASSERT_EQ((uint32_t)0, offlist.size());
	}

	{
		buffer_needle bn({0x00, 0x01, 0x9f});
		auto offlist = bn.match(ab, 0);
		ASSERT_EQ((uint32_t)1, offlist.size());
		ASSERT_EQ((uint32_t)37, offlist.front());
	}

	{
		buffer_needle bn({0x00, 0x00});
		auto offlist = bn.match(ab, 0);

		ASSERT_EQ((uint32_t)6, offlist.size());
		ASSERT_EQ((uint32_t)6, offlist.front());
		offlist.pop_front();
		ASSERT_EQ((uint32_t)7, offlist.front());
		offlist.pop_front();
		ASSERT_EQ((uint32_t)14, offlist.front());
		offlist.pop_front();
		ASSERT_EQ((uint32_t)19, offlist.front());
		offlist.pop_front();
		ASSERT_EQ((uint32_t)20, offlist.front());
		offlist.pop_front();
		ASSERT_EQ((uint32_t)21, offlist.front());
		offlist.pop_front();
	}
}

TEST(wcl_needle, byte_match)
{
	uint8_t corpus[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
						 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
						 0x98, 0xa4, 0xbc, 0xc7, 0xd9, 0xef, 0xf1, 0xff };
	uint16_t _ = 0;
	wildcard_const_len wcl(&_, 1);

	// Exact match
	ASSERT_TRUE(wcl.byte_match(0x00, 0xff00));
	ASSERT_FALSE(wcl.byte_match(0x00, 0xff01));
	ASSERT_TRUE(wcl.byte_match(0x07, 0xff07));
	ASSERT_FALSE(wcl.byte_match(0x07, 0xff06));
	ASSERT_TRUE(wcl.byte_match(0xff, 0xffff));
	ASSERT_FALSE(wcl.byte_match(0xff, 0xfffe));

	// Complete wildcard
	for (uint32_t i = 0; i < 8; ++i) {
		ASSERT_TRUE(wcl.byte_match(corpus[i], 0x0000));
		ASSERT_TRUE(wcl.byte_match(corpus[i], 0x00ff));
	}

	uint8_t mask = ~0x7;
	for (uint32_t i = 0; i < 8; ++i) {
		ASSERT_TRUE(wcl.byte_match(corpus[i], (mask << 8) | 0x00));
		ASSERT_TRUE(wcl.byte_match(corpus[i], (mask << 8) | 0x07));
	}

	mask = ~0xf;
	for (uint32_t i = 8; i < 16; ++i) {
		ASSERT_TRUE(wcl.byte_match(corpus[i], (mask << 8) | 0x00));
		ASSERT_TRUE(wcl.byte_match(corpus[i], (mask << 8) | 0x07));
		ASSERT_TRUE(wcl.byte_match(corpus[i], (mask << 8) | 0x0f));
	}

	mask = 0xf;
	ASSERT_TRUE(wcl.byte_match(0x98, (mask << 8) | 0x08));
	ASSERT_TRUE(wcl.byte_match(0x98, (mask << 8) | 0x58));
	ASSERT_FALSE(wcl.byte_match(0x98, (mask << 8) | 0x09));
	ASSERT_TRUE(wcl.byte_match(0x97, (mask << 8) | 0x07));
	ASSERT_TRUE(wcl.byte_match(0x97, (mask << 8) | 0xf7));
	ASSERT_FALSE(wcl.byte_match(0x97, (mask << 8) | 0x08));
}

TEST(wcl_needle, match)
{
	uint8_t corpus[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	                     0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
						 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
						 0x98, 0xa4, 0xbc, 0xc7, 0xd9, 0xef, 0xf1, 0xff,
						 0x6f, 0x00, 0x1e, 0xef, 0x2b, 0x94, 0x00, 0x00,
	                     0x00, 0x04, 0x6c, 0x69, 0x73, 0x74, 0x00, 0x00,
						 0x07, 0x2b, 0x95, 0x00, 0x00, 0x00, 0x00, 0x49,
						 0x6c, 0x6c, 0x69, 0x73, 0x61, 0x20, 0x4b, 0x65,
						 0x70, 0x70, 0x65, 0x49, 0x61, 0x00, 0x01, 0x9f };

	arraybuf ab(corpus, sizeof(corpus));

	// Exact match
	{
		wildcard_const_len wcl_exact({0xff2b, 0xff95});

		ASSERT_EQ((uint32_t)49, wcl_exact.first_match(ab));
	}
	{
		wildcard_const_len wcl_exact({0xff70});
		uint32_t expected[] = {22, 64, 65};

		auto rlist = wcl_exact.match(ab);

		ASSERT_EQ((uint32_t)3, rlist.size());

		for (uint32_t i = 0; i < 3; ++i) {
			uint32_t idx = rlist.front();
			rlist.pop_front();
			ASSERT_EQ(expected[i], idx);
		}
	}

	// Bitmask match
	{
		wildcard_const_len wcl_mask({0x41ff, 0xff61});

		ASSERT_EQ((uint32_t)59, wcl_mask.first_match(ab));
		ASSERT_EQ((uint32_t)67, wcl_mask.first_match(ab, 60));
	}

	// Full wildcard
	// Find all numbers between two odd numbers
	{
		wildcard_const_len wcl_wildcard({0x0101, 0x0000, 0x0101});

		auto rlist = wcl_wildcard.match(ab);

		ASSERT_EQ((uint32_t)16, rlist.size());

		for (auto i : rlist) {
			ASSERT_LE(i + 3, ab.length());
			ASSERT_EQ(0x1, ab[i] & 0x1);
			ASSERT_EQ(0x1, ab[i + 2] & 0x1);
		}
	}
}

int main(int argc, char** argv)
{
	setup();
	::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
