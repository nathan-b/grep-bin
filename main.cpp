#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <climits>
#include <cstring>
#include <ctype.h>
#include <unordered_map>

#include <arpa/inet.h>

using std::string;
using std::list;
using std::vector;
using std::ofstream;
using std::ifstream;
using std::unordered_map;
using std::setw;
using std::setfill;
using std::hex;

bool read_file_to_buffer(const string& filename, vector<uint8_t>& buf)
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
    infile.read((char*)buf.data(), length);

    infile.close();

    return true;
}

void save_file(const string& filename, const vector<uint8_t>& buf)
{
    ofstream outfile(filename, std::ios::out | std::ios::binary);

    if (!outfile) {
        std::cerr << "Could not save file " << filename << "!\n";
        return;
    }

    outfile.write((char*)buf.data(), buf.capacity());
    outfile.close();
}

list<uint32_t> find_all(const uint8_t* buf, uint32_t len, const string& needle)
{
    list<uint32_t> ret;
    const uint32_t needle_len = needle.length();

	if (needle_len > len) {
		return ret;
	}

    for (uint32_t i = 0; i < len - needle_len; ++i) {
        if (memcmp(needle.c_str(), &buf[i], needle_len) == 0) {
            ret.push_back(i);
            i += (needle_len - 1);
        }
    }

    return ret;
}

list<uint32_t> find_all(const uint8_t* buf, uint32_t len, const vector<uint8_t>* needle)
{
	list<uint32_t> ret;
	const uint32_t needle_len = needle->size();

	if (needle_len > len) {
		return ret;
	}

	for (uint32_t i = 0; i < len - needle_len; ++i) {
		if (memcmp(needle->data(), &buf[i], needle_len) == 0) {
			ret.push_back(i);
			i += (needle_len - 1);
		}
	}

	return ret;
}

void usage()
{
	std::cerr << "Usage: gb <string> <filename> \n";
	// Future work: std::cerr << "   or: gb -b <byte#> <byte> [-b ...] <filename>\n";
}

int main(int argc, char** argv)
{
	const uint32_t context_before = 16;
	const uint32_t context_after = 16;
	const char red_on[] = "\x1B[31m";
	const char red_off[] = "\033[0m";
	/*
	 * TODO:
	 *  - Byte buffer as input
	 *  - Find and replace
	 *  - Accept input from pipe
	 *  - Take multiple input files
	 *  - Unify find_all implementations
	 *  - Variable context length
	 */
    if (argc <= 2) {
		usage();
		return -1;
    }

	uint32_t idx = 1;
	// Read the needle string
	const string needle(argv[idx]);
	if (needle.length() == 0) {
		usage();
		return -1;
	}

    // Get the file
	++idx;
    const string savefile(argv[idx]);

	// Read the file
    vector<uint8_t> buf;
    if (!read_file_to_buffer(savefile, buf)) {
        std::cerr << "Could not read file " << savefile << std::endl;
        return -2;
    }

	// Search the file
	list<uint32_t> offsets = find_all(buf.data(), buf.capacity(), needle);

	// Print each output with context
	for (uint32_t offset : offsets) {
		uint32_t len = context_before + context_after + needle.length();
		uint32_t start = offset;
		if (start > context_before) {
			start -= context_before;
		} else {
			start = 0;
			//len -= context_before - start;
		}

		// A line should look like:
		// <offset>:  <context-before><match><context-after>    | ASCII........  |
		std::cout << hex << setw(8) << setfill(' ') << start << ":  ";
		for (uint32_t i = start; i < start + len; ++i) {
			if (i > buf.capacity()) {
				break;
			}
			if (i == offset) {
				std::cout << red_on;
			}
			if (i == offset + needle.length()) {
				std::cout << red_off;
			}
			std::cout << hex << setw(2) << setfill('0') << (int)buf[i] << ' ';
		}

		std::cout << "   | ";

		// Now do it again to print the ASCII representation...
		for (uint32_t i = start; i < start + len; ++i) {
			if (i > buf.capacity()) {
				break;
			}
			if (i == offset) {
				std::cout << red_on;
			}
			if (i == offset + needle.length()) {
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

    return 0;
}
