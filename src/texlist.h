#pragma once

#include <stdint.h>
#include <raylib.h>

typedef enum {
    TEXTURE_FLAG_NONE = 0x00000000,
    TEXTURE_FLAG_DYNAMIC = 0x01000000,
} texture_flags_t;

typedef struct {
    char *name;
    texture_flags_t flags;
    Vector2 topLeft;
    Vector2 botRight;

    uint16_t width;
    uint16_t height;

    uint16_t atlasId;
} texlist_entry_t;

typedef struct {
    texlist_entry_t *textures;
} texlist_t;

texlist_t* texlist_load(const char *filename);
void texlist_free(texlist_t *texlist);
