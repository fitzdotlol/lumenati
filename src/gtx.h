#pragma once

#include <stdio.h>
#include <stdint.h>
#include <raylib.h>

#define PIXELFORMAT_COMPRESSED_RED_RGTC1_EXT              (0x8DBB)
#define PIXELFORMAT_COMPRESSED_SIGNED_RED_RGTC1_EXT       (0x8DBC)
#define PIXELFORMAT_COMPRESSED_RED_GREEN_RGTC2_EXT        (0x8DBD)
#define PIXELFORMAT_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT (0x8DBE)

int getFormatBPP (PixelFormat format);
int getPixelIndex (int x, int y, int bpp);
int surfaceAddrFromCoordMacroTiled(int x, int y, int bpp, int pitch, int swizzle);
uint8_t* gtxSwizzle(uint8_t *data, size_t dataSize, int width, int height, PixelFormat format, int pitch, int swizzleIn);
