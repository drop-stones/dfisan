#include "sanitizer_common/sanitizer_common.h"
#include "sanitizer_common/sanitizer_internal_defs.h"

//#include <initializer_list>

using namespace __sanitizer;

//extern "C" SANITIZER_INTERFACE_ATTRIBUTE
void __dfisan_store_id(u32 *StoreAddr, u32 DefID) {
  Report("INFO: Set DefID(%d) at %p\n", DefID, (void *)StoreAddr);
}

//extern "C" SANITIZER_INTERFACE_ATTRIBUTE
template<typename T, typename... Args>
void __dfisan_check_ids(u32 *LoadAddr, T t, Args... args) {
  Report("INFO: Check SetID at %p\n", LoadAddr);
}