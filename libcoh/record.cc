/* vim: set sts=4 sw=4: */
#include "record.h"
#include "buffer.h"
#include "recordtypes.h"
#include <iostream>

namespace coh {

/* -------------------- Record -------------------- */

    Record::Record() {}

    Record::Record(Buffer& buf) {
	Parse(buf);
    }

    Record::~Record() {}

    int Record::Parse(Buffer& buf) {
	int bias = 0;
	try {
	    ParseFields(buf);
	} catch (err::Error& e) {
	    bias = 100;
	}
	return ParseRemain(buf) + bias;
    }

    int Record::ParseRemain(Buffer& buf) {
	return 0;
    }

    vector<char> Record::GetRemain() const {
	return vector<char>();
    }

/* -------------------- BinRecord -------------------- */

    int BinRecord::ParseRemain(Buffer& buf) {
	mRemain_ = buf.GetBytes(buf.Tell(), buf.Size() - buf.Tell());
	return mRemain_.size();
    }

    vector<char> BinRecord::GetRemain() const {
	return mRemain_;
    }

/* -------------------- GenericRecord -------------------- */

    string GenericRecord::TypeName() const {
	return "Generic";
    }

    string GenericRecord::FamilyName() const {
	return "Generic";
    }

    string GenericRecord::NameHint() const {
	return "";
    }

    GenericRecord* GenericRecord::ToFamily() const {
	return 0;
    }

    void GenericRecord::ParseFields(Buffer& buf) {}

}
