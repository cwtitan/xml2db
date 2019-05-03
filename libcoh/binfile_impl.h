/* vim: set sts=4 sw=4: */
#ifndef __BINFILE_IMPL__
#define __BINFILE_IMPL__

#include "buffer.h"
#include "record.h"

#include <stdint.h>
#include <istream>
#include <string>
#include <vector>
#include <memory>
#include <boost/scoped_ptr.hpp>

namespace coh {
    using boost::scoped_ptr;
    using std::istream;
    using std::string;
    using std::vector;
    using std::shared_ptr;

    class BinFile::Impl {
	public:
	    Impl();
	    void Init(istream &in, BinRecordType const& brtype);

	    vector<FileEnt> SourceFiles() const;
	    int Fuzz() const;
	    size_t Size() const;
	    Record const& operator[] (size_t n);

	    // STL compat
	    BinFileIterator begin(shared_ptr<Impl>& impl);
	    BinFileIterator end(shared_ptr<Impl>& impl);

	private:
	    typedef shared_ptr<Record> record_ptr;
	    typedef shared_ptr<Buffer> buffer_ptr;

	    DetectResults mRecType;
	    bool mUseFamily;
	    vector<buffer_ptr> mBuffers;
	    vector<record_ptr> mRecords;
	    vector<BinFile::FileEnt> mSourceList;

	    bool Read(istream& in);
	    void ReadFiles(istream& in);
	    bool ReadRecords(istream& in);
    };

}

#endif
