/* vim: set sts=4 sw=4: */
#ifndef __BINFILE_H__
#define __BINFILE_H__

#include "error.h"
#include "impl.h"

#include <stdint.h>
#include <istream>
#include <iterator>
#include <string>
#include <vector>

namespace coh {
    using std::istream;
    using std::string;
    using std::vector;
    
    class BinFileIterator;
    class Record;
    class RecordFactory;

    struct BinRecordType {
	BinRecordType(): RecType(0) {}
	RecordFactory const* RecType;
	string NameHint;
	string Family;
    };

    struct BinRecordAuto: public BinRecordType {
	BinRecordAuto(string namehint);
    };

    struct BinRecordFactory: public BinRecordType {
	BinRecordFactory(RecordFactory const& rectype);
    };

    struct BinRecordFamily: public BinRecordType {
	BinRecordFamily(string family);
    };

    struct BinRecordName: public BinRecordType {
	BinRecordName(string name);
    };

#define BinRecordExact(x) BinRecordFactory(x::Factory::Get())

    class BinFile {
	public:
	    struct FileEnt {
		string Name;
		uint32_t Date;
	    };

	    BinFile();
	    explicit BinFile(istream& in, BinRecordType const& brtype);
	    ~BinFile();

/* TODO:    int NumExtra() const;
	    vector<char> const* ExtraSection(int n) const; */

	    vector<FileEnt> SourceFiles() const;
	    int Fuzz() const;
	    size_t Size() const;
	    Record const& operator[] (size_t n);

	    // STL compat
	    typedef BinFileIterator iterator;
	    typedef BinFileIterator const_iterator;
	    
	    inline size_t size() const { return Size(); }
	    iterator begin();
	    iterator end();

	protected:
	    void Init(istream &in, BinRecordType const& rtype);

	    friend class BinFileIterator;

	USE_IMPL(BinFile);
    };

    class BinFileIterator {
	public:
	    typedef std::random_access_iterator_tag iterator_category;
	    typedef Record const value_type;
	    typedef size_t difference_type;
	    typedef Record const* pointer;
	    typedef Record const& reference;

	    BinFileIterator();
	    explicit BinFileIterator(shared_ptr<BinFile::Impl> impl,
		    size_t pos = 0);

	    Record const& operator*();
	    Record const* operator->();
	    BinFileIterator& operator++();
	    BinFileIterator operator++(int);
	    BinFileIterator& operator--();
	    BinFileIterator operator--(int);
	    BinFileIterator operator+(int n) const;
	    BinFileIterator operator-(int n) const;
	    int operator-(BinFileIterator const& rhs) const;
	    bool operator<(BinFileIterator const& rhs) const;
	    bool operator==(BinFileIterator const& rhs) const;
	    bool operator!=(BinFileIterator const& rhs) const;
	    Record const& operator[](size_t n);

	protected:
	    shared_ptr<BinFile::Impl> mImpl;
	    size_t mPos;
    };

    template<class T> class BinIterator: public BinFileIterator {
	public:
	    typedef T const value_type;
	    typedef T const* pointer;
	    typedef T const& reference;

	    BinIterator(): BinFileIterator() {};
	    explicit BinIterator(shared_ptr<BinFile::Impl> impl,
		    size_t pos = 0): BinFileIterator(impl, pos) {}
	    BinIterator(BinFileIterator const& orig): BinFileIterator(orig) {}

	    T const& operator*() { return static_cast<T const&>(BinFileIterator::operator*()); }
	    T const* operator->() { return static_cast<T const*>(BinFileIterator::operator->()); }
	    T const& operator[](size_t n) { return static_cast<T const&>(BinFileIterator::operator[](n)); }
    };

    template<class T> class Bin: public BinFile {
	public:
	    Bin(): BinFile() {}
	    explicit Bin(istream& in): BinFile(in, BinRecordFamily(T::StaticFamilyName())) {}
	    explicit Bin(istream& in, BinRecordType const& brtype): BinFile(in, brtype) {}

	    T const& operator[] (size_t n) {
		return static_cast<T const&>(BinFile::operator[](n));
	    }

	    // STL compat
	    typedef BinIterator<T> iterator;
	    typedef BinIterator<T> const_iterator;
	    
	    iterator begin() { return BinFile::begin(); }
	    iterator end() { return BinFile::end(); }
    };
};

LIBCOH_EXCEPTION_CATEGORY(BinFile);
LIBCOH_EXCEPTION(BinFile, Parse, "Parse error");
LIBCOH_EXCEPTION(BinFile, IO, "I/O error");
LIBCOH_EXCEPTION(BinFile, IndexRange, "Index range error");

#endif
