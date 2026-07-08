#include "recomp.h"
#include "ultramodern/ultramodern.hpp"

extern "C" void __osPfsSelectBank_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 11;
}

extern "C" void __osContRamRead_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 11;
}

extern "C" void __osContRamWrite_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 11;
}

extern "C" void __osViGetCurrentContext_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __osViSwapContext_recomp(uint8_t* rdram, recomp_context* ctx) {
}

extern "C" void __osTimerInterrupt_recomp(uint8_t* rdram, recomp_context* ctx) {
}

extern "C" void __osGetCause_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __rmonRCPrunning_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __rmonReadWordAt_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __rmonWriteWordTo_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __rmonStepRCP_recomp(uint8_t* rdram, recomp_context* ctx) {
}

extern "C" void __rmonGetThreadStatus_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0;
}

extern "C" void __rmonSendReply_recomp(uint8_t* rdram, recomp_context* ctx) {
}

extern "C" void __rmonSendFault_recomp(uint8_t* rdram, recomp_context* ctx) {
}
