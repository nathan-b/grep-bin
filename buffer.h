#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>
#include <iterator>
#include <string>

/**
 * Class defining a buffer interface.
 */
class buffer
{
public:
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

	virtual bool cmp(const buffer& other)
	{
		return cmp(other, 0);
	}

	virtual bool cmp(const buffer& other, uint32_t start)
	{
		if (other.length() > (length() - start)) {
			return false;
		}
		return memcmp(&other[0], &(*this)[start], other.length()) == 0;
	}

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
	arraybuf() :
		m_buf(nullptr),
		m_len(0),
		m_free(false)
	{}
	arraybuf(uint8_t* arr, uint32_t length) :
		m_buf(arr),
		m_len(length),
		m_free(false)
	{
		if (!arr) {
			m_buf = new uint8_t[length];
			m_free = true;
		}
	}

	~arraybuf()
	{
		if (m_free && m_buf) {
			delete[] m_buf;
		}
	}

	arraybuf(const arraybuf& other) = delete;
	arraybuf& operator=(const arraybuf& rhs) = delete;

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
