/* vim: set sts=4 sw=4: */

#include "buffer.h"
#include "record.h"

#include <algorithm>
#include <iostream>

using namespace std;

#define EXTRACTOR(type) Buffer& Buffer::operator>> (type& v) { \
    if (mReadPos + sizeof(type) > mDataSize) \
	throw err::Buffer_Range(); \
    v = *reinterpret_cast<type*>(mData + mReadPos); \
    mReadPos += sizeof(type); \
    return *this; \
}

namespace coh {

    typedef Buffer::offset_type offset_type;

/* -------------------- Buffer -------------------- */

    Buffer::Buffer(istream &src, offset_type len):
	mDataBlock(new char[len]),
	mData(mDataBlock.get()),
	mReadPos(0),
	mDataSize(len),
	mFuzz(0) {
	src.read(mDataBlock.get(), len);
	if ((src.rdstate() & (ios::failbit | ios::badbit)) ||
		offset_type(src.gcount()) != len)
	    throw err::Buffer_IO();
    }

    Buffer::Buffer(bufref_type parent, char* d, offset_type sz):
	mDataBlock(parent),
	mData(d),
	mReadPos(0),
	mDataSize(sz),
	mFuzz(0) {}

    vector<char> Buffer::GetBytes(offset_type off, offset_type len) const {
	if (off > mDataSize)
	    off = mDataSize;
	if (len > mDataSize - off)
	    len = mDataSize - off;

	return vector<char>(mData + off, mData + off + len);
    }

    int Buffer::Fuzz() const {
	return mFuzz;
    }

    offset_type Buffer::Size() const {
	return mDataSize;
    }

    offset_type Buffer::Tell() const {
	return mReadPos;
    }

    offset_type Buffer::Seek(offset_type off) {
	mReadPos = min(off, mDataSize);
	return mReadPos;
    }

    void Buffer::ClearFuzz() {
	mFuzz = 0;
    }

    void Buffer::Rewind() {
	mReadPos = 0;
    }

    Buffer Buffer::Slice(offset_type sz) {
	if (mReadPos + sz > mDataSize)
	    sz = mDataSize - mReadPos;
	Buffer ret(mDataBlock, mData + mReadPos, sz);
	mReadPos += sz;
	return ret;
    }

    EXTRACTOR(uint16_t);
    EXTRACTOR(int32_t);
    EXTRACTOR(uint32_t);
    EXTRACTOR(int64_t);
    EXTRACTOR(uint64_t);
    EXTRACTOR(float);

    Buffer& Buffer::operator>> (bool& v) {
	if (mReadPos + 4 > mDataSize)
	    throw err::Buffer_Range();
	v = *reinterpret_cast<int32_t*>(mData + mReadPos);
	mReadPos += 4;
	return *this;
    }

    Buffer& Buffer::operator>> (string& v) {
	uint16_t len;

	*this >> len;
	if (mReadPos + len > mDataSize)
	    throw err::Buffer_Range();

	v.assign(mData + mReadPos, len);
	if (v.find('\0') != string::npos)
	    throw err::Buffer_Parse("Embedded null in string");
	mReadPos += len;

	/* BinFile string alignment */
	len += 2;
	if (len % 4)
	    mReadPos += 4 - (len % 4);

	return *this;
    }

    Buffer& Buffer::operator>> (SimpleRecord& v) {
	mFuzz += v.Parse(*this);
	return *this;
    }

    Buffer& Buffer::operator>> (Record& v) {
	uint32_t len;

	*this >> len;
	Buffer tbuf = Slice(len);
	mFuzz += v.Parse(tbuf) + tbuf.mFuzz;

	return *this;
    }

}
