/* vim: set sts=4 sw=4: */
#ifndef __PIGG_H__
#define __PIGG_H__

#include "error.h"
#include "impl.h"

#include <stdint.h>
#include <istream>
#include <string>
#include <vector>

namespace coh {
    using std::istream;
    using std::streampos;
    using std::string;
    using std::vector;

    class Pigg {
	public:
	    struct FileInfo {
		string Name;
		uint32_t Size;
		uint32_t ZSize;
		uint32_t Date;
		streampos Offset;
		vector<char> Extra;
		unsigned char MD5[16];
	    };

	    Pigg();
	    explicit Pigg(string name);
	    ~Pigg();

	    uint32_t NumFiles() const;
	    uint32_t FindFile(string name) const;
	    static uint32_t const NotFound = 0xfffffff;

	    FileInfo Info(uint32_t idx) const;
	    istream* OpenFile(uint32_t idx) const;
	    istream* OpenFile(string name) const;

	protected:
	    void Init(string const& name);

	USE_IMPL(Pigg);
    };

}

LIBCOH_EXCEPTION_CATEGORY(Pigg);
LIBCOH_EXCEPTION(Pigg, InvalidParam, "Invalid parameter");
LIBCOH_EXCEPTION(Pigg, Decompression, "Decompression error");
LIBCOH_EXCEPTION(Pigg, Parse, "Parse error");
LIBCOH_EXCEPTION(Pigg, IO, "I/O error");
LIBCOH_EXCEPTION(Pigg, IndexRange, "Index out of range");
LIBCOH_EXCEPTION(Pigg, FileNotFound, "File not found");

#endif
