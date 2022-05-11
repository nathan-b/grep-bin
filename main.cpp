#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>

#include "buffer.h"

using std::dec;
using std::hex;
using std::ifstream;
using std::list;
using std::ofstream;
using std::setfill;
using std::setw;
using std::string;
using std::unordered_map;
using std::vector;

struct options
{
	string search_string;
	vector<uint8_t> search_bytes;
	list<string> input_files;
};

bool read_file_to_buffer(const string& filename, arraybuf& buf)
{
	uint32_t length;
	ifstream infile(filename, std::ios::in | std::ios::binary);

	if (!infile) {
		return false;
	}

	// Get the file length
	infile.seekg(0, std::ios::end);
	length = infile.tellg();

	// Allocate the buffer
	buf.reserve(length);

	// Read the file
	infile.seekg(0, std::ios::beg);
	infile.read((char*)&buf[0], length);

	infile.close();

	return true;
}

void save_file(const string& filename, const buffer& buf)
{
	ofstream outfile(filename, std::ios::out | std::ios::binary);

	if (!outfile) {
		std::cerr << "Could not save file " << filename << "!\n";
		return;
	}

	outfile.write((char*)&buf[0], buf.length());
	outfile.close();
}

void usage()
{
	std::cerr << "Usage: gb <string> <filename> [<filename> ...] \n"
			  << "   or: gb <byte#> <byte> [-b ...] <filename> [<filename> ...]\n";
}

bool get_opts(int argc, char** argv, options& opts)
{
	bool got_needle = false, got_haystack = false;

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			// Read the option
			switch (argv[i][1]) {
			case 'b': {
				if (++i == argc) {
					return false;
				}
				std::stringstream ss(argv[i]);
				ss << dec;
				uint32_t idx;
				ss >> idx;
				if (++i == argc) {
					return false;
				}
				ss.str(argv[i]);
				ss.clear();
				uint32_t byte;
				ss << hex;
				ss >> byte;
				if (opts.search_bytes.size() < (idx + 1)) {
					opts.search_bytes.resize(idx + 1);
				}
				opts.search_bytes[idx] = byte;
				got_needle = true;
			}
				break;
            default:
                std::cerr << "Unrecognized option " << argv[i] << '\n';
                return false;
			}
		} else {
			if (!got_needle) {
				// Assume this is the search string
				if (!opts.search_bytes.empty()) {
					std::cerr << "Error: Can't specify both a search string and search bytes\n";
					return false;
				}
				opts.search_string = argv[i];
				got_needle = true;
			} else {
				got_haystack = true;
				opts.input_files.emplace_back(argv[i]);
			}
		}
	}
	return got_needle && got_haystack;
}

int main(int argc, char** argv)
{
	const uint32_t context_before = 16;
	const uint32_t context_after = 16;
	const char red_on[] = "\x1B[31m";
	const char red_off[] = "\033[0m";
	/*
	 * TODO:
	 *  - Find and replace
	 *  - Accept input from pipe
	 *  - Variable context length
	 */
	options opts;

	if (!get_opts(argc, argv, opts)) {
		usage();
		return -1;
	}

	// Search each input file
	for (const string& savefile : opts.input_files) {
		if (opts.input_files.size() > 1) {
			std::cout << savefile << ':' << std::endl;
		}
		// Read the file
		arraybuf buf;
		if (!read_file_to_buffer(savefile, buf)) {
			std::cerr << "Could not read file " << savefile << std::endl;
			return -2;
		}

		// Search the file
		list<uint32_t> offsets;
		uint32_t needle_len = 0;
		if (!opts.search_string.empty()) {
			offsets = buf.find_all(strbuf(opts.search_string));
			needle_len = opts.search_string.length();
		} else {
			offsets = buf.find_all(arraybuf(opts.search_bytes.data(), opts.search_bytes.size()));
			needle_len = opts.search_bytes.size();
		}
		if (needle_len == 0) {
			std::cerr << "Null search string\n";
			return -3;
		}

		// Print each output with context
		for (uint32_t offset : offsets) {
			uint32_t len = context_before + context_after + needle_len;
			uint32_t start = offset;
			if (start > context_before) {
				start -= context_before;
			} else {
				start = 0;
			}

			// A line should look like:
			// <offset>:  <context-before><match><context-after>    | ASCII........  |
			std::cout << hex << setw(8) << setfill(' ') << start << ":  ";
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
				std::cout << hex << setw(2) << setfill('0') << (int)buf[i] << ' ';
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
					std::cout << setw(0) << (char)buf[i];
				} else {
					std::cout << setw(0) << '.';
				}
			}

			std::cout << " |" << std::endl;
		}
	}

	return 0;
}
