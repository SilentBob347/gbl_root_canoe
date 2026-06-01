#include "../patchlib.h"

static void put32(CHAR8* buf, INT32 off, UINT32 raw) {
    buf[off] = (CHAR8)(raw & 0xff);
    buf[off + 1] = (CHAR8)((raw >> 8) & 0xff);
    buf[off + 2] = (CHAR8)((raw >> 16) & 0xff);
    buf[off + 3] = (CHAR8)((raw >> 24) & 0xff);
}

static UINT32 encode_add_x_imm(UINT8 rd, UINT8 rn, UINT32 imm) {
    return 0x91000000u | ((imm & 0xfffu) << 10) | ((UINT32)rn << 5) | rd;
}

static UINT32 encode_ldr_x_imm(UINT8 rt, UINT8 rn, UINT32 imm) {
    return 0xF9400000u | (((imm >> 3) & 0xfffu) << 10) | ((UINT32)rn << 5) | rt;
}

static UINT32 encode_adrp(UINT8 rd, INT32 pc_va, INT32 target_va) {
    INT32 imm = ((target_va & ~0xfff) - (pc_va & ~0xfff)) >> 12;
    UINT32 imm21 = (UINT32)imm & 0x1fffffu;
    return 0x90000000u | ((imm21 & 3u) << 29) | (((imm21 >> 2) & 0x7ffffu) << 5) | rd;
}

static UINT32 encode_bl(INT32 from, INT32 to) {
    INT32 imm26 = (to - from) >> 2;
    return 0x94000000u | ((UINT32)imm26 & 0x03ffffffu);
}

static void make_prefixed_pe(CHAR8* buf, INT32 delta) {
    buf[delta] = 'M';
    buf[delta + 1] = 'Z';
    put32(buf, delta + 0x3c, 0x40);
    buf[delta + 0x40] = 'P';
    buf[delta + 0x41] = 'E';
}

static void put_cstr(CHAR8* buf, INT32 off, const CHAR8* s) {
    while (*s) buf[off++] = *s++;
    buf[off] = 0;
}

int main(void) {
    const INT32 delta = 0x80;
    const INT32 key_va = 0x3000;
    const INT32 lit_va = 0x3100;
    const INT32 code_va = 0x1200;
    const INT32 key_off = key_va + delta;
    const INT32 lit_off = lit_va + delta;
    const INT32 code_off = code_va + delta;

    CHAR8 pcba[0x5000] = {0};
    make_prefixed_pe(pcba, delta);
    put_cstr(pcba, key_off, " androidboot.pcbaidinfo=");
    put_cstr(pcba, lit_off - 1, "");
    put_cstr(pcba, lit_off, "ROW");
    put32(pcba, code_off, encode_adrp(1, code_va, key_va));
    put32(pcba, code_off + 4, encode_add_x_imm(1, 1, 0));
    put32(pcba, code_off + 8, encode_add_x_imm(2, 31, 0x590));

    if (patch_pcbaidinfo_override(pcba, sizeof(pcba), "ROW") != 1)
        return 1;
    if (read_instr(pcba, code_off + 8) == encode_add_x_imm(2, 31, 0x590))
        return 2;

    CHAR8 hq[0x5000] = {0};
    make_prefixed_pe(hq, delta);
    put_cstr(hq, key_off, " hqsysfs.pcba_config=");
    put_cstr(hq, lit_off - 1, "");
    put_cstr(hq, lit_off, "ROW");
    put32(hq, code_off, encode_adrp(2, code_va, key_va));
    put32(hq, code_off + 4, encode_add_x_imm(2, 2, 0));
    put32(hq, code_off + 8, encode_bl(code_va + 8, 0x2000));
    put32(hq, code_off + 12, encode_ldr_x_imm(2, 31, 0x250));
    put32(hq, code_off + 16, encode_bl(code_va + 16, 0x2000));

    if (patch_hqsysfs_pcba_config_override(hq, sizeof(hq), "ROW") != 1)
        return 3;
    if (read_instr(hq, code_off + 12) == encode_ldr_x_imm(2, 31, 0x250))
        return 4;

    return 0;
}
