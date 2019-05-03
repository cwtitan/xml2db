/* vim: set sts=4 sw=4: */
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "error.h"

#include <stdint.h>
#include <istream>
#include <string>
#include <vector>
#include <boost/shared_array.hpp>

LIBCOH_EXCEPTION_CATEGORY(Buffer);
LIBCOH_EXCEPTION(Buffer, IO, "I/O Error");
LIBCOH_EXCEPTION(Buffer, Parse, "Parse Error");
LIBCOH_EXCEPTION(Buffer, Range, "Range Error");

namespace coh {
    using boost::shared_array;
    using std::istream;
    using std::string;
    using std::vector;

    uint32_t const BUFFER_MAXARRAYSIZE = 131072;

    class SimpleRecord;
    class Record;
    class Buffer {
	public:
	    typedef uint32_t offset_type;

	private:
	    typedef shared_array<char> bufref_type;

	    bufref_type mDataBlock;
	    char* mData;
	    offset_type mReadPos;
	    offset_type mDataSize;
	    int mFuzz;

	    Buffer(bufref_type parent, char* d, offset_type sz);

	public:
	    Buffer(istream& src, offset_type len);

	    vector<char> GetBytes(offset_type off, offset_type len) const;

	    int Fuzz() const;
	    offset_type Size() const;
	    offset_type Tell() const;
	    offset_type Seek(offset_type off);
	    void ClearFuzz();
	    void Rewind();
	    Buffer Slice(offset_type sz);

	    Buffer& operator>> (bool& v);
	    Buffer& operator>> (uint16_t& v);
	    Buffer& operator>> (int32_t& v);
	    Buffer& operator>> (uint32_t& v);
	    Buffer& operator>> (int64_t& v);
	    Buffer& operator>> (uint64_t& v);
	    Buffer& operator>> (float& v);
	    Buffer& operator>> (string& v);
	    Buffer& operator>> (SimpleRecord& v);
	    Buffer& operator>> (Record& v);
	    template<typename T> Buffer& operator>> (vector<T>& v) {
		uint32_t len;
		T temp;

		*this >> len;
		if (len > BUFFER_MAXARRAYSIZE)
		    throw err::Buffer_Parse();

		v.clear();
		for (uint32_t i = 0; i < len; ++i) {
		    *this >> temp;
		    v.push_back(temp);
		}

		return *this;
	    }

	    // STL compat
	    inline size_t size() const { return Size(); }
    };

}

#endif
