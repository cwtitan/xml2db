/* vim: set sts=4 sw=4: */
#ifndef __IMPL_H__
#define __IMPL_H__

#include <memory>

namespace coh {

    using std::shared_ptr;

#define USE_IMPL(cname) \
    public: \
        class Impl; \
    private: \
	shared_ptr<Impl> mImpl; \
    public: \
	inline bool operator==(cname const& rhs) const \
	    { return mImpl == rhs.mImpl; } \
	inline operator bool() const { return !!mImpl; }

}

#endif
