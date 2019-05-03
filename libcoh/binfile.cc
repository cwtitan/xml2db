/* vim: set sts=4 sw=4: */

#include "binfile.h"
#include "binfile_impl.h"
#include "record.h"
#include <iostream>

using namespace std;

namespace coh {

    static uint32_t readlong(istream& in) {
	uint32_t ret;
	in.read(reinterpret_cast<char*>(&ret), sizeof(ret));
	return ret;
    }

    static string readstring(istream& in) {
	uint16_t len;
	vector<char> buf;
	streampos pos;
	string ret;

	in.read(reinterpret_cast<char*>(&len), sizeof(len));

	buf.resize(len);
	in.read(&buf[0], len);
	ret.assign(&buf[0], len);

	pos = in.tellg();
	if (pos % 4)
	    in.seekg(pos % 4 + 4);

	return ret;
    }

/* -------------------- BinRecordTypes -------------------- */

    BinRecordAuto::BinRecordAuto(string namehint) {
	NameHint = namehint;
    }
    
    BinRecordFactory::BinRecordFactory(RecordFactory const& rectype) {
	RecType = &rectype;
    }

    BinRecordFamily::BinRecordFamily(string family) {
	Family = family;
    }

    BinRecordName::BinRecordName(string name) {
	RecType = &RecordByName(name);
    }

/* -------------------- BinFile -------------------- */

    BinFile::BinFile(): mImpl(new Impl) {}

    BinFile::BinFile(istream& in, BinRecordType const& brtype) {
	Init(in, brtype);
    }

    void BinFile::Init(istream& in, BinRecordType const& brtype) {
	mImpl.reset(new Impl);
	mImpl->Init(in, brtype);
    }

    BinFile::~BinFile() { }

    vector<BinFile::FileEnt> BinFile::SourceFiles() const {
	return mImpl->SourceFiles();
    }
    int BinFile::Fuzz() const { return mImpl->Fuzz(); }
    size_t BinFile::Size() const { return mImpl->Size(); }
    Record const& BinFile::operator[] (size_t n) { return (*mImpl)[n]; }
    BinFile::iterator BinFile::begin() {
	return mImpl->begin(mImpl);
    }
    BinFile::iterator BinFile::end() {
	return mImpl->end(mImpl);
    }

/* -------------------- BinFile::Impl -------------------- */

    BinFile::Impl::Impl(): mUseFamily(false) {}

    void BinFile::Impl::Init(istream& in, BinRecordType const& brtype) {
	mRecType.Factory = brtype.RecType;
	bool single = Read(in);

	if (!mRecType.Factory) {
	    if (mBuffers.empty())
		throw err::BinFile_Parse("Empty data section");

	    if (brtype.Family.size()) {
		mRecType = RecordByFamily(*mBuffers[0], brtype.Family);
		mUseFamily = true;
	    } else {
		mRecType = DetectRecord(*mBuffers[0], brtype.NameHint, single);
	    }
	}

	// For oddball records, have to parse all of them up front to get
	// an accurate count.
	if (single) {
	    Buffer datasec = *mBuffers[0];
	    mRecords.clear();
	    while (datasec.Tell() < datasec.Size()) {
		Buffer::offset_type last = datasec.Tell();
		record_ptr nrec(mRecType.Factory->Create());
		nrec->Parse(datasec);
		if (mUseFamily) {
		    Record* rfam = nrec->ToFamily();
		    if (rfam)
			nrec.reset(rfam);
		}
		mRecords.push_back(nrec);

		// Avoid infinite loop with Generic (which eats 0 bytes)
		if (datasec.Tell() == last)
		    break;
	    }
	    mBuffers[0].reset();
	}
    }

    vector<BinFile::FileEnt> BinFile::Impl::SourceFiles() const {
	return mSourceList;
    }

    int BinFile::Impl::Fuzz() const {
	return mRecType.Score;
    }

    size_t BinFile::Impl::Size() const {
	return mRecords.size();
    }

