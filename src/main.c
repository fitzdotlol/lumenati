#include <stdio.h>
#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <time.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raygui_theme.h"

#include "lumen.h"
#include "nut.h"
#include "texlist.h"
#include "global.h"
#include "panels.h"

void processKeyframe(lumen_document_t *doc, sprite_instance_t *inst, int keyframeIdx)
{
    if (!inst->sprite->keyframes) {
        return;
    }

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

            processKeyframe(doc, spriteInst, 0);

            ch = (character_t*)spriteInst;
        }

        entry.character = ch;

        inst->displayList[p->depth] = entry;
    }
}

// FIXME: not functional
void jumpToFrame(lumen_document_t *doc, sprite_instance_t *inst, int destFrameIdx)
{
    if (inst->currentFrame == destFrameIdx) {
        printf("[jumpToFrame] already at that frame.\n");
        return;
    }

    if (inst->currentFrame > destFrameIdx) {
        printf("[jumpToFrame] jumping back not yet implemented\n");
        // TODO: ultimately, the goal here is to rewind to the
        // previous keyframe and then just proceed as normal.
        return;
    }

    // TODO: if a keyframe exists between current frame and dest,
    // we should jump there first.

    while (inst->currentFrame < destFrameIdx) {
        int newFrameIdx = inst->currentFrame+1;
        frame_t *frame = &inst->sprite->frames[newFrameIdx];

        size_t numDeletions = stbds_arrlenu(frame->deletions);
        size_t numPlacements = stbds_arrlenu(frame->placements);
        // size_t numActions = stbds_arrlenu(frame->actions);
        
        for (int i = 0; i < numDeletions; ++i) {
            deletion_t *d = &frame->deletions[i];
            // FIXME: this is weird. I'd kinda prefer being
            // able to set the whole entry to null, but w/e.
            inst->displayList[d->depth].character = NULL;
        }

        for (int i = 0; i < numPlacements; ++i) {
            placement_t *p = &frame->placements[i];

            if (p->flags == PLACE_FLAG_MOVE) {

            } else {

            }
        }

        inst->currentFrame = newFrameIdx;
    }
}


