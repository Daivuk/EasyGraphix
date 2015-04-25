#pragma once

#ifndef EG_SHADERS_H_INCLUDED
#define EG_SHADERS_H_INCLUDED

#include <d3dcompiler.h>

extern const char *g_vs;
extern const char *g_ps;
extern const char *g_psAlphaTest[8];
extern const char *g_vsPassThrough;
extern const char *g_psPassThrough;
extern const char *g_psAmbient;
extern const char *g_psOmni;

ID3DBlob *compileShader(const char *szSource, const char *szProfile);

#endif /* EG_SHADERS_H_INCLUDED */
