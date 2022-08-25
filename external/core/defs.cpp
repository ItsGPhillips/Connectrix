#include "defs.h"

#if !defined(NDEBUG)
#include "debug_break.h"
[[ noreturn ]]  void debug_abort() {
	debug_break;
	std::abort();
}
#else
[[ noreturn ]] void debug_abort() {
	std::abort();
}
#endif