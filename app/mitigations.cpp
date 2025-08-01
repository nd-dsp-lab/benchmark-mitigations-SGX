// mitigations.cpp

#include "mitigations.h"
#include <string.h>

MitigationConfig g_enclave_config;

void set_enclave_config(const MitigationConfig* config) {
    if (config) {
        g_enclave_config = *config;
    }
}

namespace mitigations {
    const size_t CACHE_LINE_SIZE = 64;

    void lfence_barrier() {
        if (g_enclave_config.lfence_barrier) {
            __asm__ volatile ("lfence" ::: "memory");
        }
    }

    void mfence_barrier() {
        if (g_enclave_config.mfence_barrier) {
            __asm__ volatile ("mfence" ::: "memory");
        }
    }

    void cache_flush(const void* addr, size_t size) {
        if (!g_enclave_config.cache_flushing) return;
        char* ptr = const_cast<char*>(static_cast<const char*>(addr));
        for (size_t i = 0; i < size; i += CACHE_LINE_SIZE) {
            __asm__ volatile ("clflush %0" : "+m" (*(ptr + i)));
        }
        __asm__ volatile ("mfence" ::: "memory");
    }

    void memory_barrier() {
        if (!g_enclave_config.memory_barriers) return;
        __asm__ volatile ("mfence" ::: "memory");
    }

    void constant_time_memcpy(void* dest, const void* src, size_t n) {
        if (!g_enclave_config.constant_time_ops) {
            memcpy(dest, src, n);
            return;
        }
        volatile unsigned char* d = static_cast<volatile unsigned char*>(dest);
        const volatile unsigned char* s = static_cast<const volatile unsigned char*>(src);
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }

    void secure_memzero(void* ptr, size_t len) {
        if (!g_enclave_config.constant_time_ops) {
            memset(ptr, 0, len);
            return;
        }
        cache_flush(ptr, len);
        volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
        for (size_t i = 0; i < len; i++) {
            p[i] = 0;
        }
        cache_flush(ptr, len);
    }
}