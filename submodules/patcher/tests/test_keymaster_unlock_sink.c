#include "../patchlib.h"

static void put32(CHAR8* buf, INT32 off, UINT32 raw) {
    buf[off] = (CHAR8)(raw & 0xff);
    buf[off + 1] = (CHAR8)((raw >> 8) & 0xff);
    buf[off + 2] = (CHAR8)((raw >> 16) & 0xff);
    buf[off + 3] = (CHAR8)((raw >> 24) & 0xff);
}

int main(void) {
    CHAR8 buf[0x200] = {0};
    const INT32 anchor = 0x40;
    const INT32 sink = 0xc8;

    put32(buf, sink, 0x390183eb);
    put32(buf, sink + 0x0c, 0x290ca7e8);

    if (patch_keymaster_unlock_sink(buf, sizeof(buf), anchor) != 1)
        return 1;

    if (read_instr(buf, sink) != 0x390183ff)
        return 2;

    return 0;
}
