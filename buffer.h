#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <vector>

/**
 * Class defining a buffer interface.
 */
class buffer
{
public:
	/**
	 * A fairly simple forward iterator for the buffer.
	 */
	struct iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = std::ptrdiff_t;
		using value_type        = uint8_t;
		using reference         = value_type&;
		using pointer           = value_type*;

		iterator(pointer p): m_pointer(p) {}

		reference operator*() { return *m_pointer; }
		pointer operator->() { return m_pointer; }
    	iterator& operator++() { ++m_pointer; return *this; }
    	iterator operator++(int) {
			iterator it(m_pointer);
			++m_pointer;
			return it;
		}

    	bool operator==(const iterator& rhs) const {
			return m_pointer == rhs.m_pointer;
		};
    	bool operator!=(const iterator& rhs) const {
			return m_pointer != rhs.m_pointer;
		};

	private:
		pointer m_pointer;
	};

	virtual ~buffer() = default;

	virtual uint32_t length() const = 0;

	virtual iterator begin() = 0;
	virtual iterator end() = 0;
	virtual uint8_t& operator[](uint32_t idx) = 0;
	virtual const uint8_t& operator[](uint32_t idx) const = 0;

	/**
	 * Compares this buffer to another.
	 *
	 * @return true if the buffers are identical; false otherwise.
	 */
	virtual bool cmp(const buffer& other) const
	{
		if (length() != other.length()) return false;
		return cmp(other, 0);
	}

	/**
	 * Compares a fraction of this buffer to another.
	 *
	 * Returns true if the slice starting at @start and going @other.length bytes
	 * is equal to other. False otherwise.
	 */
	virtual bool cmp(const buffer& other, uint32_t start) const
	{
		if (other.length() > (length() - start)) {
			return false;
		}
		return memcmp(&other[0], &(*this)[start], other.length()) == 0;
	}

	/**
	 * Finds the first occurence of @needle in this buffer.
	 *
	 * Returns the offset, or UINT32_MAX if not found.
	 */
	virtual uint32_t find_first(const buffer& needle,
	                            uint32_t start_at = 0) const
	{
		const uint32_t len = length();
		const uint32_t needle_len = needle.length();

		if (needle_len > len) {
			return UINT32_MAX;
		}

		uint32_t upto = len - needle_len;
		uint8_t needle_start = needle[0];
		for (uint32_t i = start_at; i <= upto; ++i) {
			// Optimization: compare the first bytes to see if we should even
			// bother calling cmp at all
			if (needle_start != (*this)[i]) continue;
			if (cmp(needle, i)) {
				return i;
			}
		}
		return UINT32_MAX;
	}

	/**
	 * Find all instances of @needle in this buffer.
	 *
	 * Returns a list of buffer offsets where the needle is found.
	 */
	virtual std::list<uint32_t> find_all(const buffer& needle,
	                                     uint32_t start_at = 0) const
	{
		std::list<uint32_t> ret;
		const uint32_t len = length();
		const uint32_t needle_len = needle.length();

		if (needle_len > len) {
			return ret;
		}

		uint32_t upto = len - needle_len;
		uint8_t needle_start = needle[0];
		for (uint32_t i = start_at; i <= upto; ++i) {
			// Optimization: compare the first bytes to see if we should even
			// bother calling cmp at all
			if (needle_start != (*this)[i]) continue;
			if (cmp(needle, i)) {
				ret.push_back(i);
			}
		}

		return ret;
	}


	/*************************************************************************
	 * Read various types from the buffer
	 *   Bounds checking not guaranteed.
	 */
	int16_t read_short(uint32_t offset) const
	{
		return *(int16_t*)(&(*this)[offset]);
	}

	uint16_t read_ushort(uint32_t offset) const
	{
		return *(uint16_t*)(&(*this)[offset]);
	}

	int32_t read_int(uint32_t offset) const
	{
		return *(int32_t*)(&(*this)[offset]);
	}

	uint32_t read_uint(uint32_t offset) const
	{
		return *(uint32_t*)(&(*this)[offset]);
	}

	int64_t read_long(uint32_t offset) const
	{
		return *(int64_t*)(&(*this)[offset]);
	}

	uint64_t read_ulong(uint32_t offset) const
	{
		return *(uint64_t*)(&(*this)[offset]);
	}
};

