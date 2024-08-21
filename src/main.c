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

int g_screenWidth = 1280;
int g_screenHeight = 720;
Texture g_bgTexture;

typedef struct {
    bool visible;
    Vector2 scroll;
} panel_state_t;

int ui_selectedCharacterId;
sprite_instance_t ui_selectedSpriteInstance = { 0 };

bool ui_charsPanelShowShape = true;
bool ui_charsPanelShowSprite = true;
bool ui_charsPanelShowDynText = true;
bool ui_charsPanelShowNull = false;
bool ui_charsPanelShowUnk = true;

panel_state_t ui_previewPanel = { true };
panel_state_t ui_timelinePanel = { true };
panel_state_t ui_charactersPanel = { true };
panel_state_t ui_displayListPanel = { true };
panel_state_t ui_texlistPanel = { false };
panel_state_t ui_atlasPanel = { true };

sprite_t *g_rootSprite;
sprite_instance_t g_rootSpriteInstance = { 0 };

void drawDisplayListPanel(lumen_document_t *doc)
{
    if (!ui_displayListPanel.visible)
        return;

    int x = 865;
    int y = 34;
    int w = 250;
    int h = 300;

    sprite_instance_t *inst = &g_rootSpriteInstance;
    if (ui_selectedSpriteInstance.sprite) {
        inst = &ui_selectedSpriteInstance;
    }
    size_t numEntries = stbds_arrlenu(inst->displayList);

    GuiScrollPanel(
        (Rectangle) { x, y, w, h},
        "#173#Display List",
        (Rectangle) { x+8, y+8, w-16, numEntries * 18 + 16},
        &ui_displayListPanel.scroll,
        NULL
    );

    // close button
    if (GuiButton((Rectangle) { x + w - 20, y + 4, 16, 16 }, "X")) {
        ui_displayListPanel.visible = false;
        return;
    }

    BeginScissorMode(x, y + 24 + 8, w, h - (24 + 8*2));

    x += 8 + ui_displayListPanel.scroll.x;
    y += 24 + 8 + ui_displayListPanel.scroll.y;
    for (int i = 0; i < numEntries; ++i) {
        displaylist_entry_t *entry = &inst->displayList[i];
        if (entry->character == NULL) {
            continue;
        }

        const char *labelText;

        if (entry->character->type == CHARACTER_TYPE_SHAPE) {
            shape_t *shape = (shape_t*)entry->character;
            labelText = TextFormat("#12#<Shape 0x%02X>", entry->character->id);
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE) {
            assert(false);
        } else if (entry->character->type == CHARACTER_TYPE_DYNAMIC_TEXT) {
            labelText = TextFormat("#12#text(id=0x%04X)", entry->character->id);
        } else if (entry->character->type == CHARACTER_TYPE_SPRITE_INSTANCE) {
            sprite_instance_t *inst = (sprite_instance_t*)entry->character;
            labelText = TextFormat("#13#%s (0x%04X)", doc->strings[entry->nameId], entry->character->id);
        }

        GuiLabel((Rectangle) { x, y, 184, 16 }, labelText);

        y += 18;
    }
    EndScissorMode();
}

void drawTexlistPanel(texlist_t *texlist)
{
    if (!ui_texlistPanel.visible)
        return;

    int x = 500;
    int y = 34;
    size_t numTextures = stbds_arrlenu(texlist->textures);

    GuiScrollPanel(
        (Rectangle) { x, y, 200, 300},
        "#143#Texlist",
        (Rectangle) { x+8, y+8, 200-16, numTextures * 18 + 16},
        &ui_texlistPanel.scroll,
        NULL
    );

    // close button
    if (GuiButton((Rectangle) { x + 200 - 20, y + 4, 16, 16 }, "X")) {
        ui_texlistPanel.visible = false;
        return;
    }

    BeginScissorMode(x, y + 24 + 8, 200, 300 - (24 + 8*2));
    x += 8 + ui_texlistPanel.scroll.x;
    y += 24 + 8 + ui_texlistPanel.scroll.y;

    for (int i = 0; i < numTextures; ++i) {
        texlist_entry_t *texlistEntry = &texlist->textures[i];

        const char *s = TextFormat("%d: %s", i, texlistEntry->name);
        GuiLabel((Rectangle) { x, y, 184, 16 }, s);

        y += 18;
    }
    EndScissorMode();
}

