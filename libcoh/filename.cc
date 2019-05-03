/* vim: set sts=4 sw=4: */

#include "filename.h"
#include "pigg.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <fstream>

using namespace std;
using namespace boost;

namespace coh {

    FileName::FileName() {}

    FileName::FileName(string name, string deffile) {
	size_t colon = name.find(":");

	if (colon != string::npos && colon != name.size()) {
	    mPigg = name.substr(0, colon);
	    mFile = name.substr(colon + 1);
	    return;
	}
	
	if (name.size() > 5) {
	    string lastp = name.substr(name.size() - 5);
	    to_lower(lastp);
	    if (lastp == ".pigg") {
		mPigg = name;
		mFile = deffile;
		return;
	    }
	}

	mFile = name;
    }

    string FileName::GetPigg() const { return mPigg; }
    string FileName::GetFile() const { return mFile; }
    void FileName::SetPigg(string pigg) {
	mPigg = pigg;
	mPiggRef.reset();
    }
    void FileName::SetFile(string file) { mFile = file; }

    istream* FileName::Open() {
	if (mFile.empty())
	    throw err::FileName_Invalid();

	if (mPigg.empty())
	    return new ifstream(mFile.c_str(), ios::in | ios::binary);

	if (!mPiggRef)
	    mPiggRef.reset(new Pigg(mPigg));

	return mPiggRef->OpenFile(mFile);
    }

}
