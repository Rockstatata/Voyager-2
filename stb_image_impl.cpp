// Single translation unit that compiles stb_image's implementation.
// stb_image.h is a public-domain header-only decoder (github.com/nothings/stb) —
// vendored the same way glad.c/GLFW/GLM are, it is not part of the geometry or
// shading pipeline, only a JPEG/PNG byte decoder feeding Texture2D::loadFromFile.
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
