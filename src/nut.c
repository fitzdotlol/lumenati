#include <assert.h>
#include <raylib.h>

#include "nut.h"
#include "gtx.h"

PixelFormat getPixelFormatFromNutFormat(int typet)
{
    switch (typet)
    {
        case 0x0:
            return PIXELFORMAT_COMPRESSED_DXT1_RGBA;
        case 0x1:
            return PIXELFORMAT_COMPRESSED_DXT3_RGBA;
        case 0x2:
            return PIXELFORMAT_COMPRESSED_DXT5_RGBA;
        case 14:
            // utype = PixelFormat.Rgba;
            return PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        case 17:
            // utype = PixelFormat.Bgra;
            return PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        case 21:
            return PIXELFORMAT_COMPRESSED_DXT1_RGBA;
            // return PIXELFORMAT_COMPRESSED_RED_RGTC1_EXT;
        case 22:
            return PIXELFORMAT_COMPRESSED_DXT5_RGBA;
            // return PIXELFORMAT_COMPRESSED_RED_GREEN_RGTC2_EXT;
    }

    return 0;
}

Image readNTP3(filereader_t *file)
{
    file_skip(file, 0x14);

    int32_t totalSize = file_readInt32(file);
    int16_t headerSize = file_readInt16(file);
    
    int16_t numMips = file_readInt32(file);
    int16_t nutFormat = file_readInt16(file);
    int16_t width = file_readInt16(file);
    int16_t height = file_readInt16(file);

    file_skip(file, 0x08);
    int dataOffset = file_readInt32(file) + 0x10;

    file_skip(file, headerSize - 0x50 + 0x24);
    int32_t id = file_readInt32(file);

    Image image = {
        file->data+dataOffset,
        width, height,
        1,
        getPixelFormatFromNutFormat(nutFormat)
    };

    return image;
}

Image readNTWU(filereader_t *file)
{
    file_skip(file, 0x18);

    int16_t headerSize = file_readInt16(file);
    int32_t numMips = file_readInt32(file);
    int16_t nutFormat = file_readInt16(file);
    int16_t width = file_readInt16(file);
    int16_t height = file_readInt16(file);

    file_skip(file, 8); // mipmaps and padding
    int dataOffset = file_readInt32(file) + 0x10;

    file_skip(file, 4);
    int gtxHeaderOffset = file_readInt32(file) + 0x10;

    file_skip(file, 4);
    file_skip(file, headerSize - 0x50);

    file_skip(file, 0x18);
    int32_t id = file_readInt32(file);

    // buf.ptr = (uint)gtxHeaderOffset;
    file->ptr = gtxHeaderOffset;
    file_skip(file, 4); // dim
    file_skip(file, 4); // width
    file_skip(file, 4); // height
    file_skip(file, 4); // depth
    file_skip(file, 4); // numMips
    int format = file_readInt32(file);
    file_skip(file, 4); // aa
    file_skip(file, 4); // use
    int imageSize = file_readInt32(file);
    file_skip(file, 4); // imagePtr
    file_skip(file, 4); // mipSize
    file_skip(file, 4); // mipPtr
    file_skip(file, 4); // tileMode
    int swizzle = file_readInt32(file);
    file_skip(file, 4); // alignment
    int pitch = file_readInt32(file);

    //// ------
    uint8_t *swizzledData = gtxSwizzle(
        file->data+dataOffset, imageSize,
        width, height,
        getPixelFormatFromNutFormat(nutFormat),
        pitch,
        swizzle
    );

    Image image = {
        swizzledData,
        width, height,
        1,
        getPixelFormatFromNutFormat(nutFormat)
    };

    return image;
}

Texture LoadNUTTexture(const char *filename)
{
    Image image;

    int dataSize;
    uint8_t *data_ = LoadFileData(filename, &dataSize);
    filereader_t file = {
        .data = data_,
        .ptr = 0,
        .endian = ENDIAN_BIG
    };

    uint32_t magic = file_readInt32(&file);

    if (magic == 0x4E545033) {
        image = readNTP3(&file);
    } else if (magic == 0x4E545755) {
        image = readNTWU(&file);
    } else {
        fprintf(stderr, "unhandled nut header (magic = 0x%08X)\n", magic);
        assert(false);
    }

    Texture tex = LoadTextureFromImage(image);

    MemFree(data_);

    return tex;
}
