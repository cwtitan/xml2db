/* vim: set sts=4 sw=4: */
#ifndef __FILENAME_H__
#define __FILENAME_H__

#include "error.h"

#include <istream>
#include <string>
#include <memory>

namespace coh {
    using std::istream;
    using std::string;
    using std::shared_ptr;

    class Pigg;
    class FileName {
	public:
	    FileName();

	    explicit FileName(string name, string deffile = "");

	    string GetPigg() const;
	    void SetPigg(string pigg);
	    string GetFile() const;
	    void SetFile(string file);
	    istream* Open();

	protected:
	    string mPigg;
	    string mFile;
	    shared_ptr<Pigg> mPiggRef;
    };

}

LIBCOH_EXCEPTION_CATEGORY(FileName);
LIBCOH_EXCEPTION(FileName, Invalid, "Invalid filename");

#endif
