#include <cstddef>
#include <cstdint>
#include <cstdio>

typedef union alignas(16) header {
    struct {
        size_t size;       // 8 bytes
        uint32_t is_free;  // 4 bytes
        uint32_t __pad;    // 4 bytes (explicit padding)
        // No 'next' pointer (sacrifice list for size)
    } s;
    char stub[16];
} header_t;

int main() {
    printf("%d", sizeof(header_t));
}
