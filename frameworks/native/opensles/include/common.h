#ifndef COMMON_H
#define COMMON_H

#include <OpenSLES.h>
#include <OpenSLES_OpenHarmony.h>
#include <OpenSLES_Platform.h>
#include <audioplayer_adapter.h>
#include <builder.h>
#include <table_struct.h>
#include <iostream>
#include <cstdlib>
#include <stddef.h>
#include "media_log.h"

struct CEngine;

struct ClassTable;

/** itf struct **/

struct IObject {
    const struct SLObjectItf_ *mItf;
    CEngine *mEngine;
    const ClassTable *mClass;
    SLuint8 mState;
};

struct IEngine {
    const struct SLEngineItf_ *mItf;
    IObject *mThis;
};

struct IPlay {
    const struct SLPlayItf_ *mItf;
    SLuint32 mState;
    SLuint8 mId;
};

struct IOHBufferQueue {
    const struct SLOHBufferQueueItf_ *mItf;
    SLuint32 mState;
    SLuint8 mId;
};

struct IVolume {
    const struct SLVolumeItf_ *mItf;
    SLuint8 mId;
};

/** class struct **/

struct CEngine {
    IObject mObject;
    IEngine mEngine;
};

struct CAudioPlayer {
    IObject mObject;
    IPlay mPlay;
    IVolume mVolume;
    IOHBufferQueue mBufferQueue;
    SLuint32 mId;
};

struct COutputMix {
    IObject mObject;
};

void IOHBufferQueueInit(void *self, SLuint32 id);

void IEngineInit(void *self);

void IObjectInit(void *self);

void IPlayInit(void *self, SLuint32 id);

void IVolumeInit(void *self, SLuint32 id);

SLresult EngineDestory(void* self);

SLresult AudioPlayerDestroy(void* self);

SLresult OutputMixDestroy(void* self);
#endif