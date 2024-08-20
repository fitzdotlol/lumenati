#include <assert.h>
#include <raylib.h>
#include "stb_ds.h"
#include "lumen.h"
#include "file.h"
#include "nut.h"

Texture loadLmTexture(int idx)
{
    const char *filename = TextFormat("data/chara/img-%05d.nut", idx);

    if (FileExists(filename)) {
        Texture tex = LoadNUTTexture(filename);
        return tex;
    }

    Texture fallbackTex = LoadTexture("data/fallback.png");
    return fallbackTex;
}

Mesh GenMeshGraphic(graphic_t *graphic)
{
    Mesh mesh = { 0 };

    size_t numIndices = stbds_arrlenu(graphic->indices);

    mesh.vertexCount = stbds_arrlenu(graphic->verts);
    mesh.triangleCount = numIndices / 3;
    mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount*2*sizeof(float));
    mesh.normals = (float *)RL_MALLOC(mesh.vertexCount*3*sizeof(float));
    mesh.indices = (unsigned short *)RL_MALLOC(numIndices*sizeof(unsigned short));

    // Mesh vertices position array
    for (int i = 0; i < mesh.vertexCount; ++i)
    {
        mesh.vertices[3*i] = graphic->verts[i].x;
        mesh.vertices[3*i + 1] = graphic->verts[i].y;
        mesh.vertices[3*i + 2] = 1;

        mesh.texcoords[2*i] = graphic->verts[i].z;
        mesh.texcoords[2*i + 1] = graphic->verts[i].w;

        mesh.normals[3*i] = 0;
        mesh.normals[3*i + 1] = 0;
        mesh.normals[3*i + 2] = 0;
    }

    // Mesh indices array initialization
    for (int i = 0; i < numIndices; ++i)
        mesh.indices[i] = graphic->indices[i];

    // Upload vertex data to GPU (static mesh)
    UploadMesh(&mesh, false);

    return mesh;
}

