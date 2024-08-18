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

void drawSpriteInstance(lumen_document_t *doc, sprite_instance_t *instance)
{
    size_t numEntries = stbds_arrlenu(instance->displayList);
    for (int i = 0; i < numEntries; ++i) {
        placement_t *place = instance->displayList[i];
        if (place == NULL) {
            continue;
        }

        character_t *ch = doc->characters[place->characterId];

        if (ch->type == CHARACTER_TYPE_SHAPE) {
            shape_t *shape = (shape_t*)ch;
            size_t numGraphics = stbds_arrlenu(shape->graphics);

            for (int i = 0; i < numGraphics; ++i) {
                graphic_t *graphic = &shape->graphics[i];

                rlDisableBackfaceCulling();

                Matrix mtx = MatrixIdentity();
                mtx = MatrixMultiply(mtx, MatrixScale(0.01f, 0.01f, 0.01f));

                atlas_t *atlas = &doc->atlases[graphic->atlasId];
                DrawMesh(graphic->mesh, atlas->mat, mtx);
            }
        } else if (ch->type == CHARACTER_TYPE_SPRITE) {
            // sprite_t *sprite = (sprite_t*)ch;
            // sprite_instance_t *childInstance;
            // drawSpriteInstance(doc, childInstance);
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

    lumen_document_t *doc = lumen_document_load("/home/fitz/s4data/ui/lumen/chara/chara.lm");

    int selectedShapeId = -1;
    int selectedSpriteId = -1;
    selectedSpriteId = 0;
    int currentFrameId = 0;

    size_t numSprites = stbds_arrlenu(doc->sprites);
    selectedSpriteId = numSprites-1;

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
    size_t numPlacements = stbds_arrlenu(rootSprite->keyframes[0].placements);

    stbds_arrsetlen(rootSpriteInstance.displayList, maxDepth);
    memset(rootSpriteInstance.displayList, 0, maxDepth * sizeof(rootSpriteInstance.displayList[0]));

    for (int i = 0; i < numPlacements; ++i) {
        placement_t *p = &rootSprite->keyframes[0].placements[i];

        rootSpriteInstance.displayList[p->depth] = p;
    }

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
                (Rectangle) { x, y, 200, 300},
                "Display List",
                (Rectangle) { x+8, y+8, 200-16, numEntries * 18 + 16},
                &ui_displayListPanelScroll,
                &displayListPanelView
            );
            BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2));

            x += 8 + ui_displayListPanelScroll.x;
            y += 24 + 8 + ui_displayListPanelScroll.y;
            for (int i = 0; i < numEntries; ++i) {
                placement_t *place = rootSpriteInstance.displayList[i];
                if (place == NULL) {
                    continue;
                }

                character_t *ch = doc->characters[place->characterId];

                if (ch->type == CHARACTER_TYPE_SHAPE) {
                    const char *s = TextFormat("shape(name=\"%s\", )", doc->strings[place->nameId]);
                    GuiLabel((Rectangle) { x, y, 184, 16 }, s);
                } else if (ch->type == CHARACTER_TYPE_SPRITE) {
                    const char *s = TextFormat("sprite(name=\"%s\", )", doc->strings[place->nameId]);
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

                if (i == selectedSpriteId) {
                    GuiSetState(STATE_FOCUSED);
                }
                const char *s = TextFormat("sprite 0x%04X", i);
                if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
                    selectedSpriteId = i;
                    currentFrameId = 0;
                    selectedShapeId = -1;

                    {
                        sprite_t *sprite = doc->sprites[selectedSpriteId];
                        int frame = 0;
                        size_t numPlacements = stbds_arrlenu(sprite->frames[frame].placements);

                        // stbds_arrsetlen(sprite->displayList, maxDepth);
                        // memset(sprite->displayList, 0, maxDepth * sizeof(sprite->displayList[0]));

                        for (int i = 0; i < numPlacements; ++i) {
                            placement_t *p = &sprite->frames[frame].placements[i];

                            // sprite->displayList[p->depth] = p;
                            // stbds_arrput(displayList, p);
                        }
                    }
                }
                GuiSetState(STATE_NORMAL);
                y += 18;
            }
            EndScissorMode();

            #define PANEL_PADDING (8)
            #define HEADER_HEIGHT (24)
            #define MAX(a,b) (((a)>(b))?(a):(b))

            ////// TIMELINE PANEL
            if (selectedSpriteId != -1) {
                sprite_t *sprite = doc->sprites[selectedSpriteId];
                size_t numFrames = stbds_arrlenu(sprite->frames);
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
                    TextFormat("Timeline - Sprite 0x%04X", selectedSpriteId),
                    (Rectangle) { x+PANEL_PADDING, y+PANEL_PADDING, contentWidth, contentHeight },
                    &ui_spritePropertiesPanelScroll,
                    &spritePropertiesPanelView
                );

                BeginScissorMode(x, y + HEADER_HEIGHT + PANEL_PADDING, w, contentHeight);

                x += PANEL_PADDING + ui_spritePropertiesPanelScroll.x;
                y += HEADER_HEIGHT + PANEL_PADDING + ui_spritePropertiesPanelScroll.y;

                int startX = x;
                x += 110;
                for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
                    frame_t *frame = &sprite->frames[frameIdx];
                    int size = minSize;

                    if (frame->label) {
                        const char *labelStr = doc->strings[frame->label->nameId];
                        Vector2 textSize = MeasureTextEx(GuiGetFont(), labelStr, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                        GuiLabel((Rectangle) { x, y, textSize.x + 4, 16 }, labelStr);
                        size = MAX(size, textSize.x+4);
                    }

                    x += size + 2;
                }


                x = startX;
                size_t numEntries = stbds_arrlenu(rootSpriteInstance.displayList);
                for (int i = 0; i < numEntries; ++i) {
                    placement_t *placeObj = rootSpriteInstance.displayList[i];
                    if (placeObj == NULL) {
                        continue;
                    }

                    character_t *ch = doc->characters[placeObj->characterId];

                    const char *s = TextFormat("\"%s\"", doc->strings[placeObj->nameId]);
                    GuiLabel((Rectangle) { x, y + 18, 184, 16 }, s);

                    x += 110;

                    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
                        frame_t *frame = &sprite->frames[frameIdx];
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
                                GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0xc9c9c9ff);
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
