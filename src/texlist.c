#include <assert.h>
#include <raylib.h>

#include "stb_ds.h"
#include "texlist.h"
#include "file.h"

texlist_t* texlist_load(const char *filename)
{
    texlist_t *texlist = (texlist_t*)MemAlloc(sizeof(*texlist));

    int dataSize;
    uint8_t *data_ = LoadFileData(filename, &dataSize);

    filereader_t file = {
        .data = data_,
        .ptr = 0,
        .endian = ENDIAN_BIG
    };

    int32_t _magic = file_readInt32(&file);
    uint16_t _unk04 = file_readUInt16(&file);

    uint16_t numAtlases = file_readInt16(&file);
    uint16_t numTextures = file_readUInt16(&file);
    uint16_t flagsOffset = file_readUInt16(&file);
    uint16_t texturesOffset = file_readUInt16(&file);
    uint16_t stringsOffset = file_readUInt16(&file);

    file.ptr = texturesOffset;
    for (int i = 0; i < numTextures; ++i) {
        texlist_entry_t tex = { 0 };

        int32_t nameOffset = file_readInt32(&file);
        int32_t nameOffset2 = file_readInt32(&file);

        // NOTE: I have yet to see this and would like to.
        // do not just remove this if you find it, please help documentation.
        if (nameOffset != nameOffset2) {
            assert("texlist name offsets don't match?");
        }

        tex.name = strdup((char*)file.data + stringsOffset + nameOffset);

        tex.topLeft = (Vector2) { file_readFloat(&file), file_readFloat(&file) };
        tex.botRight = (Vector2) { file_readFloat(&file), file_readFloat(&file) };
        tex.width = file_readUInt16(&file);
        tex.height = file_readUInt16(&file);
        tex.atlasId = file_readUInt16(&file);

        // NOTE: padding
        file_skip(&file, 0x02);

        stbds_arrput(texlist->textures, tex);
    }

    // MAYBE FIXME: this could be done in the previous loop.
    // would require either:
    //    - temporarily changing the file ptr
    //    - adding file methods for reading at specific offset
    // or - read manually without file methods. (please no)
    file.ptr = flagsOffset;
    for (int i = 0; i < numTextures; ++i) {
        texlist->textures[i].flags = (texture_flags_t)file_readInt32(&file);
    }

    MemFree(data_);

    return texlist;
}

void texlist_free(texlist_t *texlist)
{
    if (!texlist)
        return;

    size_t numTextures = stbds_arrlenu(texlist->textures);
    for (int i = 0; i < numTextures; ++i) {
        MemFree(texlist->textures[i].name);
    }

    stbds_arrfree(texlist->textures);
    MemFree(texlist);
}