void selectSprite(lumen_document_t *doc, sprite_t *sprite)
{
    if (ui_selectedSpriteInstance.displayList) {
        // FIXME: free
    }

    ui_selectedSpriteInstance.sprite = sprite;
    ui_selectedSpriteInstance.currentFrame = 0;
    ui_selectedSpriteInstance.displayList = NULL;

    stbds_arrsetlen(ui_selectedSpriteInstance.displayList, sprite->maxDepth);
    memset(ui_selectedSpriteInstance.displayList, 0, sprite->maxDepth * sizeof(ui_selectedSpriteInstance.displayList[0]));

    processKeyframe(doc, &ui_selectedSpriteInstance, 0);
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

void drawSpriteInstance(lumen_document_t *doc, sprite_instance_t *instance, Matrix mtxIn)
{
    size_t numEntries = stbds_arrlenu(instance->displayList);
    for (int i = 0; i < numEntries; ++i) {
        displaylist_entry_t *entry = &instance->displayList[i];
        if (entry->character == NULL) {
            continue;
        }

        Matrix mtx = mtxIn;
        if (entry->positionFlags == 0) {
            mtx = MatrixMultiply(mtx, doc->transforms[entry->positionId]);
        } else {
            Vector2 pos = doc->positions[entry->positionId];
            mtx = MatrixMultiply(mtx, MatrixTranslate(pos.x, pos.y, 0));
        }

        if (entry->character->type == CHARACTER_TYPE_SHAPE) {
            drawShape(doc, (shape_t*)entry->character, mtx);
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE) {
            assert(false);
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE_INSTANCE) {
            sprite_instance_t *inst = (sprite_instance_t*)entry->character;
            drawSpriteInstance(doc, inst, mtx);
        }
    }
}

// Draw part of a texture (defined by a rectangle) with rotation and scale tiled into dest.
void DrawTextureTiled(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, float scale, Color tint)
{
    if ((texture.id <= 0) || (scale <= 0.0f)) return;  // Wanna see a infinite loop?!...just delete this line!
    if ((source.width == 0) || (source.height == 0)) return;

    int tileWidth = (int)(source.width*scale), tileHeight = (int)(source.height*scale);
    if ((dest.width < tileWidth) && (dest.height < tileHeight))
    {
        // Can fit only one tile
        DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, ((float)dest.height/tileHeight)*source.height},
                    (Rectangle){dest.x, dest.y, dest.width, dest.height}, origin, rotation, tint);
    }
    else if (dest.width <= tileWidth)
    {
        // Tiled vertically (one column)
        int dy = 0;
        for (;dy+tileHeight < dest.height; dy += tileHeight)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, source.height}, (Rectangle){dest.x, dest.y + dy, dest.width, (float)tileHeight}, origin, rotation, tint);
        }

        // Fit last tile
        if (dy < dest.height)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                        (Rectangle){dest.x, dest.y + dy, dest.width, dest.height - dy}, origin, rotation, tint);
        }
    }
    else if (dest.height <= tileHeight)
    {
        // Tiled horizontally (one row)
        int dx = 0;
        for (;dx+tileWidth < dest.width; dx += tileWidth)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, source.width, ((float)dest.height/tileHeight)*source.height}, (Rectangle){dest.x + dx, dest.y, (float)tileWidth, dest.height}, origin, rotation, tint);
        }

        // Fit last tile
        if (dx < dest.width)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, ((float)dest.height/tileHeight)*source.height},
                        (Rectangle){dest.x + dx, dest.y, dest.width - dx, dest.height}, origin, rotation, tint);
        }
    }
    else
    {
        // Tiled both horizontally and vertically (rows and columns)
        int dx = 0;
        for (;dx+tileWidth < dest.width; dx += tileWidth)
        {
            int dy = 0;
            for (;dy+tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture, source, (Rectangle){dest.x + dx, dest.y + dy, (float)tileWidth, (float)tileHeight}, origin, rotation, tint);
            }

            if (dy < dest.height)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                    (Rectangle){dest.x + dx, dest.y + dy, (float)tileWidth, dest.height - dy}, origin, rotation, tint);
            }
        }

        // Fit last column of tiles
        if (dx < dest.width)
        {
            int dy = 0;
            for (;dy+tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, source.height},
                        (Rectangle){dest.x + dx, dest.y + dy, dest.width - dx, (float)tileHeight}, origin, rotation, tint);
            }

            // Draw final tile in the bottom right corner
            if (dy < dest.height)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                    (Rectangle){dest.x + dx, dest.y + dy, dest.width - dx, dest.height - dy}, origin, rotation, tint);
            }
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

    int renderWidth = (int)doc->properties.width;
    int renderHeight = (int)doc->properties.height;
    SetTargetFPS((int)doc->properties.framerate);

    RenderTexture renderBuffer = LoadRenderTexture(renderWidth, renderHeight);

    Camera3D camera = {
        (Vector3){ 0, 0, 10 }, // position
        (Vector3){ 0, 0, 0  }, // target
        (Vector3){ 0, 1, 0  }, // up
        1000, // fovy
        CAMERA_ORTHOGRAPHIC
    };

    g_bgTexture = LoadTexture("data/bg.png");

    g_rootSprite = (sprite_t*)doc->characters[doc->properties.rootCharacterId];
    g_rootSpriteInstance.sprite = g_rootSprite;
    g_rootSpriteInstance.currentFrame = 0;
    g_rootSpriteInstance.displayList = NULL;

    processKeyframe(doc, &g_rootSpriteInstance, 0);

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            g_screenWidth = GetScreenWidth();
            g_screenHeight = GetScreenHeight();
        }

        float t = GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        {
            BeginTextureMode(renderBuffer);
            DrawTextureTiled(
                g_bgTexture,
                (Rectangle) { 0, 0, g_bgTexture.width, g_bgTexture.height },
                (Rectangle) { 0, 0, renderWidth, renderHeight },
                Vector2Zero(), 0, 1, WHITE
            );

            // ClearBackground(MAGENTA);
            BeginMode3D(camera);

            if (ui_selectedCharacterId != -1 && doc->characters[ui_selectedCharacterId] && doc->characters[ui_selectedCharacterId]->type == CHARACTER_TYPE_SHAPE) {
                shape_t *selectedShape = (shape_t*)doc->characters[ui_selectedCharacterId];

                Matrix mtx = MatrixIdentity();
                drawShape(doc, selectedShape, mtx);
            } else if (ui_selectedSpriteInstance.sprite) {
                Matrix mtx = MatrixIdentity();
                drawSpriteInstance(doc, &ui_selectedSpriteInstance, mtx);
            } else {
                Matrix mtx = MatrixTranslate(-960, -540, 0);
                drawSpriteInstance(doc, &g_rootSpriteInstance, mtx);
            }
            
            EndMode3D();
            EndTextureMode();
        }

        drawMenuBar();

        drawPreviewPanel(&renderBuffer);

        drawDisplayListPanel(doc);
        drawTexlistPanel(texlist);
        drawCharactersPanel(doc);
        drawTimelinePanel(doc);
        drawAtlasPanel(doc);

        EndDrawing();
    }

    return 0;
}
