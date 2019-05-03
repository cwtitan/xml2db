/* vim: set sts=4 sw=4: */
#ifndef __PSTRINGS_IMPL__
#define __PSTRINGS_IMPL__

#include <stdint.h>
#include <istream>
#include <map>
#include <string>
#include <vector>
#include <boost/scoped_array.hpp>
#include <boost/unordered_map.hpp>

namespace coh {
    using boost::scoped_array;
    using boost::unordered_map;
    using std::istream;

    class Chunk {
	public:
	    void Read(istream& input);
	    void ClearIndex();

	    int Size() const;
	    char const* operator[](unsigned int i) const;

	private:
	    scoped_array<char> mStringTable;
	    vector<char*> mIndex;
    };

    struct TransImpl {
	typedef map<string, string> sub_type;

	uint32_t Order;
	char const* Primary;
	char const* Secondary;
	sub_type Sub;

	operator PString::Translation() const;
    };

    class PString::Impl {
	public:
	    void Init(istream& input);

	    int Size() const;
	    void Keys(vector<string>& ret) const;
	    Translation Find(string const& k) const;
	    string operator[](string const& k) const;

	private:
	    typedef unordered_map<string, TransImpl> trans_type;

	    Chunk mMainChunk;
	    Chunk mSubChunk;
	    trans_type mTrans;

	    void Read(istream& input);
    };

}

#endif
