#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <sys/ioctl.h>

#include "buffer.h"

struct options
{
	std::string search_string;
	std::unique_ptr<buffer> search_bytes;
	std::list<std::string> input_files;
	int16_t context_before;
	int16_t context_after;
};

void save_file(const std::string& filename, const buffer& buf)
{
	std::ofstream outfile(filename, std::ios::out | std::ios::binary);

	if (!outfile) {
		std::cerr << "Could not save file " << filename << "!\n";
		return;
	}

	outfile.write((char*)&buf[0], buf.length());
	outfile.close();
}

void usage()
{
	std::cerr << "Usage: gb [-s] <string> [<filename> <filename> ...] \n"
			  << "   or: gb -b <byte#> <byte> [-b ...] [<filename> <filename> ...]\n"
			  << "   or: gb -be <big-endian value> [<filename> <filename> ...]\n"
			  << "   or: gb -le <little-endian value> [<filename> <filename> ...]\n"
}

bool get_window_dimensions(uint32_t& rows, uint32_t& cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
		return false;
	}
	rows = ws.ws_row;
	cols = ws.ws_col;

	return true;
}

uint32_t get_default_context_len(uint32_t needle_len)
{
	uint32_t rows, cols;
	if (!get_window_dimensions(rows, cols)) {
		return 16;
	}

	// Each byte of context and needle requires 4 characters
	// There's two contexts (before and after) for an additional /2
	// 17 characters of "slush"
	// Extra -1 in there because sometimes the numbers don't divide evenly
	return ((((cols - 17 - (needle_len * 4)) / 4) - 1) / 2);
}

bool get_opts(int argc, char** argv, options& opts)
{
	bool got_needle = false;
	opts.context_before = -1;
	opts.context_after = -1;
	std::vector<uint8_t> needle_bytes;
	std::string needle_string;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			// Read the option
			switch (argv[i][1]) {
			//
			// Get the needle
			//
			case 'b':
				if (argv[i][2] == '\0') { // -b
					if (++i == argc) {
						return false;
					}
					std::stringstream ss(argv[i]);
					uint32_t idx;
					ss >> std::dec >> idx;
					if (++i == argc) {
						return false;
					}
					ss.str(argv[i]);
					ss.clear();
					uint32_t byte;
					ss >> std::hex >> byte;
					if (needle_bytes.size() < (idx + 1)) {
						needle_bytes.resize(idx + 1);
					}
					needle_bytes[idx] = byte;
					got_needle = true;
				} else if (argv[i][2] == 'e' && argv[i][3] == '\0') { // -be
					if (++i == argc) {
						std::cerr << "-be requires an argument\n";
						return false;
					}
					if (got_needle) {
						std::cerr << "Only one search pattern can be specified\n";
						return false;
					}
					opts.search_bytes = buffer_conversion::number_string_to_buffer(argv[i], true, true);
					if (opts.search_bytes) {
						got_needle = true;
					}
				} else {
					std::cerr << "Unrecognized option " << argv[i] << '\n';
					return false;
				}
			break;
			case 'l':
				if (argv[i][2] == 'e' && argv[i][3] == '\0') {
					if (++i == argc) {
						std::cerr << "-be requires an argument\n";
						return false;
					}
					if (got_needle) {
						std::cerr << "Only one search pattern can be specified\n";
						return false;
					}
					opts.search_bytes = buffer_conversion::number_string_to_buffer(argv[i], false, true);
					if (opts.search_bytes) {
						got_needle = true;
					}
				} else {
					std::cerr << "Unrecognized option " << argv[i] << '\n';
					return false;
				}
			break;
			case 's':
				if (argv[i][2] == '\0') {
					if (++i == argc || needle_bytes.size() > 0 || !needle_string.empty()) {
						return false;
					}
					needle_string = argv[i];
				} else {
					std::cerr << "Unrecognized option " << argv[i] << '\n';
					return false;
				}
				got_needle = true;
			break;
			//
			// Display options
			//
			case 'A':
			case 'B':
				if (argv[i][2] == '\0') { // -B, -A
					char c = argv[i][1];
					if (++i == argc) {
						return false;
					}
					std::stringstream ss(argv[i]);
					uint32_t ctx;
					ss >> std::dec >> ctx;

					if (c == 'B') {
						opts.context_before = ctx;
					} else if (c == 'A') {
						opts.context_after = ctx;
					}
				}
			break;
			default:
				std::cerr << "Unrecognized option " << argv[i] << '\n';
				return false;
			}
		} else {
			if (!got_needle) {
				// Assume this is the search string
				if (!needle_bytes.empty()) {
					std::cerr << "Error: Can't specify both a search string and search bytes\n";
					return false;
				}
				needle_string = argv[i];
				got_needle = true;
			} else {
				opts.input_files.emplace_back(argv[i]);
			}
		}
	}

	if (!needle_bytes.empty()) {
		opts.search_bytes = std::make_unique<arraybuf>(needle_bytes);
	} else if (!needle_string.empty()) {
		opts.search_bytes = std::make_unique<strbuf>(needle_string);
	}
	return got_needle;
}