/**
 * Buffer backed by a byte array.
 *
 * Pass in nullptr as the buffer and it will alloc one itself.
 */
class arraybuf : public buffer
{
public:
	/**
	 * Create an empty arraybuf.
	 *
	 * Use the reserve API to set a size for the buffer.
	 */
	arraybuf() :
		m_buf(nullptr),
		m_len(0),
		m_free(false)
	{}

	/**
	 * Create an arraybuf from an existing buffer.
	 *
	 * If the input buffer is non-null, the arraybuf will wrap it but will not
	 * take ownerwhip -- freeing the buffer is the responsibility of the caller.
	 *
	 * If the input buffer is null and length is nonzero, the constructor will
	 * create a buffer of the given length. The arraybuf will own this buffer and
	 * will free it internally when done with it.
	 *
	 * If the input buffer is null and length is zero, this constructor is
	 * equivalent to the empty constructor.
	 */
	arraybuf(uint8_t* arr, uint32_t length) :
		m_buf(arr),
		m_len(length),
		m_free(false)
	{
		if (!arr) {
			if (length > 0) {
				m_buf = new uint8_t[length];
				m_free = true;
			} else {
				m_buf = nullptr;
			}
		}
	}

	/**
	 * Create an arraybuf from a vector of bytes.
	 *
	 * Copies the vector into the internal buffer.
	 */
	arraybuf(const std::vector<uint8_t>& vec)
	{
		m_len = vec.size();
		m_free = true;
		m_buf = new uint8_t[m_len];

		memcpy(m_buf, vec.data(), m_len);
	}

	/**
	 * Create an arraybuf given an initializer list.
	 *
	 * Allows the programmatic construction of arraybuf "literals".
	 */
	arraybuf(std::initializer_list<uint8_t>&& in_list)
	{
		m_len = in_list.size();
		m_buf = new uint8_t[m_len];
		m_free = true;
		uint8_t* pos = m_buf;

		for (uint8_t val : in_list) {
			*pos++ = val;
		}
	}

	/**
	 * Create a buffer from the contents of a file
	 */
	arraybuf(const std::filesystem::path& file_path) :
		m_buf(nullptr),
		m_len(0),
		m_free(false)
	{
		std::ifstream infile(file_path, std::ios::in | std::ios::binary);

		if (!infile) {
			return;
		}

		// Get the file length
		infile.seekg(0, std::ios::end);
		m_len = infile.tellg();

		// Allocate the buffer
		m_buf = new uint8_t[m_len];
		m_free = true;

		// Read the file
		infile.seekg(0, std::ios::beg);
		infile.read((char*)m_buf, m_len);
		infile.close();
	}

	/**
	 * Create and populate a buffer from a stream.
	 *
	 * This constructor will read from the stream until EOF / error. So
	 * make sure any stream you give it will eventually finish.
	 */
	arraybuf(std::istream& stream) :
		m_buf(nullptr),
		m_len(0),
		m_free(false)
	{
		const uint32_t buf_len = 65536;
		std::list<uint8_t*> buf_list;
		uint8_t* buf = nullptr;
		uint32_t total_len = 0;

		// Read the stream until EOF
		while (stream && total_len < UINT32_MAX) {
			buf = new uint8_t[buf_len];
			stream.read((char*)buf, buf_len);
			if (stream.gcount() > UINT32_MAX - total_len) {
				// Buffer too large
				total_len = UINT32_MAX;
			} else {
				total_len += stream.gcount();
			}
			buf_list.push_back(buf);
			buf = nullptr;
		}

		// Now construct the buffer from the individual chunks
		uint32_t remaining_len = total_len;
		uint32_t bufp = 0;
		m_buf = new uint8_t[total_len];
		for (auto* b : buf_list) {
			uint32_t to_copy = buf_len;
			if (to_copy > remaining_len) {
				to_copy = remaining_len;
			}
			memcpy(&m_buf[bufp], b, to_copy);
			bufp += to_copy;
			remaining_len -= to_copy;

			delete[] b;
		}

		if (remaining_len > 0) {
			std::cerr << "Logic error: did not copy entre stream!\n";
		}
		m_len = total_len;
		m_free = true;
	}

	~arraybuf()
	{
		if (m_free && m_buf) {
			delete[] m_buf;
		}
	}

