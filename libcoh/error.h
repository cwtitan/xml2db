/* vim: set sts=4 sw=4: */
#ifndef __LIBCOH_H__
#define __LIBCOH_H__

#include <exception>

namespace coh {
    namespace err {
	class Error: public std::exception {
	    private:
		char const* mErrorMsg;

	    protected:
		virtual char const* ErrorClass() const throw() =0;

	    public:
		Error() throw();
		explicit Error(char const* errormsg) throw();
		virtual ~Error() throw();

		virtual char const* what() const throw();
	};
    }
}

#define LIBCOH_EXCEPTION_CATEGORY(catn) namespace coh { namespace err { \
    class catn: public Error { \
	public: \
	    catn() throw(): Error() {} \
	    explicit catn(char const* emsg) throw(): Error(emsg) {} \
	protected: \
	    virtual char const* ErrorClass() const throw() { return "General " #catn " Error"; } \
    }; \
} }

#define LIBCOH_EXCEPTION(catn, name, eclass) namespace coh { namespace err { \
    class catn##_##name: public catn { \
	public: \
	    catn##_##name() throw(): catn() {} \
	    explicit catn##_##name(char const* emsg) throw(): catn(emsg) {} \
	protected: \
	    virtual char const* ErrorClass() const throw() { return eclass; } \
    }; \
} }

#endif
