#pragma once

#include "lumen.h"
#include "texlist.h"

#define PANEL_PADDING (8)
#define HEADER_HEIGHT (24)

// FIXME: move somewhere sensible
#define MAX(a,b) (((a)>(b))?(a):(b))

void drawMenuBar();

void drawDisplayListPanel(lumen_document_t *doc);
void drawTexlistPanel(texlist_t *texlist);
void drawCharactersPanel(lumen_document_t *doc);
void drawTimelinePanel(lumen_document_t *doc);
void drawAtlasPanel(lumen_document_t *doc);
void drawPreviewPanel(RenderTexture *renderBuffer);

typedef struct {
    bool visible;
    Vector2 scroll;
} panel_state_t;
