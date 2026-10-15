/* FlashVM: identity keys for loaded InstanceOf graph traversal. */
#ifndef FVM_INSTANCE_OF_VISIT_HPP
#define FVM_INSTANCE_OF_VISIT_HPP /* Private visited-object identity guard. */
#include "support.hpp"
#include <cstddef>
namespace fvm_instance_of_detail
{
/* Wrap pointer identity so argument-dependent lookup selects its native hash. */
struct ObjectKey
{
    FVM_Object *object; /* [GC:] Borrowed node retained by the operation's rooted input graph. */
    /* Preserve object identity independently of user properties and callbacks. */
    explicit ObjectKey(FVM_Object *value) : object(value) {}
    /* Compare complete pointer values, including bits omitted from the bucket hash. */
    bool operator==(const ObjectKey &other) const { return object == other.object; }
};
/* Hash native low address bits; full pointer equality resolves LLP64 truncation collisions. */
inline unsigned long hash_key(const ObjectKey &key)
{
    /* Requested native ports all provide a size_t at least as wide as their object pointers. */
    typedef char PointerFitsSize[(sizeof(size_t) >= sizeof(FVM_Object *)) ? 1 : -1];
    (void)sizeof(PointerFitsSize);
    return fvm_detail::hash_key((unsigned long)reinterpret_cast<size_t>(key.object));
}
}
#endif