	arraybuf(const arraybuf& other) = delete;
	arraybuf& operator=(const arraybuf& rhs) = delete;

	/**
	 * Set the size of the backing array.
	 */
	void reserve(uint32_t length)
	{
		if (m_free && m_buf) {
			delete[] m_buf;
			m_buf = nullptr;
		}

		m_buf = new uint8_t[length];
		m_len = length;
		m_free = true;
	}

	virtual uint32_t length() const override { return m_len; }

	virtual iterator begin() override {
		return iterator(m_buf);
	}

	virtual iterator end() override {
		return iterator(&m_buf[m_len]);
	}

	virtual uint8_t& operator[](uint32_t idx) override {
		return m_buf[idx];
	}

	virtual const uint8_t& operator[](uint32_t idx) const override {
		return m_buf[idx];
	}

private:
	uint8_t* m_buf;
	uint32_t m_len;
	bool m_free;
};

/**
 * Buffer backed by a std::string.
 *
 * Makes a copy of the provided string.
 */
class strbuf : public buffer
{
public:
	strbuf(const std::string& str) :
		m_buf(str)
	{}

	virtual uint32_t length() const override { return m_buf.length(); }

	virtual iterator begin() override {
		return iterator((uint8_t*)&m_buf.c_str()[0]);
	}

	virtual iterator end() override {
		return iterator((uint8_t*)&m_buf.c_str()[m_buf.length()]);
	}

	virtual uint8_t& operator[](uint32_t idx) override {
		return ((uint8_t*)m_buf.c_str())[idx];
	}

	virtual const uint8_t& operator[](uint32_t idx) const override {
		return ((uint8_t*)m_buf.c_str())[idx];
	}

private:
	std::string m_buf;
};

/**
 * Some helper functions to build a buffer from various inputs.
 */
class buffer_conversion
{
public:
	static uint8_t hex_char_to_num(char c)
	{
		switch (c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a': case 'A': return 0xa;
		case 'b': case 'B': return 0xb;
		case 'c': case 'C': return 0xc;
		case 'd': case 'D': return 0xd;
		case 'e': case 'E': return 0xe;
		case 'f': case 'F': return 0xf;
		}
		return 0xff;
	}

	static std::unique_ptr<buffer> number_string_to_buffer(const std::string& str,
	                                                       bool big_endian,
	                                                       bool hex)
	{
		std::string numstr = str;
		if (!hex) {
			// Only hex for now
			return nullptr;
		}

		enum {
			LOW,
			HIGH
		} pos;

		std::list<uint8_t> num_list;
		uint8_t val;

		// Read the 0x prefix, if present
		if (numstr.starts_with("0x") || numstr.starts_with("0X")) {
			numstr = numstr.substr(2);
		}

		pos = LOW;
		for (auto it = numstr.rbegin(); it != numstr.rend(); ++it) {
			if (pos == LOW) {
				val = hex_char_to_num(*it);
				pos = HIGH;
				if (val == 0xff) return nullptr;
			} else {
				uint8_t r = hex_char_to_num(*it);
				if (r == 0xff) return nullptr;
				val += r * 0x10;
				if (big_endian) {
					num_list.push_front(val);
				} else {
					num_list.push_back(val);
				}
				pos = LOW;
			}
		}
		if (pos == HIGH) {
			if (big_endian) {
				num_list.push_front(val);
			} else {
				num_list.push_back(val);
			}
		}

		if (num_list.empty()) {
			return nullptr;
		}

		// We now have a list of bytes in the correct endianness
		auto ret = std::make_unique<arraybuf>(nullptr, num_list.size());
		uint32_t idx = 0;
		for (uint8_t i : num_list) {
			(*ret)[idx++] = i;
		}

		return ret;
	}
};

/**
 * Object representing a search target.
 */
class needle
{
public:
	virtual ~needle() = default;

	virtual uint32_t length() const = 0;

	virtual uint32_t first_match(const buffer& buf, uint32_t start = 0) const = 0;
	virtual std::list<uint32_t> match(const buffer& buf, uint32_t start = 0) const = 0;
};

/**
 * A needle backed by a buffer. No wildcard support.
 */
class buffer_needle : public needle
{
public:
	buffer_needle(uint8_t* arr, uint32_t len) :
		m_buf(arr, len)
	{}

	buffer_needle(const std::vector<uint8_t>& vec) :
		m_buf(vec)
	{}