// TODO: give this fucker a header so I can clean this up a bit.
void selectSprite(lumen_document_t *doc, sprite_t *sprite);
void processKeyframe(lumen_document_t *doc, sprite_instance_t *inst, int keyframeIdx);
void jumpToFrame(lumen_document_t *doc, sprite_instance_t *inst, int newFrameIdx);

void drawCharactersPanel(lumen_document_t *doc)
{
    if (!ui_charactersPanel.visible)
        return;

    int x = 10;
    int y = 34;

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
        } else if (!ui_charsPanelShowUnk) {
            continue;
        }

        numVisibleChars++;
    }

    Rectangle windowRect = (Rectangle) { x, y, 200, 300};
    Rectangle contentRect = { x+8, y+8, 200-16, numVisibleChars * 18 + 16};
    GuiScrollPanel(
        windowRect,
        "#218#Characters",
        contentRect,
        &ui_charactersPanel.scroll,
        NULL
    );

    // close button
    if (GuiButton((Rectangle) { x + 200 - 20, y + 4, 16, 16 }, "X")) {
        ui_charactersPanel.visible = false;
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

    x += 8 + ui_charactersPanel.scroll.x;
    y += 24 + 8 + ui_charactersPanel.scroll.y;
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
            if (ch->type == CHARACTER_TYPE_SPRITE) {
                selectSprite(doc, (sprite_t*)ch);
            }
        }
        GuiSetState(STATE_NORMAL);
        y += 18;
    }
    EndScissorMode();
}

// FIXME: move somewhere sensible
#define PANEL_PADDING (8)
#define HEADER_HEIGHT (24)
#define MAX(a,b) (((a)>(b))?(a):(b))

void drawMenuBar()
{
    GuiPanel((Rectangle) { 0, 0, g_screenWidth, 24 }, NULL);

    int x = 4;
    GuiLabel((Rectangle) { x, 4, 100, 16 }, "Panel visibility:");
    x += 90;

    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#139#", &ui_timelinePanel.visible); x += 18;
    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#218#", &ui_charactersPanel.visible); x += 18;
    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#173#", &ui_displayListPanel.visible); x += 18;
    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#143#", &ui_texlistPanel.visible); x += 18;
    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#12#", &ui_atlasPanel.visible); x += 18;
    GuiToggle((Rectangle) { x, 4, 16, 16 }, "#169#", &ui_previewPanel.visible); x += 18;
}

void drawTimelinePanel(lumen_document_t *doc)
{
    if (!ui_timelinePanel.visible)
        return;

    if (!g_rootSprite && !ui_selectedSpriteInstance.sprite)
        return;

    sprite_t *sprite;
    sprite_instance_t *spriteInstance;

    if (ui_selectedSpriteInstance.sprite) {
        sprite = ui_selectedSpriteInstance.sprite;
        spriteInstance = &ui_selectedSpriteInstance;
    } else {
        sprite = g_rootSprite;
        spriteInstance = &g_rootSpriteInstance;
    }

    size_t numFrames = stbds_arrlenu(sprite->frames);
    size_t numKeyframes = stbds_arrlenu(sprite->keyframes);

    const char *s = TextFormat("%d", numFrames);
    // FIXME: this would only be accurate with monospace fonts.
    Vector2 textSize = MeasureTextEx(GuiGetFont(), s, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
    int minSize = textSize.x+4;

    int w = g_screenWidth;
    int h = 200;
    int x = 0;
    int y = g_screenHeight - h;
    int contentWidth = numFrames * minSize + (PANEL_PADDING*2);
    int contentHeight = h - (24 + PANEL_PADDING*2);

    GuiScrollPanel(
        (Rectangle) { x, y, w, h},
        TextFormat("#139#Timeline - %d frames, %d keyframes", numFrames, numKeyframes),
        (Rectangle) { x+PANEL_PADDING, y+PANEL_PADDING, contentWidth, contentHeight },
        &ui_timelinePanel.scroll,
        NULL
    );

    BeginScissorMode(x, y + HEADER_HEIGHT + PANEL_PADDING, w, contentHeight);

    x += PANEL_PADDING + ui_timelinePanel.scroll.x;
    y += HEADER_HEIGHT + PANEL_PADDING + ui_timelinePanel.scroll.y;

    // draw labels first
    int startX = x;
    x += 110;
    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        frame_t *frame = &sprite->frames[frameIdx];
        int size = minSize;

        if (frame->label) {
            const char *labelStr = doc->strings[frame->label->nameId];
            Vector2 textSize = MeasureTextEx(GuiGetFont(), labelStr, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
            if (GuiButton((Rectangle) { x, y, textSize.x + 4, 16 }, labelStr)) {
                spriteInstance->currentFrame = frame->label->startFrame;
                processKeyframe(doc, spriteInstance, frame->label->id);
            }
            size = MAX(size, textSize.x+4);
        }

        x += size + 2;
    }

    x = startX;
    size_t numEntries = stbds_arrlenu(spriteInstance->displayList);
    for (int i = 0; i < numEntries; ++i) {
        displaylist_entry_t *entry = &spriteInstance->displayList[i];
        if (entry->character == NULL) {
            continue;
        }

        if (entry->character->type == CHARACTER_TYPE_SHAPE) {
            shape_t *shape = (shape_t*)entry->character;
            const char *s = TextFormat("<Shape 0x%02X>", entry->character->id);
            GuiLabel((Rectangle) { x, y + 18, 184, 16 }, s);
        } else {
            const char *s = TextFormat("%s", doc->strings[entry->nameId]);
            GuiLabel((Rectangle) { x, y + 18, 184, 16 }, s);
        }

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
                        spriteInstance->currentFrame = frameIdx;
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
                        spriteInstance->currentFrame = frameIdx;
                    }
                    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0xc9c9c9ff);
                    break;
                }
            }

            if (frameIdx == spriteInstance->currentFrame && i == numEntries-1) {
                GuiLabel((Rectangle) { x + size/4.f, y + 36, size, 16 }, "#121#");
            }

            x += size + 2;
        }

        y += 18;
        x = startX;
    }

    EndScissorMode();
}

