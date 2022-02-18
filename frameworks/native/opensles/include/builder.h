#ifndef BUILDER_H
#define BUILDER_H

#include <common.h>

struct ClassTable;

struct IObject;

ClassTable *ObjectIdToClass(SLuint32 objectId);

IObject *Construct(const ClassTable *classTable, SLEngineItf itf);

#endif