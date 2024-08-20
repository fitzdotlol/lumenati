#include <stdio.h>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raygui_theme.h"

#include "lumen.h"
#include "nut.h"

int g_screenWidth = 1280;
int g_screenHeight = 720;

Vector2 ui_shapesPanelScroll;
Vector2 ui_spritesPanelScroll;
Vector2 ui_spritePropertiesPanelScroll;
Vector2 ui_displayListPanelScroll;

void processKeyframe(lumen_document_t *doc, sprite_instance_t *inst, int keyframeIdx)
{
    size_t numPlacements = stbds_arrlenu(inst->sprite->keyframes[keyframeIdx].placements);
    uint16_t maxDepth = doc->properties.maxDepth;

    stbds_arrsetlen(inst->displayList, maxDepth);
    memset(inst->displayList, 0, maxDepth * sizeof(inst->displayList[0]));

    for (int i = 0; i < numPlacements; ++i) {
        placement_t *p = &inst->sprite->keyframes[keyframeIdx].placements[i];

        displaylist_entry_t entry = { 0 };

        entry.placementId = p->placementId;
        entry.unk1 = p->unk1;
        entry.nameId = p->nameId;
        entry.flags = p->flags;
        entry.blendMode = p->blendMode;
        entry.depth = p->depth;
        entry.unk4 = p->unk4;
        entry.unk5 = p->unk5;
        entry.unk6 = p->unk6;
        entry.positionFlags = p->positionFlags;
        entry.positionId = p->positionId;
        entry.colorMultId = p->colorMultId;
        entry.colorAddId = p->colorAddId;

        if (p->hasColorMatrix) {
            entry.colorMatrix = &p->colorMatrix;
        }
        if (p->hasUnkF014) {
            entry.unkF014 = &p->unkF014;
        }

        // if placement is sprite, instantiate
        character_t *ch = doc->characters[p->characterId];
        if (ch->type == CHARACTER_TYPE_SPRITE) {
            sprite_instance_t *spriteInst = (sprite_instance_t*)MemAlloc(sizeof(*spriteInst));
            spriteInst->character.type = CHARACTER_TYPE_SPRITE_INSTANCE;
            spriteInst->character.id = ch->id;
            spriteInst->sprite = (sprite_t*)ch;
            
            stbds_arrsetlen(spriteInst->displayList, maxDepth);
            memset(spriteInst->displayList, 0, maxDepth * sizeof(spriteInst->displayList[0]));

            // processKeyframe(doc, spriteInst, 0);

            ch = (character_t*)spriteInst;
        }

        entry.character = ch;

        inst->displayList[p->depth] = entry;
    }
}

void drawSpriteInstance(lumen_document_t *doc, sprite_instance_t *instance)
{
    size_t numEntries = stbds_arrlenu(instance->displayList);
    for (int i = 0; i < numEntries; ++i) {
        displaylist_entry_t *entry = &instance->displayList[i];
        if (entry->character == NULL) {
            continue;
        }

        if (entry->character->type == CHARACTER_TYPE_SHAPE) {
            shape_t *shape = (shape_t*)entry->character;
            size_t numGraphics = stbds_arrlenu(shape->graphics);

            for (int i = 0; i < numGraphics; ++i) {
                graphic_t *graphic = &shape->graphics[i];

                rlDisableBackfaceCulling();

                Matrix mtx = MatrixIdentity();
                
                /// 0 == use xform
                /// ? == use position
                // if (entry->positionFlags == 0) {
                //     mtx = doc->transforms[entry->positionId];
                //     mtx = MatrixMultiply(mtx, MatrixScale(0.01f, 0.01f, 0.01f));
                // }

                atlas_t *atlas = &doc->atlases[graphic->atlasId];
                DrawMesh(graphic->mesh, atlas->mat, mtx);
            }
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE) {
            assert(false);
            // sprite_t *sprite = (sprite_t*)ch;
            // sprite_instance_t *childInstance;
            // drawSpriteInstance(doc, childInstance);
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE_INSTANCE) {
            // sprite_t *sprite = (sprite_t*)ch;
            // sprite_instance_t *childInstance;
            sprite_instance_t *inst = (sprite_instance_t*)entry->character;
            drawSpriteInstance(doc, inst);
        }
    }
}

