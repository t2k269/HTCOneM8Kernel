#ifdef CONFIG_KMEMCHECK
# include <asm-generic/xor.h>
#else
#ifdef CONFIG_X86_32
# include "xor_32.h"
#else
# include "xor_64.h"
#endif
#endif
