/* vim: set sts=4 sw=4: */

#include "pstring.h"
#include "pstring_impl.h"

#include <boost/foreach.hpp>
#include <memory>
#define foreach BOOST_FOREACH

using namespace std;

namespace coh {

    struct pstring_chunkhdr {
	uint32_t nstrings;
	uint32_t chunklen;
    };

    struct pstring_transdata {
	uint32_t ref1;
	uint32_t ref2;
	uint32_t nsub;
    };

    typedef PString::Translation Translation;

/* -------------------- PString -------------------- */

    PString::PString(): mImpl(new Impl) {}

    PString::PString(istream& input) {
	Init(input);
    }

    void PString::Init(istream& input) {
	mImpl.reset(new Impl);
	mImpl->Init(input);
    }

    int PString::Size() const { return mImpl->Size(); }
    vector<string> PString::Keys() const {
	vector<string> ret;
	mImpl->Keys(ret);
	return ret;
    }
    Translation PString::Find(string k) const { return mImpl->Find(k); }
    string PString::operator[](string k) const { return (*mImpl)[k]; }

    PString::~PString() {}

/* -------------------- Chunk -------------------- */

    void Chunk::Read(istream& input) {
	pstring_chunkhdr hdr;
	input.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));

	mStringTable.reset(new char[hdr.chunklen]);
	mIndex.resize(hdr.nstrings);
	input.read(mStringTable.get(), hdr.chunklen);

	uint32_t i = 0;
	char *last = mStringTable.get();
	char *next = last;

	while ((next < mStringTable.get() + hdr.chunklen) &&
		(i < hdr.nstrings)) {
	    if (*next == '\0') {
		mIndex[i++] = last;
		last = next + 1;
	    }
	    ++next;
	}
    }

    void Chunk::ClearIndex() {
	mIndex.clear();
    }

    int Chunk::Size() const {
	return mIndex.size();
    }

    char const* Chunk::operator[](unsigned int i) const {
	if (i >= mIndex.size())
	    throw err::PString_IndexRange();
	return mIndex[i];
    }

/* -------------------- PString::Impl -------------------- */

    void PString::Impl::Init(istream& input) {
	mTrans.clear();
	Read(input);
    }

    void PString::Impl::Keys(vector<string>& ret) const {
	foreach(trans_type::value_type ti, mTrans) {
	    ret.push_back(ti.first);
	}
    }

    Translation PString::Impl::Find(string const& k) const {
	trans_type::const_iterator i = mTrans.find(k);
	if (i == mTrans.end())
	    return Translation(NotFound, string(), string());

	return i->second;
    }

    int PString::Impl::Size() const {
	return mTrans.size();
    }

    string PString::Impl::operator[](const string& k) const {
	trans_type::const_iterator i = mTrans.find(k);
	if (i == mTrans.end())
	    return k;

	return i->second.Primary;
    }

    void PString::Impl::Read(istream& input) {
	char magic[4];
	input.exceptions(ios::failbit | ios::badbit);
	input.read(magic, 4);

	mMainChunk.Read(input);
	mSubChunk.Read(input);

	uint32_t numtrans;
	vector<char> keybuf;
	vector<uint32_t> subbuf;
	input.read(reinterpret_cast<char*>(&numtrans), sizeof(numtrans));
	for (uint32_t i = 0; i < numtrans; ++i) {
	    uint32_t keysize;

	    input.read(reinterpret_cast<char*>(&keysize), sizeof(keysize));
	    if(keybuf.size() < keysize + 1)
		keybuf.resize(keysize + 1);
	    input.read(&keybuf[0], keysize);
	    keybuf[keysize] = '\0';

	    pstring_transdata tdata;
	    input.read(reinterpret_cast<char*>(&tdata), sizeof(tdata));

	    TransImpl& ntrans = mTrans[&keybuf[0]];
	    ntrans.Order = tdata.ref1;
	    ntrans.Primary = mMainChunk[tdata.ref1];
	    ntrans.Secondary = mMainChunk[tdata.ref2];

	    if (subbuf.size() < tdata.nsub)
		subbuf.resize(tdata.nsub);
	    input.read(reinterpret_cast<char*>(&subbuf[0]),
		    sizeof(uint32_t) * tdata.nsub);
	    for (uint32_t s = 0; s < tdata.nsub; ++s)
		ntrans.Sub[mSubChunk[subbuf[s]]] = mSubChunk[subbuf[s] + 1];
	}

	mMainChunk.ClearIndex();
	mSubChunk.ClearIndex();
    }

/* -------------------- PString::Translation -------------------- */

    PString::Translation::Translation() {}

    PString::Translation::Translation(uint32_t order, string primary,
	    string secondary):
	Order(order),
	Primary(primary),
	Secondary(secondary) {}

/* -------------------- TransImpl -------------------- */

    TransImpl::operator PString::Translation() const {
	Translation n(Order, Primary, Secondary);
	n.Sub = Sub;
	return n;
    }

}
