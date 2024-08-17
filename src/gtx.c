#include <assert.h>
#include <string.h>

#include "gtx.h"

int getFormatBPP (PixelFormat format)
{
    switch (format)
    {
        case PIXELFORMAT_COMPRESSED_DXT3_RGBA:
        case PIXELFORMAT_COMPRESSED_DXT5_RGBA:
        case PIXELFORMAT_COMPRESSED_RED_GREEN_RGTC2_EXT:
            return 0x80;
        case PIXELFORMAT_COMPRESSED_DXT1_RGBA:
        case PIXELFORMAT_COMPRESSED_RED_RGTC1_EXT:
            return 0x40;
        case PIXELFORMAT_UNCOMPRESSED_R8G8B8A8:
            return 0x20;
        
        default:
            fprintf(stderr, "[getFormatBPP] Invalid format (0x%04X)\n", format);
            assert(false);
    }

    return 0;
}

int getPixelIndex (int x, int y, int bpp)
{
    if (bpp == 0x80)
        return ((x & 7) << 1) | ((y & 6) << 3) | (y & 1);
    else if (bpp == 0x40)
        return ((x & 6) << 1) | (x & 1) | ((y & 6) << 3) | ((y & 1) << 1);
    else if (bpp == 0x20)
        return ((x & 4) << 1) | (x & 3) | ((y & 6) << 3) | ((y & 1) << 2);

    return 0;
}

int surfaceAddrFromCoordMacroTiled(int x, int y, int bpp, int pitch, int swizzle)
{
    int pipe = ((y ^ x) >> 3) & 1;
    int bank = (((y / 32) ^ (x >> 3)) & 1) | ((y ^ x) & 16) >> 3;
    int bankPipe = ((pipe + bank * 2) ^ swizzle) % 9;
    int macroTileBytes = (bpp * 512 + 7) >> 3;
    int macroTileOffset = (x / 32 + pitch / 32 * (y / 16)) * macroTileBytes;
    int unk = (bpp * getPixelIndex(x, y, bpp) + macroTileOffset) >> 3;

    return (bankPipe << 8) | ((bankPipe % 2) << 8) | ((unk & ~0xFF) << 3) | (unk & 0xFF);
}

uint8_t* gtxSwizzle(uint8_t *data, size_t dataSize, int width, int height, PixelFormat format, int pitch, int swizzleIn)
{
    uint8_t *swizzledData = MemAlloc(dataSize);

    int swizzle = ((swizzleIn & 0x100) + (swizzleIn & 0x600)) >> 8;
    int bpp = getFormatBPP(format);
    int blockSize = bpp / 8;

    if (format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
    {
        width /= 4;
        height /= 4;
    }

    for (int i = 0; i < width*height; ++i) {
        int pos = surfaceAddrFromCoordMacroTiled(i % width, i / width, bpp, pitch, swizzle);
        memcpy(swizzledData+(i*blockSize), data+pos, blockSize);
    }

    return swizzledData;
}
