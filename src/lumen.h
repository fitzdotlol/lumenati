#pragma once

#include <raylib.h>
#include <stdint.h>

typedef enum {
    CHUNK_TAG_INVALID = 0x0000,

    CHUNK_TAG_FONTS = 0x000A,

    CHUNK_TAG_STRINGS = 0xF001,
    CHUNK_TAG_COLORS = 0xF002,
    CHUNK_TAG_TRANSFORMS = 0xF003,
    CHUNK_TAG_POSITIONS = 0xF103,
    CHUNK_TAG_BOUNDS = 0xF004,
    CHUNK_TAG_ACTIONSCRIPT = 0xF005,
    CHUNK_TAG_ACTIONSCRIPT2 = 0xFF05,
    CHUNK_TAG_TEXTURE_ATLASES = 0xF007,
    CHUNK_TAG_UNK_F008 = 0xF008,
    CHUNK_TAG_UNK_F009 = 0xF009,
    CHUNK_TAG_UNK_F00A = 0xF00A,
    CHUNK_TAG_UNK_F00B = 0xF00B,
    CHUNK_TAG_PROPERTIES = 0xF00C,
    CHUNK_TAG_DEFINES = 0xF00D,

    CHUNK_TAG_SHAPE = 0xF022,
    CHUNK_TAG_GRAPHIC = 0xF024,
    CHUNK_TAG_COLOR_MATRIX = 0xF037,

    CHUNK_TAG_DYNAMIC_TEXT = 0x0025,
    CHUNK_TAG_DEFINE_SPRITE = 0x0027,
    CHUNK_TAG_FRAME_LABEL = 0x002B,
    CHUNK_TAG_SHOWFRAME = 0x0001,
    CHUNK_TAG_KEYFRAME = 0xF105,
    CHUNK_TAG_PLACE_OBJECT = 0x0004,
    CHUNK_TAG_REMOVE_OBJECT = 0x0005,
    CHUNK_TAG_DO_ACTION = 0x000C,

    CHUNK_TAG_END = 0xFF00
} chunk_tag_t;

typedef enum {
    PLACE_FLAG_PLACE = 0x01,
    PLACE_FLAG_MOVE = 0x02
} place_flag_t;


// NOTE: prefixed to avoid collision with raylib
typedef enum {
    LM_BLEND_NORMAL = 0x00,
    LM_BLEND_LAYER = 0x02,
    LM_BLEND_MULTIPLY = 0x03,
    LM_BLEND_SCREEN = 0x04,
    LM_BLEND_LIGHTEN = 0x05,
    LM_BLEND_DARKEN = 0x06,
    LM_BLEND_DIFFERENCE = 0x07,
    LM_BLEND_ADD = 0x08,
    LM_BLEND_SUBTRACT = 0x09,
    LM_BLEND_INVERT = 0x0A,
    LM_BLEND_ALPHA = 0x0B,
    LM_BLEND_ERASE = 0x0C,
    LM_BLEND_OVERLAY = 0x0D,
    LM_BLEND_HARDLIGHT = 0x0E
} blend_mode_t;

typedef enum {
    SOLID = 0x00,
    LINEAR_GRADIENT = 0x10,
    RADIAL_GRADIENT = 0x12,
    FOCAL_RADIAL_GRADIENT = 0x13,
    REPEATING_BITMAP = 0x40,
    CLIPPED_BITMAP = 0x41,
    NONSMOOTHED_REPEATING_BITMAP = 0x42,
    NONSMOOTHED_CLIPPED_BITMAP = 0x43
} fill_type_t;

#pragma pack(push, 1)
typedef struct {
    int32_t magic;
    int32_t unk0;
    int32_t unk1;
    int32_t unk2;
    int32_t unk3;
    int32_t unk4;
    int32_t unk5;
    int32_t filesize;
    int32_t unk6;
    int32_t unk7;
    int32_t unk8;
    int32_t unk9;
    int32_t unk10;
    int32_t unk11;
    int32_t unk12;
    int32_t unk13;
} header_t;

typedef struct {
    uint32_t unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t maxCharacterId;
    int32_t unk4;
    uint32_t maxCharacterId2;
    uint16_t maxDepth;
    uint16_t unk7;
    float framerate;
    float width;
    float height;
    uint32_t unk8;
    uint32_t unk9;
} properties_t;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} color_t;

typedef struct {
    float x1;
    float y1;
    float x2;
    float y2;
} bounds_t;

typedef struct {
    int32_t id;
    int32_t unk;
    float width;
    float height;

    Texture texture;
} atlas_t;

typedef struct {
    int32_t atlasId;
    int16_t fillType;

    Vector4 *verts;
    int16_t *indices;

    Mesh mesh;
} graphic_t;

typedef struct {
    int32_t id;
    int32_t unk1;
    int32_t boundsId;
    int32_t unk2;

    graphic_t *graphics;
} shape_t;

typedef struct {
    int32_t nameId;
    int32_t startFrame;
    int32_t unk1;
} label_t;

typedef struct {
    int32_t unk1;
    int16_t spriteObjectId; // or was it placement id?
    int16_t unk2;
} deletion_t;

typedef struct {
    int32_t actionId;
    int32_t unk1;
} action_t;

typedef struct {

} unk_f014_t;

typedef struct {
    float data[20];
} color_matrix_t;

typedef struct {
    int32_t characterId;
    int32_t placementId;
    int32_t unk1;
    int32_t nameId;
    int16_t flags;
    int16_t blendMode;
    int16_t depth;
    int16_t unk4;
    int16_t unk5;
    int16_t unk6;
    int16_t positionFlags;
    int16_t positionId;
    int32_t colorMultId;
    int32_t colorAddId;
    
    int32_t hasColorMatrix;
    int32_t hasUnkF014;

    //
    color_matrix_t colorMatrix;
    unk_f014_t unkF014;
} placement_t;

typedef struct {
    int32_t id;

    //
    label_t *label;

    //
    deletion_t *deletions;
    action_t *actions;
    placement_t *placements;
} frame_t;

typedef struct {
    int32_t id;
    int32_t unk1;
    int32_t unk2;
    int32_t unk3;

    //
    label_t *labels;
    frame_t *frames;
    frame_t *keyframes;
} sprite_t;

typedef struct {
    header_t header;
    char **strings;
    color_t *colors;
    Matrix *transforms;
    Vector2 *positions;
    bounds_t *bounds;
    atlas_t *atlases;
    shape_t *shapes;
    sprite_t *sprites;
} lumen_document_t;

lumen_document_t* lumen_document_load(const char *filename);

#pragma pack(pop)
