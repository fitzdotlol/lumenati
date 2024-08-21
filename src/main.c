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
#include "texlist.h"

int g_screenWidth = 1280;
int g_screenHeight = 720;

int ui_selectedCharacterId;
bool ui_drawCharsPanel = true;
Vector2 ui_charsPanelScroll;

bool ui_charsPanelShowShape = true;
bool ui_charsPanelShowSprite = true;
bool ui_charsPanelShowDynText = true;
bool ui_charsPanelShowNull = false;
bool ui_charsPanelShowUnk = true;

Vector2 ui_spritePropertiesPanelScroll;
Vector2 ui_displayListPanelScroll;
bool ui_drawTexlistPanel = false;
Vector2 ui_texlistPanelScroll;

void drawTexlistPanel(texlist_t *texlist)
{
    if (!ui_drawTexlistPanel)
        return;

    int x = 500;
    int y = 10;
    size_t numTextures = stbds_arrlenu(texlist->textures);
    Rectangle view;
    GuiScrollPanel(
        (Rectangle) { x, y, 200, 300},
        "Texlist",
        (Rectangle) { x+8, y+8, 200-16, numTextures * 18 + 16},
        &ui_texlistPanelScroll,
        &view
    );

    // close button
    if (GuiButton(
        (Rectangle) { x + 200 - 20, y + 4, 16, 16 },
        "X"
    )) {
        ui_drawTexlistPanel = false;
        return;
    }

    BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2));

    x += 8 + ui_texlistPanelScroll.x;
    y += 24 + 8 + ui_texlistPanelScroll.y;
    for (int i = 0; i < numTextures; ++i) {
        texlist_entry_t *texlistEntry = &texlist->textures[i];

        const char *s = TextFormat("%d: %s", i, texlistEntry->name);
        GuiLabel((Rectangle) { x, y, 184, 16 }, s);

        y += 18;
    }
    EndScissorMode();
}

void drawCharactersPanel(lumen_document_t *doc)
{
    if (!ui_drawCharsPanel)
        return;

    int x = 10;
    int y = 10;
    // FIXME: should just be visible (i.e., not filtered) characters.
    size_t numChars = stbds_arrlenu(doc->characters);

    size_t numVisibleChars = 0;
    for (int i = 0; i < numChars; ++i) {
        character_t *ch = doc->characters[i];
        if (!ch) {
            if (ui_charsPanelShowNull) {
                numVisibleChars++;
            }
            continue;
        }

        if (ch->type == CHARACTER_TYPE_SHAPE) {
            if (!ui_charsPanelShowShape)
                continue;
        } else if (ch->type == CHARACTER_TYPE_SPRITE) {
            if (!ui_charsPanelShowSprite)
                continue;
        } else if (ch->type == CHARACTER_TYPE_DYNAMIC_TEXT) {
            if (!ui_charsPanelShowDynText)
                continue;
        } else {
            if (!ui_charsPanelShowUnk)
                continue;
        }

        numVisibleChars++;
    }

    Rectangle charsPanelView;
    GuiScrollPanel(
        (Rectangle) { x, y, 200, 300},
        "Characters",
        (Rectangle) { x+8, y+8, 200-16, numVisibleChars * 18 + 16},
        &ui_charsPanelScroll,
        &charsPanelView
    );

    // close button
    if (GuiButton(
        (Rectangle) { x + 200 - 20, y + 4, 16, 16 },
        "X"
    )) {
        ui_drawCharsPanel = false;
        return;
    }

    y += 18;

    GuiLabel((Rectangle) { x + 8, y + 12, 50, 16}, "Filters:");
    GuiToggle((Rectangle) { x + 54, y + 12, 16, 16}, "#12#", &ui_charsPanelShowShape);
    GuiToggle((Rectangle) { x + 72, y + 12, 16, 16}, "#13#", &ui_charsPanelShowSprite);
    GuiToggle((Rectangle) { x + 90, y + 12, 16, 16}, "#10#", &ui_charsPanelShowDynText);
    GuiToggle((Rectangle) { x + 108, y + 12, 16, 16}, "#193#", &ui_charsPanelShowUnk);
    GuiToggle((Rectangle) { x + 126, y + 12, 16, 16}, "#9#", &ui_charsPanelShowNull);


    BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2 + 18));

    x += 8 + ui_charsPanelScroll.x;
    y += 24 + 8 + ui_charsPanelScroll.y;
    for (int i = 0; i < numChars; ++i) {
        character_t *ch = doc->characters[i];

        if (ch == NULL) {
            if (!ui_charsPanelShowNull) {
                continue;
            }

            const char *ss = TextFormat("NULL 0x%04X", i);
            GuiLabel((Rectangle) { x, y, 184, 16 }, ss);
            y += 18;
            continue;
        }

        const char *s;
        
        if (ch->type == CHARACTER_TYPE_SHAPE) {
            if (!ui_charsPanelShowShape)
                continue;
            s = TextFormat("#12#shape 0x%04X", i);
        } else if (ch->type == CHARACTER_TYPE_SPRITE) {
            if (!ui_charsPanelShowSprite)
                continue;
            s = TextFormat("#13#sprite 0x%04X", i);
        } else if (ch->type == CHARACTER_TYPE_DYNAMIC_TEXT) {
            if (!ui_charsPanelShowDynText)
                continue;
            s = TextFormat("#10#dtext 0x%04X", i);
        } else {
            if (!ui_charsPanelShowUnk)
                continue;
            s = TextFormat("char 0x%04X", i);
        }

        if (i == ui_selectedCharacterId) {
            GuiSetState(STATE_FOCUSED);
        }
        if (GuiLabelButton((Rectangle) { x, y, 184, 16 }, s)) {
            ui_selectedCharacterId = i;
        }
        GuiSetState(STATE_NORMAL);
        y += 18;
    }
    EndScissorMode();
}

