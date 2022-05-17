#include "buffer.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#define ASSERT_EQ(_invariant, _var)                                                                \
	if (!((_invariant) == (_var))) {                                                               \
		std::cerr << "Test " << __FUNCTION__ << " failed at line " << __LINE__ << ": " << (int)_var << " found but " << _invariant << " expected\n"; \
		return false;                                                                              \
	}

#define ASSERT_TRUE(_cond)                                 \
	if (!(_cond)) {                                        \
		std::cerr << "Test " << __FUNCTION__ << " failed at line " << __LINE__ << ": " #_cond " is false\n"; \
		return false;                                      \
	}

#define ASSERT_FALSE(_cond)                               \
	if (_cond) {                                          \
		std::cerr << "Test " << __FUNCTION__ << " failed at line " << __LINE__ << ": " #_cond " is true\n"; \
		return false;                                     \
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

bool arraybuf_test()
{
	arraybuf ab(test_buf, tb_size);

	ASSERT_EQ(tb_size, ab.length());
	uint32_t iter = 0;
	for (uint8_t val : ab) {
		ASSERT_EQ(iter, val);
		ASSERT_EQ(iter, ab[iter]);
		++iter;
	}
	return true;
}

bool strbuf_test()
{
	strbuf sb(test_str);

	ASSERT_EQ(tb_size, sb.length());

	uint32_t iter = 0;
	for (uint8_t val : sb) {
		ASSERT_EQ(seed_chars[iter % seed_len], val);
		ASSERT_EQ(seed_chars[iter % seed_len], sb[iter]);
		++iter;
	}
	return true;
}

bool cmp_test()
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

	return true;
}

bool find_test()
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
		ASSERT_EQ(1, res.size());
		ASSERT_EQ(10, res.front());
	}

	{
		std::string item("dorf");
		strbuf needle(item);
		auto res = ab.find_all(needle);
		ASSERT_EQ(0, res.size());
	}

	{
		uint8_t ndata[] = { 0x00, 0x00 };
		arraybuf needle(ndata, 2);
		auto res = ab.find_all(needle);
		ASSERT_EQ(4, res.size());
		ASSERT_EQ(6, res.front());
		res.pop_front();
		ASSERT_EQ(14, res.front());
		res.pop_front();
		ASSERT_EQ(19, res.front());
		res.pop_front();
		ASSERT_EQ(21, res.front());
	}

	return true;
}

int main(int argc, char** argv)
{
	setup();
	if (!arraybuf_test()) {
		return -1;
	}

	if (!strbuf_test()) {
		return -1;
	}

	if (!cmp_test()) {
		return -1;
	}

	if (!find_test()) {
		return -1;
	}

	std::cout << "All passed!\n";
	return 0;
}
