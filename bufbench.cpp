#include <benchmark/benchmark.h>

#include "buffer.h"

#include <stdint.h>
#include <vector>

uint8_t* get_buf(uint32_t len)
{
	const char* seed_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	uint8_t* buf = new uint8_t[len];

	for (uint32_t i = 0; i < len; ++i) {
		buf[i] = seed_chars[i % 52];
	}

	return buf;
}

std::vector<uint8_t> get_vec(uint32_t len)
{
	const char* seed_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::vector<uint8_t> vec(len);

	for (uint32_t i = 0; i < len; ++i) {
		vec[i] = seed_chars[i % 52];
	}

	return vec;
}

static void bm_create_arraybuf_from_byte_buffer(benchmark::State& state)
{
	const uint32_t len = 65536;
	uint8_t* test_buf = get_buf(len);

	for (auto _ : state) {
		arraybuf ab(nullptr, len);
		memcpy(&ab[0], test_buf, len);
	}

	delete[] test_buf;
}
// Register the function as a benchmark
BENCHMARK(bm_create_arraybuf_from_byte_buffer);

static void bm_create_arraybuf_from_vector(benchmark::State& state)
{
	const uint32_t len = 65536;
	std::vector<uint8_t> vec = get_vec(len);

	for (auto _ : state) {
		arraybuf ab(vec);
	}
}
BENCHMARK(bm_create_arraybuf_from_vector);

static void bm_find_first_easy(benchmark::State& state)
{
	const uint32_t len = 256;
	std::vector<uint8_t> vec = get_vec(len);
	arraybuf ab(vec);
	uint32_t result = 0;

	for (auto _ : state) {
		result = ab.find_first(arraybuf{'Z', 'a', 'b'});
		if (result != 25) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_first_easy);

static void bm_find_first_hard(benchmark::State& state)
{
	const uint32_t len = state.range(0);
	std::vector<uint8_t> vec = get_vec(len);
	vec[len - 5] = '1';
	vec[len - 4] = '2';
	vec[len - 3] = '3';
	arraybuf ab(vec);
	uint32_t result = 0;

	for (auto _ : state) {
		result = ab.find_first(arraybuf{'1', '2', '3'});
		if (result != len - 5) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_first_hard)->Arg(65536)->Arg(67108864)->Arg(1073741824);

static void bm_find_first_needle(benchmark::State& state)
{
	const uint32_t len = state.range(0);
	std::vector<uint8_t> vec = get_vec(len);
	vec[len - 5] = '1';
	vec[len - 4] = '2';
	vec[len - 3] = '3';
	arraybuf ab(vec);
	uint32_t result = 0;
	buffer_needle bn({'1', '2', '3'});

	for (auto _ : state) {
		result = bn.first_match(ab);
		if (result != len - 5) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_first_needle)->Arg(65536)->Arg(67108864)->Arg(1073741824);

static void bm_find_all(benchmark::State& state)
{
	const uint32_t len = state.range(0);
	std::vector<uint8_t> vec = get_vec(len);
	arraybuf ab(vec);
	std::list<uint32_t> result;
	uint32_t expected_length = len / 52;

	for (auto _ : state) {
		result = ab.find_all(arraybuf{'Z', 'a', 'b'});
		if (result.size() != expected_length) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_all)->Arg(65536)->Arg(67108864)->Arg(1073741824);

static void bm_find_all_needle_short(benchmark::State& state)
{
	const uint32_t len = state.range(0);
	std::vector<uint8_t> vec = get_vec(len);
	arraybuf ab(vec);
	std::list<uint32_t> result;
	buffer_needle bn({'Z', 'a', 'b'});
	uint32_t expected_length = len / 52;

	for (auto _ : state) {
		result = bn.match(ab);
		if (result.size() != expected_length) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_all_needle_short)->Arg(65536)->Arg(67108864)->Arg(1073741824);

static void bm_find_all_needle_long(benchmark::State& state)
{
	const uint32_t len = state.range(0);
	std::vector<uint8_t> vec = get_vec(len);
	arraybuf ab(vec);
	std::list<uint32_t> result;
	buffer_needle bn({'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
	                  'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'});
	uint32_t expected_length = len / 52;

	for (auto _ : state) {
		result = bn.match(ab);
		if (result.size() != expected_length) {
			state.SkipWithError("Could not match string");
			break;
		}
	}
}
BENCHMARK(bm_find_all_needle_long)->Arg(65536)->Arg(67108864)->Arg(1073741824);

BENCHMARK_MAIN();
