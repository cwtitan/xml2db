/* vim: set sts=4 sw=4: */
#ifndef __RECORD_H__
#define __RECORD_H__

#include "error.h"
#include <map>
#include <string>
#include <vector>

namespace coh {
    using std::map;
    using std::string;
    using std::vector;

    class Buffer;
    class Record;

    class RecordFactory {
	public:
	    virtual Record* Create() const =0;
    };

    template<class T>
	class RecordFactoryT: public RecordFactory {
	    public:
		virtual typename T::IsRecord * Create() const {
		    return new T;
		}
		static RecordFactoryT<T> const& Get() {
		    static RecordFactoryT<T> theoneandonly;
		    return theoneandonly;
		}
	    private:
		RecordFactoryT() {}
	};

    class Record {
	public:
	    Record();
	    virtual ~Record();
	    explicit Record(Buffer& buf);
	    int Parse(Buffer& buf);
	    virtual vector<char> GetRemain() const;

	    virtual string TypeName() const =0;
	    virtual string FamilyName() const =0;
	    virtual string NameHint() const =0;
	    virtual Record* ToFamily() const =0;

	protected:
	    virtual void ParseFields(Buffer& buf) =0;
	    virtual int ParseRemain(Buffer& buf);
    };

    class BinRecord: public Record {
	private:
	    vector<char> mRemain_;

	protected:
	    virtual int ParseRemain(Buffer& buf);

	public:
	    virtual vector<char> GetRemain() const;
    };

    class GenericRecord: public BinRecord {
	protected:
	    virtual void ParseFields(Buffer& buf);
	public:
	    typedef GenericRecord IsRecord;
	    typedef GenericRecord Family;
	    typedef RecordFactoryT<GenericRecord> Factory;
	    virtual string TypeName() const;
	    virtual string FamilyName() const;
	    virtual string NameHint() const;
	    virtual GenericRecord* ToFamily() const;
    };

    class SimpleRecord: public Record {};

    struct DetectResults {
	RecordFactory const* Factory;
	int Score;
	DetectResults(): Factory(0), Score(-1) {};
    };

    DetectResults DetectRecord(Buffer& in, string, bool repeat);
    DetectResults RecordByFamily(Buffer& in, string family);
    RecordFactory const& RecordByName(string name);

    typedef map<string, int32_t> FieldEnum;
    typedef map<int32_t, string> FieldRevEnum;

    class FieldEnumInfo {
	public:
	virtual bool IsFlags() const { return false; }
	virtual string GetEnumKey() const { return ""; }
	virtual FieldEnum GetEnum() const { return FieldEnum(); }
	virtual FieldRevEnum GetReverseEnum() const { return FieldRevEnum(); }
    };
}

LIBCOH_EXCEPTION_CATEGORY(Record);
LIBCOH_EXCEPTION(Record, Parse, "Parse Error");
LIBCOH_EXCEPTION(Record, UnknownType, "Unknown Record Type");

#endif
