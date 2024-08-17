#include <raylib.h>

#include "file.h"

PixelFormat getPixelFormatFromNutFormat(int typet);
Image readNTP3(filereader_t *file);
Image readNTWU(filereader_t *file);
Texture LoadNUTTexture(const char *filename);
