#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <initializer_list>
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

	virtual ~buffer() {}

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

		for (uint32_t i = start_at; i <= len - needle_len; ++i) {
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

		for (uint32_t i = start_at; i <= len - needle_len; ++i) {
			if (cmp(needle, i)) {
				ret.push_back(i);
				i += (needle_len - 1);
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
		m_buf = new uint8_t(m_len);
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
public:
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
