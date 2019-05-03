/* vim: set sts=4 sw=4: */
#include "error.h"

namespace coh {
    namespace err {

	Error::Error() throw():
	    mErrorMsg(0) {}

	Error::Error(char const* errormsg) throw():
	    mErrorMsg(errormsg) {}

	char const* Error::what() const throw() {
	    if (mErrorMsg)
		return mErrorMsg;
	    return ErrorClass();
	}

	Error::~Error() throw() {
	}

    }
}
