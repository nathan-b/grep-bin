#include "buffer.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <gtest/gtest.h>

uint8_t char2num(char c)
{
	switch (c) {
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
		return 0xa;
	case 'b':
		return 0xb;
	case 'c':
		return 0xc;
	case 'd':
		return 0xd;
	case 'e':
		return 0xe;
	case 'f':
		return 0xf;
	}
	return 0xff;
}

buffer* num2buf(const std::string& numstr, bool big_endian, bool hex)
{
	if (!big_endian) {
		// Only be for now
		return nullptr;
	}

	if (!hex) {
		// Only hex for now
		return nullptr;
	}

	enum {
		LOW,
		HIGH
	} pos = LOW;

	std::list<uint8_t> num_list;
	uint8_t val;

	for (auto it = numstr.rbegin(); it != numstr.rend(); ++it) {
		if (pos == LOW) {
			val = char2num(*it);
			pos = HIGH;
		} else {
			val += char2num(*it) * 0x10;
			num_list.push_front(val);
			pos = LOW;
		}
	}
	if (pos == HIGH) {
		num_list.push_front(val);
	}

	// We now have a list of bytes in big-endian order
	arraybuf* ret = new arraybuf(nullptr, num_list.size());
	uint32_t idx = 0;
	for (uint8_t i : num_list) {
		(*ret)[idx++] = i;
	}

	return ret;
}

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
		ASSERT_EQ((uint32_t)4, res.size());
		ASSERT_EQ((uint32_t)6, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)14, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)19, res.front());
		res.pop_front();
		ASSERT_EQ((uint32_t)21, res.front());
	}
}

TEST(buffer, num2buf_tests)
{
	buffer* buf = nullptr;

	buf = num2buf("a", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa, (*buf)[0]);
	delete buf;

	buf = num2buf("a4", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)1, buf->length());
	ASSERT_EQ((uint8_t)0xa4, (*buf)[0]);
	delete buf;

	buf = num2buf("a4f", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0x4f, (*buf)[0]);
	ASSERT_EQ((uint8_t)0xa, (*buf)[1]);
	delete buf;

	buf = num2buf("a4f0", true, true);
	ASSERT_NE(nullptr, buf);
	ASSERT_EQ((uint32_t)2, buf->length());
	ASSERT_EQ((uint8_t)0xf0, (*buf)[0]);
	ASSERT_EQ((uint8_t)0xa4, (*buf)[1]);
	delete buf;
}

int main(int argc, char** argv)
{
	setup();
	::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
