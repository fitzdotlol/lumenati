#include <stdio.h>
#include <assert.h>

#include "stb_ds.h"

#include "raygui.h"
#include "lumen.h"
#include "texlist.h"
#include "global.h"
#include "panels.h"

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

// TODO: give this fucker a header so I can clean this up a bit.
void selectSprite(lumen_document_t *doc, sprite_t *sprite);
void processKeyframe(lumen_document_t *doc, sprite_instance_t *inst, int keyframeIdx);
void jumpToFrame(lumen_document_t *doc, sprite_instance_t *inst, int newFrameIdx);

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