	buffer_needle(std::initializer_list<uint8_t>&& in_list) :
		m_buf(in_list)
	{}

	virtual uint32_t length() const override { return m_buf.length(); }

	virtual uint32_t first_match(const buffer& haystack, uint32_t start = 0) const
	{
		const uint32_t needle_len = length();
		const uint32_t buf_len = haystack.length();

		if (needle_len > buf_len) {
			return UINT32_MAX;
		}

		uint32_t upto = buf_len - needle_len;
		uint8_t needle_start = m_buf[0];
		for (uint32_t i = start; i <= upto; ++i) {
			// Optimization: compare the first bytes to see if we should even
			// bother calling cmp at all
			if (needle_start != haystack[i]) continue;
			if (haystack.cmp(m_buf, i)) {
				return i;
			}
		}
		return UINT32_MAX;
	}

	virtual std::list<uint32_t> match(const buffer& haystack, uint32_t start = 0) const
	{
		std::list<uint32_t> ret;
		const uint32_t haystack_len = haystack.length();
		const uint32_t needle_len = length();

		if (needle_len > haystack_len) {
			return ret;
		}

		uint32_t upto = haystack_len - needle_len;
		uint8_t needle_start = m_buf[0];
		for (uint32_t i = start; i <= upto; ++i) {
			// Optimization: compare the first bytes to see if we should even
			// bother calling cmp at all
			if (needle_start != haystack[i]) continue;
			if (haystack.cmp(m_buf, i)) {
				ret.push_back(i);
			}
		}

		return ret;
	}
private:
	const arraybuf m_buf;
};


/**
 * A needle with wildcard support. Each wildcard only represents a single character,
 * meaning that the search results will all be the same length.
 *
 * How to specify a search string:
 *  - As a string object: "0x4a28?15"
 *    - Use the from_string builder
 *    - Valid wildcards:
 *      .  : A . wildcard represents 4 bits (half a byte). For example,
             0x.a will match 0x0a through 0xfa.
		.* : For a wildcard_const_len needle, .* is equivalent to .., so
		     0x0a.*0c will match 0x0a000c or 0x0aff0c but not 0x0a0b0d0c.
			 A pattern like 0xa.*d will interpret the . and * as one nibble
			 each, so this pattern will match 0xa00d through 0xaffd.
 *  - As a map of offsets to expected values
 *    - Use the from_map builder
 *    - Any internal offsets not specified become wildcards
 *  - As an array of 16-bit values
 *    - The high end is a bitmask representing which bytes to match, and the
 *      low end is the target value. For example, 0xf0ff will match any byte
 *      where the top four bits are set (0xf0 through 0xff), while 0xf000 will
 *      match any byte where the top four bits are unset. 0x01ff (or 0x0101 or
 *      0x010f, which are all equivalent) will match any byte with the least
 *      significant bit set (so any odd number).
 */
class wildcard_const_len : public needle
{
	enum char_type
	{
		DIGIT,
		WILDCARD,
		INVALID
	};

	static char_type get_char_type(char c)
	{
		switch (c) {
		case '.':
		case '*':
			return WILDCARD;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':	case 'a': case 'b':
		case 'c': case 'd': case 'e': case 'f':
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F':
			return DIGIT;
		default:
			return INVALID;
		}
	}

public:
	//
	// Factories
	//

