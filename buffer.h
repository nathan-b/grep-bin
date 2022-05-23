#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <string>

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
	virtual bool cmp(const buffer& other)
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
	virtual bool cmp(const buffer& other, uint32_t start)
	{
		if (other.length() > (length() - start)) {
			return false;
		}
		return memcmp(&other[0], &(*this)[start], other.length()) == 0;
	}

	/**
	 * Find all instances of @needle in this buffer.
	 *
	 * Returns a list of buffer offsets where the needle is found.
	 */
	virtual std::list<uint32_t> find_all(const buffer& needle)
	{
		std::list<uint32_t> ret;
		const uint32_t len = length();
		const uint32_t needle_len = needle.length();

		if (needle_len > len) {
			return ret;
		}

		for (uint32_t i = 0; i < len - needle_len; ++i) {
			if (cmp(needle, i)) {
				ret.push_back(i);
				i += (needle_len - 1);
			}
		}

		return ret;
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
 */
class strbuf : public buffer
{
public:
	strbuf(std::string& str) :
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
