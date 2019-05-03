/* vim: set sts=4 sw=4: */
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>

#include "pigg.h"
#include "pigg_impl.h"

namespace coh {

    using boost::to_lower;
    using std::min;
    typedef Pigg::FileInfo FileInfo;

    struct pigg_header {
	uint32_t magic;
	uint16_t unknown[4];
	uint32_t dirsize;
    };

    struct pigg_dirent {
	uint32_t magic;
	uint32_t name;
	uint32_t size;
	uint32_t date;
	uint32_t offset;
	uint32_t unknown;
	uint32_t extra;
	unsigned char md5[16];
	uint32_t zsize;
    };

    struct pigg_tblheader {
	uint32_t magic;
	uint32_t count;
	uint32_t size;
    };

/* -------------------- Pigg -------------------- */

    Pigg::Pigg(): mImpl(new Impl) {}

    Pigg::Pigg(string name) {
	Init(name);
    }

    void Pigg::Init(string const& name) {
	mImpl.reset(new Impl);
	mImpl->Init(name);
    }

    Pigg::~Pigg() {}

    uint32_t Pigg::NumFiles() const { return mImpl->NumFiles(); }
    uint32_t Pigg::FindFile(string name) const { return mImpl->FindFile(name); }
    FileInfo Pigg::Info(uint32_t idx) const { return mImpl->Info(idx); }
    istream* Pigg::OpenFile(uint32_t idx) const { return mImpl->OpenFile(idx); }
    istream* Pigg::OpenFile(string name) const { return mImpl->OpenFile(name); }

/* -------------------- Pigg::Impl -------------------- */

    void Pigg::Impl::Parse() {
	pigg_header header;
	mInput->Read(&header, sizeof(header), 0);
	if (header.magic != 0x123)
	    throw err::Pigg_Parse("Invalid PIGG header");

	vector<pigg_dirent> piggdir;
	for (uint32_t i = 0; i < header.dirsize; ++i) {
	    pigg_dirent dirent;
	    mInput->Read(&dirent, sizeof(dirent));
	    if (dirent.magic != 0x3456)
		throw err::Pigg_Parse("Invalid directory entry");
	    piggdir.push_back(dirent);
	}

	pigg_tblheader tblhdr;
	mInput->Read(&tblhdr, sizeof(tblhdr));
	if (tblhdr.magic != 0x6789)
	    throw err::Pigg_Parse("Invalid string table header");

	scoped_array<char> stringbuf(new char[tblhdr.size]);
	char* p = stringbuf.get();
	mInput->Read(p, tblhdr.size);

	vector<char*> stringtbl;
	while (p < stringbuf.get() + tblhdr.size) {
	    uint32_t* lenptr = reinterpret_cast<uint32_t*>(p);
	    stringtbl.push_back(p + 4);
	    p += *lenptr + 4;
	}

	mInput->Read(&tblhdr, sizeof(tblhdr));
	if (tblhdr.magic != 0x9abc)
	    throw err::Pigg_Parse("Invalid extra data header");

	if (tblhdr.size > 0) {
	    scoped_array<char> extrabuf(new char[tblhdr.size]);
	    p = extrabuf.get();
	    mInput->Read(p, tblhdr.size);

	    while (p < extrabuf.get() + tblhdr.size) {
		uint32_t* lenptr = reinterpret_cast<uint32_t*>(p);

		vector<char> nextra;
		nextra.resize(*lenptr);
		memcpy(&nextra[0], p + 4, *lenptr);
		mExtra.push_back(nextra);

		p += *lenptr + 4;
	    }
	}

	for (vector<pigg_dirent>::iterator pdi = piggdir.begin();
		pdi != piggdir.end(); ++pdi) {
	    string name = stringtbl[pdi->name];

	    FileInfo ndir;
	    ndir.Name = name;
	    ndir.Size = pdi->size;
	    ndir.ZSize = pdi->zsize;
	    ndir.Offset = pdi->offset;
	    ndir.Date = pdi->date;
	    if (pdi->extra < mExtra.size())
		ndir.Extra = mExtra[pdi->extra];

	    memcpy(&ndir.MD5, &pdi->md5, sizeof(ndir.MD5));
	    mDir.push_back(ndir);
	    to_lower(name);
	    mIndex[name] = pdi - piggdir.begin();
	}
    }

    void Pigg::Impl::Init(string const& name) {
	mInput.reset(new StreamManager(name));
	Parse();
    }

    uint32_t Pigg::Impl::NumFiles() const {
	return mDir.size();
    }

    FileInfo const& Pigg::Impl::Info(uint32_t idx) const {
	if (idx > mDir.size())
	    throw err::Pigg_IndexRange();
	return mDir[idx];
    }

    uint32_t Pigg::Impl::FindFile(string& name) const {
	to_lower(name);
	index_type::const_iterator itr = mIndex.find(name);
	if (itr == mIndex.end())
	    return Pigg::NotFound;
	return itr->second;
    }

    istream* Pigg::Impl::OpenFile(uint32_t idx) const {
	if (idx > mDir.size())
	    throw err::Pigg_IndexRange();
	return new PiggIStream(mInput, mDir[idx]);
    }

    istream* Pigg::Impl::OpenFile(string& name) const {
	uint32_t idx = FindFile(name);
	if (idx == Pigg::NotFound)
	    throw err::Pigg_FileNotFound();
	return OpenFile(idx);
    }

/* -------------------- StreamManager -------------------- */

    StreamManager::StreamManager(string const& name):
	mFile(name.c_str(), ios::in | ios::binary),
	mCur(0) {
	if (mFile.rdstate() & (istream::failbit | istream::badbit))
	    throw err::Pigg_IO("Error opening file");
    }

