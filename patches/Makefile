TARGET = patches.elf

CC      ?= clang
LD      ?= ld.lld

CFLAGS   := -target mips -mips2 -mabi=32 -O2 -G0 -mno-abicalls -mno-odd-spreg -mno-check-zero-division \
			-fomit-frame-pointer -ffast-math -fno-unsafe-math-optimizations -fno-builtin-memset \
			-Wall -Wextra -Wno-incompatible-library-redeclaration -Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-but-set-variable -Wno-missing-braces -Wno-unsupported-floating-point-opt -Wno-switch
CPPFLAGS := -nostdinc -DF3DEX_GBI_2 -D_LANGUAGE_C -DMIPS -I dummy_headers -I ../lib/hm64-decomp -I ../lib/hm64-decomp/src -I ../lib/hm64-decomp/include/ -I ../lib/hm64-decomp/lib/libmus/include/PR/ -I ../lib/hm64-decomp/lib/ -I ../lib/hm64-decomp/lib/libultra/include/ -I ../lib/hm64-decomp/lib/nusys-1/include -I ../lib/hm64-decomp/lib/libultra/include -I ../lib/hm64-decomp/lib/libultra/include/PR -I ../lib/rt64/include
LDFLAGS  := -nostdlib -T patches.ld -T syms.ld -Map patches.map --unresolved-symbols=ignore-all --emit-relocs

C_SRCS := $(wildcard *.c)
C_OBJS := $(C_SRCS:.c=.o)
C_DEPS := $(C_SRCS:.c=.d)

$(TARGET): $(C_OBJS) patches.ld syms.ld
	$(LD) $(C_OBJS) $(LDFLAGS) -o $@

$(C_OBJS): %.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -MMD -MF $(@:.o=.d) -c -o $@

clean:
	rm -rf $(C_OBJS) $(TARGET) $(C_DEPS)

-include $(C_DEPS)

.PHONY: clean
