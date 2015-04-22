#pragma once

#ifndef EG_PASS_H_INCLUDED
#define EG_PASS_H_INCLUDED

#define G_DIFFUSE       0
#define G_DEPTH         1
#define G_NORMAL        2
#define G_MATERIAL      3

typedef enum
{
    EG_GEOMETRY_PASS        = 0,
    EG_AMBIENT_PASS         = 1,
    EG_OMNI_PASS            = 2,
    EG_SPOT_PASS            = 3,
    EG_POST_PROCESS_PASS    = 4
} EG_PASS;

void beginGeometryPass();
void beginAmbientPass();
void beginOmniPass();
void beginPostProcessPass();

#endif /* EG_PASS_H_INCLUDED */