void processKeyframe(lumen_document_t *doc, sprite_instance_t *inst, int keyframeIdx)
{
    size_t numPlacements = stbds_arrlenu(inst->sprite->keyframes[keyframeIdx].placements);

    stbds_arrsetlen(inst->displayList, inst->sprite->maxDepth);
    memset(inst->displayList, 0, inst->sprite->maxDepth * sizeof(inst->displayList[0]));

    for (int i = 0; i < numPlacements; ++i) {
        placement_t *p = &inst->sprite->keyframes[keyframeIdx].placements[i];

        displaylist_entry_t entry = { 0 };

        entry.placementId = p->placementId;
        entry.unk1 = p->unk1;
        entry.nameId = p->nameId;
        entry.flags = p->flags;
        entry.blendMode = p->blendMode;
        entry.depth = p->depth;
        entry.unk4 = p->clipDepth;
        entry.unk5 = p->ratio;
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
            
            stbds_arrsetlen(spriteInst->displayList, spriteInst->sprite->maxDepth);
            memset(spriteInst->displayList, 0, spriteInst->sprite->maxDepth * sizeof(spriteInst->displayList[0]));

            // processKeyframe(doc, spriteInst, 0);

            ch = (character_t*)spriteInst;
        }

        entry.character = ch;

        inst->displayList[p->depth] = entry;
    }
}

void drawShape(lumen_document_t *doc, shape_t *shape, Matrix mtx)
{
    size_t numGraphics = stbds_arrlenu(shape->graphics);

    for (int i = 0; i < numGraphics; ++i) {
        graphic_t *graphic = &shape->graphics[i];

        rlDisableBackfaceCulling();

        atlas_t *atlas = &doc->atlases[graphic->atlasId];
        DrawMesh(graphic->mesh, atlas->mat, mtx);
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
            Matrix mtx = MatrixIdentity();

            // // 0 == use xform
            // // ? == use position
            // if (entry->positionFlags == 0) {
            //     mtx = doc->transforms[entry->positionId];
            //     mtx = MatrixMultiply(mtx, MatrixScale(0.01f, 0.01f, 0.01f));
            // }

            drawShape(doc, (shape_t*)entry->character, mtx);
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
    texlist_t *texlist = texlist_load("data/chara/texlist.lst");

    int currentFrameId = 0;

    size_t numSprites = stbds_arrlenu(doc->sprites);

    int renderWidth = (int)doc->properties.width;
    int renderHeight = (int)doc->properties.height;
    SetTargetFPS((int)doc->properties.framerate);

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

    sprite_t *rootSprite = (sprite_t*)doc->characters[doc->properties.rootCharacterId];
    sprite_instance_t rootSpriteInstance = {
        .sprite = rootSprite,
        .currentFrame = 0,
        .displayList = NULL,
    };
    stbds_arrsetlen(rootSpriteInstance.displayList, maxDepth);
    memset(rootSpriteInstance.displayList, 0, maxDepth * sizeof(rootSpriteInstance.displayList[0]));

    processKeyframe(doc, &rootSpriteInstance, 0);

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

        if (ui_selectedCharacterId != -1 && doc->characters[ui_selectedCharacterId] && doc->characters[ui_selectedCharacterId]->type == CHARACTER_TYPE_SHAPE) {
            shape_t *selectedShape = (shape_t*)doc->characters[ui_selectedCharacterId];
            Matrix mtx = MatrixIdentity();
            mtx = MatrixMultiply(mtx, MatrixScale(0.01f, 0.01f, 0.01f));

            drawShape(doc, selectedShape, mtx);
        } else {
            drawSpriteInstance(doc, &rootSpriteInstance);
        }
        
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
                    y += 18;
                    s = TextFormat("    0x%04X, 0x%04X, 0x%04X", shape->unk1, shape->boundsId, shape->unk2);
                    GuiLabel((Rectangle) { x, y, 184, 16 }, s);
                    y += 18;
                    s = TextFormat("    %s", texlist->textures[shape->boundsId].name);
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

            drawTexlistPanel(texlist);
            drawCharactersPanel(doc);

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
                            if (i == frame->deletions[j].depth) {
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

                const char *s = TextFormat("0x%04X: \"%s\"", i, doc->strings[atlas->nameId]);
                GuiLabel((Rectangle) { x, y, 184, 16 }, s);
                y += 18;

                Rectangle src = { 0, 0, atlas->texture.width, atlas->texture.height };
                Rectangle dest = { x + 16, y, 64, 64 };

                DrawTexturePro(atlas->texture, src, dest, Vector2Zero(), 0, WHITE);

                y += 64;
            }
        }

        EndDrawing();
    }

    return 0;
}