lumen_document_t* lumen_document_load(const char *filename)
{
    lumen_document_t *doc = (lumen_document_t*)MemAlloc(sizeof(*doc));


    int dataSize;
    // size_t ptr = 0;
    uint8_t *data_ = LoadFileData(filename, &dataSize);

    filereader_t file = {
        .data = data_,
        .ptr = 0,
        .endian = ENDIAN_BIG
    };

    doc->header.magic = file_readInt32(&file);
    doc->header.unk0 = file_readInt32(&file);

    if (doc->header.unk0 == 0x10000000)
    {
        fprintf(stderr, "Little endian not yet supported :(\n");
        return NULL;
    }

    doc->header.unk1 = file_readInt32(&file);
    doc->header.unk2 = file_readInt32(&file);
    doc->header.unk3 = file_readInt32(&file);
    doc->header.unk4 = file_readInt32(&file);
    doc->header.unk5 = file_readInt32(&file);
    doc->header.filesize = file_readInt32(&file);
    doc->header.unk6 = file_readInt32(&file);
    doc->header.unk7 = file_readInt32(&file);
    doc->header.unk8 = file_readInt32(&file);
    doc->header.unk9 = file_readInt32(&file);
    doc->header.unk10 = file_readInt32(&file);
    doc->header.unk11 = file_readInt32(&file);
    doc->header.unk12 = file_readInt32(&file);
    doc->header.unk13 = file_readInt32(&file);

    bool done = false;
    while (!done) {
        size_t tagOffset = file.ptr;
        chunk_tag_t tagType = file_readInt32(&file);
        int32_t tagSize = file_readInt32(&file);

        if (tagType == CHUNK_TAG_STRINGS) {
            int32_t numStrings = file_readInt32(&file);

            printf("    %d strings\n", numStrings);

            for (int i = 0; i < numStrings; ++i) {
                int32_t stringLen = file_readInt32(&file);

                stbds_arrput(doc->strings, file_readString(&file, stringLen));

                // NOTE: align to DWORD
                file_align(&file);
            }
        } else if (tagType == CHUNK_TAG_COLORS) {
            int32_t numColors = file_readInt32(&file);
            printf("    %d colors\n", numColors);

            for (int i = 0; i < numColors; ++i) {
                color_t color = {
                    file_readInt16(&file) / 256.0f,
                    file_readInt16(&file) / 256.0f,
                    file_readInt16(&file) / 256.0f,
                    file_readInt16(&file) / 256.0f,
                };

                stbds_arrput(doc->colors, color);
            }
        } else if (tagType == CHUNK_TAG_TRANSFORMS) {
            int32_t numTransforms = file_readInt32(&file);
            printf("    %d transforms\n", numTransforms);

            for (int i = 0; i < numTransforms; ++i) {
                Matrix xform = {
                    file_readFloat(&file), file_readFloat(&file), 0.f, 0.f,
                    file_readFloat(&file), file_readFloat(&file), 0.f, 0.f,
                    file_readFloat(&file), file_readFloat(&file), 1.f, 0.f,
                    0.f, 0.f, 0.f, 1.f
                };

                stbds_arrput(doc->transforms, xform);
            }
        } else if (tagType == CHUNK_TAG_POSITIONS) {
            int32_t numPositions = file_readInt32(&file);
            printf("    %d positions\n", numPositions);

            for (int i = 0; i < numPositions; ++i) {
                Vector2 pos = {
                    file_readFloat(&file),
                    file_readFloat(&file)
                };

                stbds_arrput(doc->positions, pos);
            }
        } else if (tagType == CHUNK_TAG_BOUNDS) {
            int32_t numBounds = file_readInt32(&file);
            printf("    %d bounds\n", numBounds);

            for (int i = 0; i < numBounds; ++i) {
                bounds_t b = {
                    file_readFloat(&file),
                    file_readFloat(&file),
                    file_readFloat(&file),
                    file_readFloat(&file)
                };

                stbds_arrput(doc->bounds, b);
            }
        // } else if (tagType == CHUNK_TAG_ACTIONSCRIPT) {
        // } else if (tagType == CHUNK_TAG_ACTIONSCRIPT2) {
        } else if (tagType == CHUNK_TAG_TEXTURE_ATLASES) {
            int32_t numAtlases = file_readInt32(&file);
            printf("    %d atlases\n", numAtlases);

            for (int i = 0; i < numAtlases; ++i) {
                atlas_t atlas = {
                    file_readInt32(&file),
                    file_readInt32(&file),
                    file_readFloat(&file),
                    file_readFloat(&file)
                };

                atlas.texture = loadLmTexture(i);
                atlas.mat = LoadMaterialDefault();
                atlas.mat.maps[MATERIAL_MAP_DIFFUSE].texture = atlas.texture;

                stbds_arrput(doc->atlases, atlas);
            }
        } else if (tagType == CHUNK_TAG_SHAPE) {
            shape_t *shape = MemAlloc(sizeof(*shape));
            shape->character = (character_t) {
                .id = file_readInt32(&file),
                .type = CHARACTER_TYPE_SHAPE,
            };

            shape->unk1 = file_readInt32(&file);
            shape->boundsId = file_readInt32(&file);
            shape->unk2 = file_readInt32(&file);

            int numGraphics = file_readInt32(&file);

            for (int i = 0; i < numGraphics; i++)
            {
                file_skip(&file, 0x08); // graphic chunk header

                graphic_t graphic = { 0 };
                graphic.atlasId = file_readInt32(&file);
                graphic.fillType = file_readInt16(&file);

                int16_t numVerts = file_readInt16(&file);
                int32_t numIndices = file_readInt32(&file);

                for (int i = 0; i < numVerts; i++) {
                    Vector4 vert = {
                        file_readFloat(&file), // x, y
                        file_readFloat(&file),
                        file_readFloat(&file), // u, v
                        file_readFloat(&file),
                    };
                    
                    stbds_arrput(graphic.verts, vert);
                }

                for (int i = 0; i < numIndices; i++) {
                    int16_t idx = file_readInt16(&file);
                    stbds_arrput(graphic.indices, idx);
                }

                // indices are padded to word boundaries
                if ((numIndices % 2) != 0)
                    file_skip(&file, 0x02);

                graphic.mesh = GenMeshGraphic(&graphic);

                stbds_arrput(shape->graphics, graphic);
            }

            stbds_arrput(doc->shapes, shape);
            doc->characters[shape->character.id] = (character_t*)shape;

        } else if (tagType == CHUNK_TAG_GRAPHIC) {
            fprintf(stderr, "ORPHANED OBJECT CHUNK @ 0x%08lX\n", tagOffset);
            assert(false);
        } else if (tagType == CHUNK_TAG_DEFINE_SPRITE) {
            sprite_t *sprite = MemAlloc(sizeof(*sprite));

            sprite->character = (character_t) {
                .id = file_readInt32(&file),
                .type = CHARACTER_TYPE_SPRITE,
            };
            sprite->unk1 = file_readInt32(&file);
            sprite->unk2 = file_readInt32(&file);

            int numLabels = file_readInt32(&file);
            int numFrames = file_readInt32(&file);
            int numKeyframes = file_readInt32(&file);

            sprite->unk3 = file_readInt32(&file);

            for (int i = 0; i < numLabels; i++)
            {
                file_skip(&file, 0x08);
                label_t label = {
                    .nameId = file_readInt32(&file),
                    .startFrame = file_readInt32(&file),
                    .unk1 = file_readInt32(&file),
                };

                stbds_arrput(sprite->labels, label);
            }

            int totalFrames = numFrames + numKeyframes;
            for (int frameId = 0; frameId < totalFrames; frameId++)
            {
                chunk_tag_t frameType = (chunk_tag_t)file_readInt32(&file);
                file_skip(&file, 0x04);

                frame_t frame = { 0 };
                frame.id = file_readInt32(&file);
                int32_t numChildren = file_readInt32(&file);

                for (int childIdx = 0; childIdx < numChildren; ++childIdx) {
                    chunk_tag_t childType = (chunk_tag_t)file_readInt32(&file);
                    int childSize = file_readInt32(&file);

                    if (childType == CHUNK_TAG_REMOVE_OBJECT) {
                        deletion_t deletion = {
                            .unk1 = file_readInt32(&file),
                            .spriteObjectId = file_readInt16(&file),
                            .unk2 = file_readInt16(&file),
                        };

                        stbds_arrput(frame.deletions, deletion);
                    } else if (childType == CHUNK_TAG_DO_ACTION) {
                        action_t action = {
                            .actionId = file_readInt32(&file),
                            .unk1 = file_readInt32(&file),
                        };
                        stbds_arrput(frame.actions, action);
                    } else if (childType == CHUNK_TAG_PLACE_OBJECT) {
                        placement_t placement = {
                            .characterId = file_readInt32(&file),
                            .placementId = file_readInt32(&file),
                            .unk1 = file_readInt32(&file),
                            .nameId = file_readInt32(&file),
                            .flags = file_readInt16(&file),
                            .blendMode = file_readInt16(&file),
                            .depth = file_readInt16(&file),
                            .unk4 = file_readInt16(&file),
                            .unk5 = file_readInt16(&file),
                            .unk6 = file_readInt16(&file),
                            .positionFlags = file_readInt16(&file),
                            .positionId = file_readInt16(&file),
                            .colorMultId = file_readInt32(&file),
                            .colorAddId = file_readInt32(&file),
                            .hasColorMatrix = file_readInt32(&file),
                            .hasUnkF014 = file_readInt32(&file),
                        };

                        if (placement.hasColorMatrix) {
                            file_skip(&file, 0x08);
                            for (int j = 0; j < 20; ++j) {
                                placement.colorMatrix.data[j] = file_readFloat(&file);
                            }
                        }

                        if (placement.hasUnkF014) {
                            fprintf(stderr, "unhandled unkF014\n");
                            assert(false);
                        }

                        // int maxDepth = doc->properties.maxDepth;
                        // stbds_arrsetlen(placement.displayList, maxDepth);
                        // memset(placement.displayList, 0, maxDepth * sizeof(placement.displayList[0]));

                        stbds_arrput(frame.placements, placement);
                    } else {
                        fprintf(stderr, "frame has unexpected child type 0x%04X\n", childType);
                        assert(false);
                    }
                }

                if (frameType == CHUNK_TAG_KEYFRAME)
                    stbds_arrput(sprite->keyframes, frame);
                else
                    stbds_arrput(sprite->frames, frame);
            }

            for (int i = 0; i < numLabels; ++i) {
                label_t *label = &sprite->labels[i];
                label->id = i;
                sprite->frames[label->startFrame].label = label;
            }

            // int maxDepth = doc->properties.maxDepth;
            // stbds_arrsetlen(sprite->displayList, maxDepth);
            // memset(sprite->displayList, 0, maxDepth * sizeof(sprite->displayList[0]));

            stbds_arrput(doc->sprites, sprite);
            doc->characters[sprite->character.id] = (character_t*)sprite;
            // stbds_arrput(doc->characters, (character_t*)sprite);
        } else if (tagType == CHUNK_TAG_PROPERTIES) {
            doc->properties.unk0 = file_readInt32(&file);
            doc->properties.unk1 = file_readInt32(&file);
            doc->properties.unk2 = file_readInt32(&file);
            doc->properties.maxCharacterId = file_readInt32(&file);
            doc->properties.unk4 = file_readInt32(&file);
            doc->properties.maxCharacterId2 = file_readInt32(&file);
            doc->properties.maxDepth = file_readInt16(&file);
            doc->properties.unk7 = file_readInt16(&file);
            doc->properties.framerate = file_readFloat(&file);
            doc->properties.width = file_readFloat(&file);
            doc->properties.height = file_readFloat(&file);
            doc->properties.unk8 = file_readInt32(&file);
            doc->properties.unk9 = file_readInt32(&file);

            stbds_arrsetlen(doc->characters, doc->properties.maxCharacterId);
            memset(doc->characters, 0, doc->properties.maxCharacterId * sizeof(doc->characters[0]));
        } else if (tagType == CHUNK_TAG_END) {
            done = true;
        } else {
            // printf("unhandled tag 0x%04X @ 0x%08X\n", tagType, tagOffset);
            file.ptr += (tagSize * 4);
        }
    }

    MemFree(data_);

    return doc;
}
