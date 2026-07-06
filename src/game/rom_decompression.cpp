#include "harvestmoon64_game.h"

void harvestmoon64::game_on_init(uint8_t* rdram, recomp_context* ctx) {
    // User Task: Add rom decompression routine if it needs it. If the game doesn't need it, you can just leave this
    // function empty.
    (void)rdram;
    (void)ctx;
}

// Banjo's example:
/*
#include <cassert>
#include <cstring>
#include <fstream>

#include "miniz.h"

#include "librecomp/game.hpp"
#include "banjo_game.h"

#ifdef _MSC_VER
inline uint32_t byteswap(uint32_t val) {
    return _byteswap_ulong(val);
}
#else
constexpr uint32_t byteswap(uint32_t val) {
    return __builtin_bswap32(val);
}
#endif

size_t decompress_bkzip(mz_stream* stream, std::span<const uint8_t> compressed_rom, uint32_t start, uint32_t end,
                        std::vector<uint8_t>& out, size_t out_offset) {
    // Subtract 2 bytes of magic number and 4 bytes of size.
    uint32_t compressed_data_start = start + 0x6;

    uint8_t size0 = compressed_rom[start + 0x2 + 0x0];
    uint8_t size1 = compressed_rom[start + 0x2 + 0x1];
    uint8_t size2 = compressed_rom[start + 0x2 + 0x2];
    uint8_t size3 = compressed_rom[start + 0x2 + 0x3];
    size_t decompressed_size = (size0 << 24) | (size1 << 16) | (size2 << 8) | (size3 << 0);

    if (out.size() < decompressed_size + out_offset) {
        out.resize(decompressed_size + out_offset);
    }

    stream->avail_in = end - compressed_data_start;
    stream->next_in = reinterpret_cast<const Bytef*>(compressed_rom.data() + compressed_data_start);

    stream->avail_out = decompressed_size;
    stream->next_out = reinterpret_cast<Bytef*>(out.data() + out_offset);

    mz_inflate(stream, Z_NO_FLUSH);

    mz_inflateReset(stream);

    return decompressed_size;
}

// Produces a decompressed BK rom. This is only needed because the game has compressed code.
// For other recomps using this repo as an example, you can omit the decompression routine and
// set the corresponding fields in the GameEntry if the game doesn't have compressed code,
// even if it does have compressed data.
std::vector<uint8_t> banjo::decompress_bk(std::span<const uint8_t> compressed_rom) {
    // Sanity check the rom size and header. These should already be correct from the runtime's check,
    // but it should prevent this file from accidentally being copied to another recomp.
    if (compressed_rom.size() != 0x1000000) {
        assert(false);
        return {};
    }

    if (compressed_rom[0x3B] != 'N' || compressed_rom[0x3C] != 'B' || compressed_rom[0x3D] != 'K' ||
        compressed_rom[0x3E] != 'E') {
        assert(false);
        return {};
    }

    struct Overlay {
        uint32_t text_start;
        uint32_t data_start;
    };

    Overlay overlays[] = {
        { .text_start = 0xF19250, .data_start = 0xF19250 + 0x1D09B },
        { .text_start = 0xF37F90, .data_start = 0xF37F90 + 0x64B50 },
        { .text_start = 0xFA3FD0, .data_start = 0xFA3FD0 + 0x1DC6 },
        { .text_start = 0xFA5F50, .data_start = 0xFA5F50 + 0x2D96 },
        { .text_start = 0xFA9150, .data_start = 0xFA9150 + 0x512E },
        { .text_start = 0xFAE860, .data_start = 0xFAE860 + 0x328B },
        { .text_start = 0xFB24A0, .data_start = 0xFB24A0 + 0x1E39 },
        { .text_start = 0xFB44E0, .data_start = 0xFB44E0 + 0x5130 },
        { .text_start = 0xFB9A30, .data_start = 0xFB9A30 + 0x4BB2 },
        { .text_start = 0xFBEBE0, .data_start = 0xFBEBE0 + 0x540F },
        { .text_start = 0xFC4810, .data_start = 0xFC4810 + 0x23FF },
        { .text_start = 0xFC6F20, .data_start = 0xFC6F20 + 0x1BDC },
        { .text_start = 0xFC9150, .data_start = 0xFC9150 + 0x6548 },
        { .text_start = 0xFD0420, .data_start = 0xFD0420 + 0x5640 },
        { .text_start = 0xFD6190, .data_start = 0xFD6190 + 0x416F },
        { .text_start = 0xFDAA10, .data_start = 0xFDAA10 + 0xE },
    };
    const uint32_t overlays_end = 0xFDAA30;

    // Swap the overlay order from the compressed ROM to match the decompressed ROM order.
    std::swap(overlays[3], overlays[4]);

    mz_stream stream{};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    mz_inflateInit2(&stream, -MAX_WBITS);

    std::vector<uint8_t> ret{};

    // Copy everything from the original ROM up until the first overlay into the decompressed ROM.
    ret.reserve(0x2000000);
    ret.assign(compressed_rom.begin(), compressed_rom.begin() + overlays[0].text_start);

    size_t cur_size = overlays[0].text_start;

    for (size_t overlay_index = 0; overlay_index < std::size(overlays); overlay_index++) {
        uint32_t text_start = overlays[overlay_index].text_start;
        uint32_t data_start = overlays[overlay_index].data_start;
        uint32_t text_end = data_start;
        uint32_t data_end =
            overlay_index == (std::size(overlays) - 1) ? overlays_end : overlays[overlay_index + 1].text_start;

        // Decompress .text
        cur_size += decompress_bkzip(&stream, compressed_rom, text_start, text_end, ret, cur_size);
        cur_size = (cur_size + 15ULL) & ~15ULL;

        // Decompress .data and .rodata
        cur_size += decompress_bkzip(&stream, compressed_rom, data_start, data_end, ret, cur_size);
        cur_size = (cur_size + 15ULL) & ~15ULL;
    }

    mz_inflateReset(&stream);

    return ret;
}

void banjo::bk_on_init(uint8_t* rdram, recomp_context* ctx) {
    MEM_W(0, (int32_t) 0x80000310) = 6103;
    recomp::do_rom_read(rdram, (int32_t) 0x80000000, 0x100004C0, 0x2A4);
}
*/