void print_match(const buffer& buf,
	uint32_t offset,
	uint32_t needle_len,
	int16_t context_before,
	int16_t context_after)
{
	const char red_on[] = "\x1B[31m";
	const char red_off[] = "\033[0m";

	int16_t default_context_len = get_default_context_len(needle_len);
	if (context_before < 0) {
		context_before = default_context_len;
	}
	if (context_after < 0) {
		context_after = default_context_len;
	}

	uint32_t len = context_before + context_after + needle_len;
	uint32_t start = offset;
	if (start > (uint16_t)context_before) {
		start -= context_before;
	} else {
		start = 0;
	}

	// A line should look like:
	// <offset>:  <context-before><match><context-after>    | ASCII........  |
	std::cout << std::hex << std::setw(8) << std::setfill(' ') << start << ":  ";
	for (uint32_t i = start; i < start + len; ++i) {
		if (i > buf.length()) {
			break;
		}
		if (i == offset) {
			std::cout << red_on;
		}
		if (i == offset + needle_len) {
			std::cout << red_off;
		}
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buf[i] << ' ';
	}

	std::cout << "   | ";

	// Now do it again to print the ASCII representation...
	for (uint32_t i = start; i < start + len; ++i) {
		if (i > buf.length()) {
			break;
		}
		if (i == offset) {
			std::cout << red_on;
		}
		if (i == offset + needle_len) {
			std::cout << red_off;
		}
		if (isprint(buf[i])) {
			std::cout << std::setw(0) << (char)buf[i];
		} else {
			std::cout << std::setw(0) << '.';
		}
	}

	std::cout << " |" << std::endl;
}

int main(int argc, char** argv)
{
	/*
	 * TODO:
	 *  - Find and replace
	 *  - Accept input from pipe
	 *  - More flexible search terms / options
	 */
	options opts;

	if (!get_opts(argc, argv, opts)) {
		usage();
		return -1;
	}

	// Go through each input file
	for (const std::string& savefile : opts.input_files) {
		if (opts.input_files.size() > 1) {
			std::cout << savefile << ':' << std::endl;
		}
		// Read the file
		arraybuf buf(savefile);
		if (buf.length() == 0) {
			std::cerr << "Could not read file " << savefile << std::endl;
			return -2;
		}

		// Search the file
		uint32_t needle_len = opts.search_bytes->length();
		if (needle_len == 0) {
			std::cerr << "Null search string\n";
			return -3;
		}
		std::list<uint32_t> offsets = buf.find_all(*opts.search_bytes);

		// Print each output with context
		for (uint32_t offset : offsets) {
			print_match(buf, offset, needle_len, opts.context_before, opts.context_after);
		}
	}

	return 0;
}
