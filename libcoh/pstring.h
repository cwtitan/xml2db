/* vim: set sts=4 sw=4: */
#ifndef __PSTRING_H__
#define __PSTRING_H__

#include "error.h"
#include "impl.h"

#include <istream>
#include <map>
#include <string>
#include <vector>

namespace coh {
    using std::istream;
    using std::map;
    using std::string;
    using std::vector;

    class PString {
	public:
	    struct Translation {
		typedef map<string, string> sub_type;

		uint32_t Order;
		string Primary;
		string Secondary;
		sub_type Sub;

		Translation();
		Translation(uint32_t order, string primary, string secondary);
		inline operator bool() const { return Order != NotFound; }
	    };

	    PString();
	    explicit PString(istream& input);
	    ~PString();

	    int Size() const;
	    vector<string> Keys() const;
	    Translation Find(string k) const;
	    static uint32_t const NotFound = 0xfffffff;
	    string operator[](string k) const;

	    // STL compat
	    inline size_t size() const { return Size(); }

	protected:
	    void Init(istream& input);

	USE_IMPL(PString);
    };

}

LIBCOH_EXCEPTION_CATEGORY(PString);
LIBCOH_EXCEPTION(PString, Parse, "Parse error");
LIBCOH_EXCEPTION(PString, IO, "I/O error");
LIBCOH_EXCEPTION(PString, IndexRange, "Index range error");

#endif
