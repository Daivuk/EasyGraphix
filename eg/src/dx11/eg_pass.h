#pragma once

#ifndef EG_PASS_H_INCLUDED
#define EG_PASS_H_INCLUDED

#define G_DIFFUSE       0
#define G_DEPTH         1
#define G_NORMAL        2
#define G_MATERIAL      3

typedef enum
{
    EG_GEOMETRY_PASS,
    EG_AMBIENT_PASS,
    EG_OMNI_PASS,
    EG_SPOT_PASS,
    EG_POST_PROCESS_PASS,
    EG_PASS_COUNT
} EG_PASS;

void beginGeometryPass();
void beginAmbientPass();
void beginOmniPass();
void beginPostProcessPass();

#endif /* EG_PASS_H_INCLUDED */