	/**
	 * Build a needle from a wildcard string
	 *
	 * Valid wildcards:
     *  .  : A . wildcard represents 4 bits (half a byte). For example,
     *       0x.a will match 0x0a through 0xfa.
	 *	.* : For a wildcard_const_len needle, .* is equivalent to .., so
	 *	     0x0a.*0c will match 0x0a000c or 0x0aff0c but not 0x0a0b0d0c.
	 *		 A pattern like 0xa.*d will interpret the . and * as one nibble
	 *		 each, so this pattern will match 0xa00d through 0xaffd.
	 */
	static std::unique_ptr<wildcard_const_len> from_string(const std::string& str)
	{
		std::string numstr = str;

		// The string might start with 0x, remove that from the length
		if (str.starts_with("0x") || str.starts_with("0X")) {
			numstr = str.substr(2);
		}

		uint32_t len = (numstr.length() + 1) / 2;
		if (len == 0) {
			return nullptr;
		}

		std::vector<uint16_t> vec(len);
		uint32_t vecp = len - 1;
		enum {
			LOW,
			HIGH
		} pos = LOW;
		uint16_t val;

		for (auto it = numstr.rbegin(); it != numstr.rend(); ++it) {
			char_type ctype = get_char_type(*it);
			if (ctype == INVALID) {
				return nullptr;
			}
			if (pos == LOW) {
				if (ctype == WILDCARD) {
					val = 0;
				} else if (ctype == DIGIT) {
					val = 0x0f00 | buffer_conversion::hex_char_to_num(*it);
				}
				pos = HIGH;
			} else { // pos == HIGH
				if (ctype == WILDCARD) {
					// Nothing to do...
				} else if (ctype == DIGIT) {
					uint8_t r = buffer_conversion::hex_char_to_num(*it);
					val += r * 0x10;
					val |= 0xf000;
				}
				vec[vecp--] = val;
				pos = LOW;
			}
		}
		if (pos == HIGH) {
			vec[vecp--] = val;
		}

		if (vecp != UINT32_MAX) {
			return nullptr;
		}

		// We now have a vector of wildcard values
		return std::make_unique<wildcard_const_len>(vec);
	}

	/**
	 * Build a needle from a map of offsets to values.
	 *
	 * Any offsets between the minimum and maximum offset become wildcards.
	 * Example:
	 *     [1] -> 0xff
	 *     [2] -> 0xfe
	 *     [4] -> 0xfc
	 *   This will translate to "0xfffe..fc", where the byte at offset 3
	 *   is a wildcard. It will match any value from 0xfffe00fc to 0xfffefffc.
	 */
	template<typename map_t>
	static std::unique_ptr<wildcard_const_len> from_map(map_t map)
	{
		return nullptr;
	}

	//
	// Methods
	//

	///
	/// The needle array is a 16-bit integer array
	/// The high end is a bitmask of which bits to match.
	///   0x00 matches nothing -- full wildcard
	///   0xff matches everything -- exact value
	/// The low end is the match value.
	wildcard_const_len(const uint16_t* buf, uint32_t len) :
		m_vec(len)
	{
		// Copy the search string to the internal vector
		for (uint32_t i = 0; i < len; ++i) {
			if (i < 0xff) {
				m_vec[i] = buf[i] | 0xff00;
			} else {
				m_vec[i] = 0xffff; // For now, match everything
			}
		}
	}

	wildcard_const_len(std::initializer_list<uint16_t>&& ilist) :
		m_vec(ilist)
	{}

	wildcard_const_len(std::vector<uint16_t>& vec) :
		m_vec(vec)
	{}

	virtual uint32_t length() const override { return m_vec.size(); }

	virtual uint32_t first_match(const buffer& haystack, uint32_t start = 0) const override
	{
		const uint32_t needle_len = length();
		const uint32_t buf_len = haystack.length();

		if (needle_len > buf_len) {
			return UINT32_MAX;
		}

		uint32_t upto = buf_len - needle_len;
		for (uint32_t i = start; i <= upto; ++i) {
			bool found = true;
			for (uint32_t j = 0; j < needle_len; ++j) {
				if (!byte_match(haystack[i + j], m_vec[j])) {
					found = false;
					break;
				}
			}
			if (found) {
				return i;
			}
		}
		return UINT32_MAX;
	}

	virtual std::list<uint32_t> match(const buffer& haystack, uint32_t start = 0) const override
	{
		std::list<uint32_t> ret;
		const uint32_t needle_len = length();
		const uint32_t buf_len = haystack.length();

		if (needle_len > buf_len) {
			return ret;
		}

		uint32_t upto = buf_len - needle_len;
		for (uint32_t i = start; i <= upto; ++i) {
			bool found = true;
			for (uint32_t j = 0; j < needle_len; ++j) {
				if (!byte_match(haystack[i + j], m_vec[j])) {
					found = false;
					break;
				}
			}
			if (found) {
				ret.push_back(i);
			}
		}
		return ret;
	}

	inline bool byte_match(uint8_t haystack, uint16_t needle) const
	{
		uint8_t mask = (needle >> 8) & 0xff;

		if ((haystack & mask) == (needle & mask)) {
			return true;
		}
		return false;
	}

private:
	std::vector<uint16_t> m_vec;
};