    Record const& BinFile::Impl::operator[] (size_t n) {
	if (n > mRecords.size())
	    throw err::BinFile_IndexRange();

	if (!mRecords[n] && mBuffers[n]) {
	    mRecords[n].reset(mRecType.Factory->Create());
	    int ret = mRecords[n]->Parse(*mBuffers[n]);
	    if (ret + mBuffers[n]->Fuzz() > mRecType.Score)
		mRecType.Score = ret + mBuffers[n]->Fuzz();
	    if (mUseFamily) {
		Record* rfam = mRecords[n]->ToFamily();
		if (rfam)
		    mRecords[n].reset(rfam);
	    }
	    mBuffers[n].reset();
	}

	return *mRecords[n];
    }

    bool BinFile::Impl::Read(istream& in) {
	char signature[8];
	uint32_t checksum;
	string htype[2];

	in.seekg(0);
	in.read(signature, 8);
	checksum = readlong(in);
	htype[0] = readstring(in);
	htype[1] = readstring(in);

	ReadFiles(in);
	return ReadRecords(in);
    }

    void BinFile::Impl::ReadFiles(istream& in) {
	uint32_t nentries;
	Buffer filesec(in, readlong(in));
	filesec >> nentries;

	BinFile::FileEnt nent;
	for (uint32_t i = 0; i < nentries; ++i) {
	    filesec >> nent.Name >> nent.Date;
	    mSourceList.push_back(nent);
	}
    }

    bool BinFile::Impl::ReadRecords(istream& in) {
	bool single = false;
	Buffer datasec(in, readlong(in));

	try {
	    uint32_t nentries;
	    datasec >> nentries;

	    uint32_t reclen;
	    for (uint32_t i = 0; i < nentries; ++i) {
		datasec >> reclen;
		mBuffers.push_back(buffer_ptr(new Buffer(datasec.Slice(reclen))));
	    }

	    if (datasec.Tell() == datasec.Size())   // Processed entire file?
		mRecords.resize(nentries);
	    else
		single = true;
	} catch(...) {
	    single = true;
	}

	if (single) {
	    datasec.Rewind();
	    mBuffers.clear();
	    mBuffers.push_back(buffer_ptr(new Buffer(datasec)));
	}

	return single;
    }

    BinFileIterator BinFile::Impl::begin(shared_ptr<Impl>& impl) {
	return BinFileIterator(impl, 0);
    }

    BinFileIterator BinFile::Impl::end(shared_ptr<Impl>& impl) {
	return BinFileIterator(impl, mRecords.size());
    }

/* -------------------- BinFileIterator -------------------- */

    BinFileIterator::BinFileIterator(): mPos(0) {}

    BinFileIterator::BinFileIterator(shared_ptr<BinFile::Impl> impl,
	    size_t pos):
	mImpl(impl), mPos(pos) {}

    Record const& BinFileIterator::operator*() {
	return (*mImpl)[mPos];
    }

    Record const* BinFileIterator::operator->() {
	return &(*mImpl)[mPos];
    }

    BinFileIterator& BinFileIterator::operator++() {
	++mPos;
	return *this;
    }

    BinFileIterator BinFileIterator::operator++(int) {
	BinFileIterator ret = *this;
	++mPos;
	return ret;
    }

    BinFileIterator& BinFileIterator::operator--() {
	--mPos;
	return *this;
    }

    BinFileIterator BinFileIterator::operator--(int) {
	BinFileIterator ret = *this;
	--mPos;
	return ret;
    }

    BinFileIterator BinFileIterator::operator+(int n) const {
	return BinFileIterator(mImpl, mPos + n);
    }

    BinFileIterator BinFileIterator::operator-(int n) const {
	return BinFileIterator(mImpl, mPos - n);
    }

    int BinFileIterator::operator-(BinFileIterator const& rhs) const {
	return mPos - rhs.mPos;
    }
    
    bool BinFileIterator::operator<(BinFileIterator const& rhs) const {
	return (mImpl == rhs.mImpl) && (mPos < rhs.mPos);
    }

    bool BinFileIterator::operator==(BinFileIterator const& rhs) const {
	return (mImpl == rhs.mImpl) && (mPos == rhs.mPos);
    }

    bool BinFileIterator::operator!=(BinFileIterator const& rhs) const {
	return (mImpl != rhs.mImpl) || (mPos != rhs.mPos);
    }

    Record const& BinFileIterator::operator[](size_t n) {
	return (*mImpl)[mPos + n];
    }

}