    int StreamManager::ReadStream(char* buf, streamsize num) {
	mFile.read(buf, num);
	if (mFile.rdstate() & (istream::failbit | istream::badbit))
	    throw err::Pigg_IO("Error reading file");

	mCur += num;

	return mFile.gcount();
    }

    int StreamManager::ReadStream(char* buf, streamsize num, streampos off) {
	if (off != mCur) {
	    mFile.clear();
	    mFile.seekg(off);
	    mCur = off;
	}
	mFile.read(buf, num);
	if (mFile.rdstate() & (istream::failbit | istream::badbit))
	    throw err::Pigg_IO("Error reading file");

	mCur += num;
	return mFile.gcount();
    }

/* -------------------- PiggStreamBuf -------------------- */

    PiggStreamBuf::PiggStreamBuf(shared_ptr<StreamManager> stream,
	    FileInfo const& info, int bufsize):
	mStream(stream),
	mInfo(info),
	mBufSize(bufsize),
	mBuffer(new char[bufsize]) {
	if (mInfo.ZSize > 0)
	    ZReset();
	setg(mBuffer.get(), mBuffer.get(), mBuffer.get());
    }

    void PiggStreamBuf::ZReset() {
	mZBuffer.reset(new char[mBufSize]);

	mZStream.reset(new z_stream, inflateEnd);
	memset(mZStream.get(), 0, sizeof(z_stream));

	mZStream->next_in = reinterpret_cast<Bytef*>(mZBuffer.get());
	mZStream->avail_in = 0;
	mZStream->next_out = reinterpret_cast<Bytef*>(mBuffer.get());
	mZStream->avail_out = mBufSize;

	if (inflateInit2(mZStream.get(), MAX_WBITS) != Z_OK) {
	    throw err::Pigg_Decompression("Error initializing zlib decoder");
	}
	mZPos = 0;
    }

    int PiggStreamBuf::is_open() const {
	if (mStream)
	    return true;
	return false;
    }

    void PiggStreamBuf::close() {
	mZStream.reset();
	mBuffer.reset();
	mZBuffer.reset();
	mStream.reset();
    }

    int PiggStreamBuf::ZUnderflow() {
	int ret;

	if (mZStream->avail_in == 0) {
	    int didread = mStream->Read(mZBuffer.get(),
		    min(mBufSize, streamsize(mInfo.ZSize - mZPos)),
		    mZPos + mInfo.Offset);
	    mZPos += didread;
	    if (didread == 0)
		return EOF;
	    mZStream->next_in = reinterpret_cast<Bytef*>(mZBuffer.get());
	    mZStream->avail_in = didread;
	}

	mZStream->next_out = reinterpret_cast<Bytef*>(mBuffer.get());
	mZStream->avail_out = mBufSize;
	ret = inflate(mZStream.get(), Z_SYNC_FLUSH);
	if (!(ret == Z_OK || ret == Z_STREAM_END))
	    throw err::Pigg_Decompression();

	setg(mBuffer.get(), mBuffer.get(),
		reinterpret_cast<char*>(mZStream->next_out));
	if (egptr() == gptr())
	    return EOF;

	mCurPos += egptr() - gptr();
	return * reinterpret_cast<unsigned char *>(gptr());
    }

    int PiggStreamBuf::underflow() {
	if (mCurPos >= mInfo.Size)
	    return EOF;

	if (mInfo.ZSize) {
	    return ZUnderflow();
	}

	int didread = mStream->Read(mBuffer.get(),
		min(mBufSize, streamsize(mInfo.Size - mCurPos)),
		mCurPos + mInfo.Offset);
	mCurPos += didread;
	setg(mBuffer.get(), mBuffer.get(), mBuffer.get() + didread);

	return * reinterpret_cast<unsigned char *>(gptr());
    }

    PiggStreamBuf::pos_type PiggStreamBuf::seekpos(pos_type pos,
	    ios::openmode __mode = ios::in | ios::out) {
	return seekoff(off_type(pos), ios::beg, __mode);
    }

    PiggStreamBuf::pos_type PiggStreamBuf::seekoff(off_type off,
	    ios::seekdir dir,
	    ios::openmode __mode = ios::in | ios::out) {
	if (off == 0 && dir == ios::cur) {
	    // hack to make tellg not kill performance for compressed files
	    return mCurPos - off_type(in_avail());
	}

	switch(dir) {
	    case ios::beg:
		mCurPos = 0;
		break;
	    case ios::cur:
		mCurPos -= in_avail();   // subtract unread buffer
		break;
	    case ios::end:
		mCurPos = mInfo.Size;
		break;
	    default:
		throw err::Pigg_InvalidParam("Invalid seek type");
		break;
	}
	mCurPos += off;

	if (mInfo.ZSize == 0)
	    setg(mBuffer.get(), mBuffer.get(), mBuffer.get());
	else {			// oh boy
	    streampos wantpos = mCurPos;
	    ZReset();
	    mCurPos = 0;
	    while (mCurPos < wantpos) {
		if (ZUnderflow() == EOF)
		    return pos_type(off_type(-1));
	    }

	    setg(egptr() - mCurPos + wantpos, egptr() - mCurPos + wantpos,
		    egptr());
	    return wantpos;
	}

	return mCurPos;
    }


    PiggStreamBuf::~PiggStreamBuf() {}

/* -------------------- PiggIStream -------------------- */

    PiggIStream::PiggIStream(shared_ptr<StreamManager> stream,
	    Pigg::FileInfo const& info, int bufsize):
	istream(&mBuf),
	mBuf(stream, info, bufsize)
    {
	init(&mBuf);
    }

    PiggIStream::~PiggIStream() {}

}