int main(int argc, char **argv)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(g_screenWidth, g_screenHeight, "Lumenati");
    // SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    GuiLoadStyleCyber();

    lumen_document_t *doc = lumen_document_load("data/chara/chara.lm");

    int selectedShapeId = -1;
    int currentFrameId = 0;

    size_t numSprites = stbds_arrlenu(doc->sprites);

    // TODO: load from file
    int renderWidth = 1920;
    int renderHeight = 1080;

    RenderTexture renderBuffer = LoadRenderTexture(renderWidth, renderHeight);

    Camera3D camera = {
        Vector3Zero(), // position
        (Vector3){ 0, 0, 1 }, // target
        (Vector3){ 0, 1, 0}, // up
        45, // fovy
        CAMERA_ORTHOGRAPHIC
    };

    Camera2D camera2d = {
        Vector2Zero(),
        Vector2Zero(),
        0, // rotation
        0.1f // zoom
    };

    Material mat = LoadMaterialDefault();


    uint16_t maxDepth = doc->properties.maxDepth;

    sprite_t *rootSprite = doc->sprites[numSprites-1];
    sprite_instance_t rootSpriteInstance = {
        .sprite = rootSprite,
        .currentFrame = 0,
        .displayList = NULL,
    };
    stbds_arrsetlen(rootSpriteInstance.displayList, maxDepth);
    memset(rootSpriteInstance.displayList, 0, maxDepth * sizeof(rootSpriteInstance.displayList[0]));

    processKeyframe(doc, &rootSpriteInstance, 0);
    // size_t numKeyframes = stbds_arrlenu(rootSprite->keyframes);
    // printf("keyframes = %ld\n", numKeyframes);
    // for (int keyframeIdx = 0; keyframeIdx < numKeyframes; ++keyframeIdx) {
    //     frame_t *keyframe = &rootSprite->keyframes[keyframeIdx];
    //     size_t numPlacements = stbds_arrlen(keyframe->placements);
    //     printf("    key(id=%d)\n", keyframeIdx);
    //     for (int j = 0; j < numPlacements; ++j) {
    //         printf("    \"%s\"\n", doc->strings[keyframe->placements[j].nameId]);
    //     }
    // }

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            g_screenWidth = GetScreenWidth();
            g_screenHeight = GetScreenHeight();
        }

        float t = GetFrameTime();
        BeginDrawing();
        ClearBackground(BLACK);

        BeginTextureMode(renderBuffer);
        ClearBackground(BLACK);
        BeginMode3D(camera);


        // if (selectedSpriteId != -1) {
            // sprite_t *sprite = doc->sprites[selectedSpriteId];
            drawSpriteInstance(doc, &rootSpriteInstance);
        // }
        
        // if (selectedShapeId != -1) {
        //     shape_t *selectedShape = doc->shapes[selectedShapeId];
        //     size_t numGraphics = stbds_arrlenu(selectedShape->graphics);

        //     for (int i = 0; i < numGraphics; ++i) {
        //         graphic_t *graphic = &selectedShape->graphics[i];

        //         rlDisableBackfaceCulling();

        //         mat.maps[MATERIAL_MAP_DIFFUSE].texture = doc->atlases[graphic->atlasId].texture;

        //         Matrix mtx = MatrixIdentity();
        //         mtx = MatrixMultiply(mtx, MatrixScale(0.01f, 0.01f, 0.01f));

        //         DrawMesh(graphic->mesh, mat, mtx);
        //     }
        // }
        EndMode3D();
        EndTextureMode();

        DrawTexturePro(
            renderBuffer.texture,
            (Rectangle) { 0, 0, renderWidth, renderHeight },
            (Rectangle) { 0, 0, g_screenWidth, g_screenHeight },
            Vector2Zero(), 0, WHITE
        );

        bool drawUI = true;
        if (drawUI) {
            int x;
            int y;

            //// DISPLAY LIST PANEL
            x = 230;
            y = 10;
            // sprite_t *sprite = doc->sprites[selectedSpriteId];
            size_t numEntries = stbds_arrlenu(rootSpriteInstance.displayList);
            Rectangle displayListPanelView;
            GuiScrollPanel(
                (Rectangle) { x, y, 250, 300},
                "Display List",
                (Rectangle) { x+8, y+8, 250-16, numEntries * 18 + 16},
                &ui_displayListPanelScroll,
                &displayListPanelView
            );
            BeginScissorMode(x, y + 24 + 8, 250, 300 - (24 + 8*2));

            x += 8 + ui_displayListPanelScroll.x;
            y += 24 + 8 + ui_displayListPanelScroll.y;
            for (int i = 0; i < numEntries; ++i) {
                displaylist_entry_t *entry = &rootSpriteInstance.displayList[i];
                if (entry->character == NULL) {
                    continue;
                }

                if (entry->character->type == CHARACTER_TYPE_SHAPE) {
                    shape_t *shape = (shape_t*)entry->character;
                    const char *s = TextFormat("shape(id=0x%04X)", entry->character->id);
                    GuiLabel((Rectangle) { x, y, 184, 16 }, s);

                    size_t numGraphics = stbds_arrlenu(shape->graphics);
                    for (int j = 0; j < numGraphics; ++j) {
                        y += 18;
                        graphic_t *graphic = &shape->graphics[j];
                        atlas_t *graphicAtlas = &doc->atlases[graphic->atlasId];
                        s = TextFormat("graphic(atlas=%d)", graphic->atlasId);
                        GuiLabel((Rectangle) { x + 16, y, 184, 16 }, s);

                        size_t numVerts = stbds_arrlenu(graphic->verts);
                        for (int vertIdx = 0; vertIdx < numVerts; ++vertIdx) {
                            Vector4 vert = graphic->verts[vertIdx];
                            y += 18;
                            s = TextFormat("xy=[%g, %g], uv=[%g, %g]",
                                vert.x, vert.y,
                                vert.z * graphicAtlas->width, vert.w * graphicAtlas->height
                            );
                            GuiLabel((Rectangle) { x + 32, y, 184, 16 }, s);
                        }
                    }
                } else if (entry->character->type == CHARACTER_TYPE_SPRITE_INSTANCE) {
                    sprite_instance_t *inst = (sprite_instance_t*)entry->character;
                    const char *s = TextFormat("sprite_0x%04X(\"%s\")", entry->character->id, doc->strings[entry->nameId]);
                    GuiLabel((Rectangle) { x, y, 184, 16 }, s);
                }

                y += 18;
            }
            EndScissorMode();

            // //// SHAPES PANEL
            // x = 230;
            // y = 10;
            // size_t numShapes = stbds_arrlenu(doc->shapes);
            // Rectangle view;
            // GuiScrollPanel(
            //     (Rectangle) { x, y, 200, 300},
            //     "Shapes",
            //     (Rectangle) { x+8, y+8, 200-16, numShapes * 18 + 16},
            //     &ui_shapesPanelScroll,
            //     &view
            // );
            // BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2));

            // x += 8;
            // y += 24 + 8 + ui_shapesPanelScroll.y;
            // for (int i = 0; i < numShapes; ++i) {
            //     shape_t *shape = doc->shapes[i];

            //     if (i == selectedShapeId) {
            //         GuiSetState(STATE_FOCUSED);
            //     }
            //     const char *s = TextFormat("shape 0x%04X", i);
            //     if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
            //         selectedShapeId = i;
            //         selectedSpriteId = -1;
            //         currentFrameId = 0;
            //     }
            //     GuiSetState(STATE_NORMAL);
            //     y += 18;
            // }
            // EndScissorMode();

            ////// SPRITES PANEL
            x = 10;
            y = 10;
            size_t numSprites = stbds_arrlenu(doc->sprites);
            Rectangle spritesPanelView;
            GuiScrollPanel(
                (Rectangle) { x, y, 200, 300},
                "Sprites",
                (Rectangle) { x+8, y+8, 200-16, numSprites * 18 + 16},
                &ui_spritesPanelScroll,
                &spritesPanelView
            );
            BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2));

            x += 8 + ui_spritesPanelScroll.x;
            y += 24 + 8 + ui_spritesPanelScroll.y;
            for (int i = 0; i < numSprites; ++i) {
                sprite_t *sprite = doc->sprites[i];

                // if (i == selectedSpriteId) {
                //     GuiSetState(STATE_FOCUSED);
                // }
                const char *s = TextFormat("sprite 0x%04X", i);
                if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
                    // selectedSpriteId = i;
                    // currentFrameId = 0;
                    // selectedShapeId = -1;

                    // {
                    //     sprite_t *sprite = doc->sprites[selectedSpriteId];
                    //     int frame = 0;
                    //     size_t numPlacements = stbds_arrlenu(sprite->frames[frame].placements);

                    //     // stbds_arrsetlen(sprite->displayList, maxDepth);
                    //     // memset(sprite->displayList, 0, maxDepth * sizeof(sprite->displayList[0]));

                    //     for (int i = 0; i < numPlacements; ++i) {
                    //         placement_t *p = &sprite->frames[frame].placements[i];

                    //         // sprite->displayList[p->depth] = p;
                    //         // stbds_arrput(displayList, p);
                    //     }
                    // }
                }
                GuiSetState(STATE_NORMAL);
                y += 18;
            }
            EndScissorMode();

            #define PANEL_PADDING (8)
            #define HEADER_HEIGHT (24)
            #define MAX(a,b) (((a)>(b))?(a):(b))

            ////// TIMELINE PANEL
            if (rootSprite != NULL) {
                // sprite_t *sprite = doc->sprites[selectedSpriteId];
                size_t numFrames = stbds_arrlenu(rootSprite->frames);
                const char *s = TextFormat("%d", numFrames);
                Vector2 textSize = MeasureTextEx(GuiGetFont(), s, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                int minSize = textSize.x+4;

                int w = g_screenWidth;
                int h = 200;
                x = 0;
                y = g_screenHeight - h;
                int contentWidth = numFrames * minSize + (PANEL_PADDING*2);
                int contentHeight = h - (24 + PANEL_PADDING*2);

                Rectangle spritePropertiesPanelView;

                GuiScrollPanel(
                    (Rectangle) { x, y, w, h},
                    "Timeline",
                    (Rectangle) { x+PANEL_PADDING, y+PANEL_PADDING, contentWidth, contentHeight },
                    &ui_spritePropertiesPanelScroll,
                    &spritePropertiesPanelView
                );

                BeginScissorMode(x, y + HEADER_HEIGHT + PANEL_PADDING, w, contentHeight);

                x += PANEL_PADDING + ui_spritePropertiesPanelScroll.x;
                y += HEADER_HEIGHT + PANEL_PADDING + ui_spritePropertiesPanelScroll.y;

                // draw labels first
                int startX = x;
                x += 110;
                for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
                    frame_t *frame = &rootSprite->frames[frameIdx];
                    int size = minSize;

                    if (frame->label) {
                        const char *labelStr = doc->strings[frame->label->nameId];
                        Vector2 textSize = MeasureTextEx(GuiGetFont(), labelStr, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                        if (GuiButton((Rectangle) { x, y, textSize.x + 4, 16 }, labelStr)) {
                            processKeyframe(doc, &rootSpriteInstance, frame->label->id);
                        }
                        size = MAX(size, textSize.x+4);
                    }

                    x += size + 2;
                }


                x = startX;
                size_t numEntries = stbds_arrlenu(rootSpriteInstance.displayList);
                for (int i = 0; i < numEntries; ++i) {
                    displaylist_entry_t *entry = &rootSpriteInstance.displayList[i];
                    if (entry->character == NULL) {
                        continue;
                    }

                    if (entry->character->type == CHARACTER_TYPE_SHAPE) {
                        shape_t *shape = (shape_t*)entry->character;
                        const char *s = TextFormat("<Shape %02d>", entry->character->id);
                        GuiLabel((Rectangle) { x, y + 18, 184, 16 }, s);
                    } else {
                        const char *s = TextFormat("%s", doc->strings[entry->nameId]);
                        GuiLabel((Rectangle) { x, y + 18, 184, 16 }, s);
                    }

                    x += 110;

                    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
                        frame_t *frame = &rootSprite->frames[frameIdx];
                        const char *s;
                        
                        int size = minSize;

                        if (frame->label) {
                            const char *labelStr = doc->strings[frame->label->nameId];
                            Vector2 textSize = MeasureTextEx(GuiGetFont(), labelStr, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                            // GuiLabel((Rectangle) { x, y, textSize.x + 4, 16 }, labelStr);
                            size = MAX(size, textSize.x+4);
                        }

                        s = TextFormat("%d", frameIdx+1);

                        size_t numPlacements = stbds_arrlenu(frame->placements);
                        for (int j = 0; j < numPlacements; ++j) {
                            if (i == frame->placements[j].depth) {
                                if (frame->placements[j].flags == PLACE_FLAG_PLACE) {
                                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x007700FF);
                                } else {
                                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0xAA5500FF);
                                }
                                if (GuiButton((Rectangle) { x, y + 18, size, 16 }, s)) {
                                    currentFrameId = frameIdx;
                                }
                                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x024658ff);
                                break;
                            }
                        }

                        size_t numDeletions = stbds_arrlenu(frame->deletions);
                        for (int j = 0; j < numDeletions; ++j) {
                            if (i == frame->deletions[j].spriteObjectId) {
                                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x770000FF);
                                if (GuiButton((Rectangle) { x, y + 18, size, 16 }, s)) {
                                    currentFrameId = frameIdx;
                                }
                                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0xc9c9c9ff);
                                break;
                            }
                        }

                        // if (frameIdx == currentFrameId) {
                        //     GuiLabel((Rectangle) { x + size/4.f, y + 36, size, 16 }, "#121#");
                        // }

                        x += size + 2;
                    }

                    y += 18;
                    x = startX;
                }

                EndScissorMode();
            } // end timeline panel

            /// ATLAS PANEL
            x = g_screenWidth - 150 - 8;
            y = 10;
            GuiPanel((Rectangle) { x, y, 150, 300}, "Atlases");
            x += 8;
            y += 24 + 8;
            size_t numAtlases = stbds_arrlenu(doc->atlases);

            for (int i = 0; i < numAtlases; ++i) {
                atlas_t *atlas = &doc->atlases[i];

                const char *s = TextFormat("0x%04X: 0x%08X", i, atlas->unk);

                if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
                    y += 1;
                    // selectedGraphicId = i;
                }

                y += 18;

                Rectangle src = { 0, 0, atlas->texture.width, atlas->texture.height };
                Rectangle dest = { x + 16, y, 64, 64 };

                DrawTexturePro(atlas->texture, src, dest, Vector2Zero(), 0, WHITE);

                y += 64;
            }

            ///// GRAPHICS PANEL
            // x = 480;
            // y = 10;
            // GuiPanel((Rectangle) { x, y, 150, 300}, "Graphics");
            // x += 8;
            // y += 24 + 8;
            // if (selectedShapeId != -1) {
            //     shape_t *shape = &shapes[selectedShapeId];
            //     size_t numGraphics = stbds_arrlenu(shape->graphics);

            //     for (int i = 0; i < numGraphics; ++i) {
            //         graphic_t *graphic = &shape->graphics[i];

            //         if (i == selectedGraphicId) {
            //             GuiSetState(STATE_FOCUSED);
            //         }
            //         const char *s = TextFormat("graphic 0x%04X", i);
            //         if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
            //             selectedGraphicId = i;
            //         }
            //         GuiSetState(STATE_NORMAL);
            //         y += 18;
            //     }
            // }
        }

        EndDrawing();
    }

    return 0;
}
