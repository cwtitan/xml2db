/* vim: set sts=4 sw=4: */
#ifndef __PIGG_IMPL_H__
#define __PIGG_IMPL_H__

#include <stdint.h>
#include <zlib.h>
#include <fstream>
#include <vector>
#include <memory>
#include <boost/scoped_array.hpp>
#include <boost/unordered_map.hpp>

namespace coh {
    using boost::scoped_array;
    using boost::unordered_map;
    using std::ifstream;
    using std::ios;
    using std::streambuf;
    using std::streamsize;
    using std::vector;
    using std::shared_ptr;

    class StreamManager {
	public:
	    explicit StreamManager(string const& name);

	    template<typename T> int Read(T* buf, streamsize num) {
		return ReadStream(reinterpret_cast<char*>(buf), num);
	    }
	    template<typename T> int Read(T* buf, streamsize num, streampos off) {
		return ReadStream(reinterpret_cast<char*>(buf), num, off);
	    }

	private:
	    ifstream mFile;
	    streampos mCur;

	    int ReadStream(char* buf, streamsize num);
	    int ReadStream(char* buf, streamsize num, streampos off);
    };

    class Pigg::Impl {
	public:
	    typedef unordered_map<string, uint32_t> index_type;
	    typedef Pigg::FileInfo FileInfo;

	    void Init(string const& name);

	    uint32_t NumFiles() const;
	    uint32_t FindFile(string& name) const;

	    FileInfo const& Info(uint32_t idx) const;
	    istream* OpenFile(uint32_t idx) const;
	    istream* OpenFile(string& name) const;

	private:
	    shared_ptr<StreamManager> mInput;
	    vector<FileInfo> mDir;
	    vector<vector<char> > mExtra;
	    index_type mIndex;

	    void Parse();
    };

    class PiggStreamBuf: public streambuf {
	public:
	    typedef Pigg::FileInfo FileInfo;

	    PiggStreamBuf(shared_ptr<StreamManager> stream,
		    FileInfo const& info, int bufsize);
	    virtual ~PiggStreamBuf();

	    // streambuf overrides
	    int is_open() const;
	    void close();
	    virtual int underflow();
	    virtual pos_type seekpos(pos_type, ios::openmode);
	    virtual pos_type seekoff(off_type, ios::seekdir, ios::openmode);

	private:
	    shared_ptr<StreamManager> mStream;
	    FileInfo mInfo;
	    streamsize mBufSize;
	    scoped_array<char> mBuffer;
	    scoped_array<char> mZBuffer;

	    streampos mCurPos;
	    streampos mZPos;
	    shared_ptr<z_stream> mZStream;

	    void ZReset();
	    int ZUnderflow();
    };

    class PiggIStream: public istream {
	public:
	    PiggIStream(shared_ptr<StreamManager> stream,
		    Pigg::FileInfo const& info, int bufsize = 16384);
	    virtual ~PiggIStream();

	    // istream overrides
	    virtual streambuf* rdbuf() { return &mBuf; }

	protected:
	    PiggStreamBuf mBuf;
    };

}

#endif
