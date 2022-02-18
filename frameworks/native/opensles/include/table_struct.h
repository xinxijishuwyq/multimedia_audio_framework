#ifndef TABLE_STRUCT_H
#define TABLE_STRUCT_H

#include <common.h>

struct ClassTable {
    SLuint32 mObjectId;
    size_t mSize;
};

extern ClassTable EngineTab;

extern ClassTable AudioPlayerTab;

extern ClassTable OutputMixTab;

#endif