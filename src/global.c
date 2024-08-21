#include "global.h"

int g_screenWidth = 1280;
int g_screenHeight = 720;
Texture g_bgTexture;

int ui_selectedCharacterId;
sprite_instance_t ui_selectedSpriteInstance = { 0 };

sprite_t *g_rootSprite;
sprite_instance_t g_rootSpriteInstance = { 0 };