void drawAtlasPanel(lumen_document_t *doc)
{
    if (!ui_atlasPanel.visible)
        return;

    int x = g_screenWidth - 150 - 8;
    int y = 34;
    int w = 150;
    int h = 480;

    size_t numAtlases = stbds_arrlenu(doc->atlases);

    GuiScrollPanel(
        (Rectangle) { x, y, w, h },
        "#12#Atlases",
        (Rectangle) { x+PANEL_PADDING, y+PANEL_PADDING, w-(PANEL_PADDING*2), numAtlases * (18+64) + (PANEL_PADDING*2) },
        &ui_atlasPanel.scroll,
        NULL
    );

    // close button
    if (GuiButton((Rectangle) { x + w - 20, y + 4, 16, 16 }, "X")) {
        ui_atlasPanel.visible = false;
        return;
    }

    BeginScissorMode(x, y + 24 + 8, w, h - (HEADER_HEIGHT + PANEL_PADDING*2));

    x += 8 + ui_atlasPanel.scroll.x;
    y += 24 + 8 + ui_atlasPanel.scroll.y;

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

    EndScissorMode();
}

void drawPreviewPanel(RenderTexture *renderBuffer)
{
    if (!ui_previewPanel.visible)
        return;

    int x = 218;
    int y = 34;
    int w = g_screenWidth / 2;
    int h = g_screenHeight / 2;

    Rectangle contentRect = {
        x+1,
        y+HEADER_HEIGHT+1,
        w-2,
        h-HEADER_HEIGHT-2
    };

    if (GuiWindowBox((Rectangle) { x, y, w, h }, "#169#Preview")) {
        ui_previewPanel.visible = false;
        return;
    }

    DrawRectangleRec(contentRect, BLACK);

    float rx = contentRect.width / renderBuffer->texture.width;
    float ry = contentRect.height / renderBuffer->texture.height;
    float scale = (rx < ry) ? rx : ry;

    Rectangle bufferRect = {
        contentRect.x,
        contentRect.y,
        renderBuffer->texture.width * scale,
        renderBuffer->texture.height * scale
    };

    bufferRect.x += (contentRect.width - bufferRect.width) / 2;
    bufferRect.y += (contentRect.height - bufferRect.height) / 2;

    DrawTexturePro(
        renderBuffer->texture,
        (Rectangle) { 0, 0, renderBuffer->texture.width, renderBuffer->texture.height },
        bufferRect,
        Vector2Zero(), 0, WHITE
    );
}

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
