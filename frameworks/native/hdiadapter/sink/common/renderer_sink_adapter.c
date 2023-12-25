/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "renderer_sink_adapter.h"
#include "audio_log.h"

#ifdef __cplusplus
extern "C" {
#endif

const int32_t  SUCCESS = 0;
const int32_t  ERROR = -1;

const int32_t CLASS_TYPE_PRIMARY = 0;
const int32_t CLASS_TYPE_A2DP = 1;
const int32_t CLASS_TYPE_FILE = 2;
const int32_t CLASS_TYPE_REMOTE = 3;
const int32_t CLASS_TYPE_USB = 4;
const int32_t CLASS_TYPE_OFFLOAD = 5;

const char *g_deviceClassPrimary = "primary";
const char *g_deviceClassUsb = "usb";
const char *g_deviceClassA2DP = "a2dp";
const char *g_deviceClassFile = "file_io";
const char *g_deviceClassRemote = "remote";
const char *g_deviceClassOffload = "offload";

int32_t LoadSinkAdapter(const char *device, const char *deviceNetworkId, struct RendererSinkAdapter **sinkAdapter)
{
    AUDIO_INFO_LOG("%{public}s: device:[%{public}s]", __func__, device);
    CHECK_AND_RETURN_RET_LOG((device != NULL) && (sinkAdapter != NULL), ERROR, "Invalid parameter");

    struct RendererSinkAdapter *adapter = (struct RendererSinkAdapter *)calloc(1, sizeof(*adapter));
    CHECK_AND_RETURN_RET_LOG(adapter != NULL, ERROR, "alloc sink adapter failed");

    if (FillinSinkWapper(device, deviceNetworkId, adapter) != SUCCESS) {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        free(adapter);
        return ERROR;
    }
    // fill deviceClass for hdi_sink.c
    adapter->deviceClass = !strcmp(device, g_deviceClassPrimary) ? CLASS_TYPE_PRIMARY : adapter->deviceClass;
    adapter->deviceClass = !strcmp(device, g_deviceClassUsb) ? CLASS_TYPE_USB : adapter->deviceClass;
    adapter->deviceClass = !strcmp(device, g_deviceClassA2DP) ? CLASS_TYPE_A2DP : adapter->deviceClass;
    adapter->deviceClass = !strcmp(device, g_deviceClassFile) ? CLASS_TYPE_FILE : adapter->deviceClass;
    adapter->deviceClass = !strcmp(device, g_deviceClassRemote) ? CLASS_TYPE_REMOTE : adapter->deviceClass;
    adapter->deviceClass = !strcmp(device, g_deviceClassOffload) ? CLASS_TYPE_OFFLOAD : adapter->deviceClass;

    adapter->RendererSinkInit = IAudioRendererSinkInit;
    adapter->RendererSinkDeInit = IAudioRendererSinkDeInit;
    adapter->RendererSinkStart = IAudioRendererSinkStart;
    adapter->RendererSinkStop = IAudioRendererSinkStop;
    adapter->RendererSinkPause = IAudioRendererSinkPause;
    adapter->RendererSinkResume = IAudioRendererSinkResume;
    adapter->RendererRenderFrame = IAudioRendererSinkRenderFrame;
    adapter->RendererSinkSetVolume = IAudioRendererSinkSetVolume;
    adapter->RendererSinkGetVolume = IAudioRendererSinkGetVolume;
    adapter->RendererSinkGetLatency = IAudioRendererSinkGetLatency;
    adapter->RendererRegCallback = IAudioRendererSinkRegCallback;
    adapter->RendererSinkGetPresentationPosition = IAudioRendererSinkGetPresentationPosition;
    adapter->RendererSinkFlush = IAudioRendererSinkFlush;
    adapter->RendererSinkReset = IAudioRendererSinkReset;
    adapter->RendererSinkSetBufferSize = IAudioRendererSinkSetBufferSize;
    adapter->RendererSinkOffloadRunningLockInit = IAudioRendererSinkOffloadRunningLockInit;
    adapter->RendererSinkOffloadRunningLockLock = IAudioRendererSinkOffloadRunningLockLock;
    adapter->RendererSinkOffloadRunningLockUnlock = IAudioRendererSinkOffloadRunningLockUnlock;

    *sinkAdapter = adapter;

    return SUCCESS;
}

int32_t UnLoadSinkAdapter(struct RendererSinkAdapter *sinkAdapter)
{
    CHECK_AND_RETURN_RET_LOG(sinkAdapter != NULL, ERROR, "Invalid parameter");

    free(sinkAdapter);

    return SUCCESS;
}

const char *GetDeviceClass(int32_t deviceClass)
{
    if (deviceClass == CLASS_TYPE_PRIMARY) {
        return g_deviceClassPrimary;
    } else if (deviceClass == CLASS_TYPE_USB) {
        return g_deviceClassUsb;
    } else if (deviceClass == CLASS_TYPE_A2DP) {
        return g_deviceClassA2DP;
    } else if (deviceClass == CLASS_TYPE_FILE) {
        return g_deviceClassFile;
    } else if (deviceClass == CLASS_TYPE_REMOTE) {
        return g_deviceClassRemote;
    } else if (deviceClass == CLASS_TYPE_OFFLOAD) {
        return g_deviceClassOffload;
    } else {
        return "";
    }
}
#ifdef __cplusplus
}
#endif
