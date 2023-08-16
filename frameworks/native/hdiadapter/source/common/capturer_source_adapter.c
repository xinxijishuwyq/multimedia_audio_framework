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

#include "audio_log.h"
#include "capturer_source_adapter.h"
#include "i_audio_capturer_source_intf.h"

#ifdef __cplusplus
extern "C" {
#endif

const int32_t  SUCCESS = 0;
const int32_t  ERROR = -1;

const int32_t CLASS_TYPE_PRIMARY = 0;
const int32_t CLASS_TYPE_A2DP = 1;
const int32_t CLASS_TYPE_FILE = 2;
const int32_t CLASS_TYPE_REMOTE = 3;

const char *g_deviceClassPrimary = "primary";
const char *g_deviceClassA2DP = "a2dp";
const char *g_deviceClassFile = "file_io";
const char *g_deviceClassRemote = "remote";

int32_t g_deviceClass = -1;

static int32_t CapturerSourceInitInner(void *wapper, const SourceAttr *attr)
{
    AUDIO_INFO_LOG("%{public}s: CapturerSourceInitInner", __func__);
    if (attr == NULL) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    return IAudioCapturerSourceInit(wapper, (IAudioSourceAttr *)attr);
}

int32_t LoadSourceAdapter(const char *device, const char *deviceNetworkId, const int32_t sourceType,
        const char *sourceName, struct CapturerSourceAdapter **sourceAdapter)
{
    AUDIO_INFO_LOG("%{public}s: sourceType: %{public}d  device: %{public}s ", __func__, sourceType, device);
    if ((device == NULL) || (deviceNetworkId == NULL) || (sourceAdapter == NULL)) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    struct CapturerSourceAdapter *adapter = (struct CapturerSourceAdapter *)calloc(1, sizeof(*adapter));
    if (adapter == NULL) {
        AUDIO_ERR_LOG("%{public}s: alloc sink adapter failed", __func__);
        return ERROR;
    }

    if (FillinSourceWapper(device, deviceNetworkId, sourceType, sourceName, &adapter->wapper) != SUCCESS) {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        free(adapter);
        return ERROR;
    }
    // fill deviceClass for hdi_source.c
    if (!strcmp(device, g_deviceClassPrimary)) {
        adapter->deviceClass = CLASS_TYPE_PRIMARY;
    }
    if (!strcmp(device, g_deviceClassA2DP)) {
        adapter->deviceClass = CLASS_TYPE_A2DP;
    }
    if (!strcmp(device, g_deviceClassFile)) {
        adapter->deviceClass = CLASS_TYPE_FILE;
    }
    if (!strcmp(device, g_deviceClassRemote)) {
        adapter->deviceClass = CLASS_TYPE_REMOTE;
    }
    adapter->CapturerSourceInit = CapturerSourceInitInner;
    adapter->CapturerSourceDeInit = IAudioCapturerSourceDeInit;
    adapter->CapturerSourceStart = IAudioCapturerSourceStart;
    adapter->CapturerSourceStop = IAudioCapturerSourceStop;
    adapter->CapturerSourceSetMute = IAudioCapturerSourceSetMute;
    adapter->CapturerSourceIsMuteRequired = IAudioCapturerSourceIsMuteRequired;
    adapter->CapturerSourceFrame = IAudioCapturerSourceFrame;
    adapter->CapturerSourceSetVolume = IAudioCapturerSourceSetVolume;
    adapter->CapturerSourceGetVolume = IAudioCapturerSourceGetVolume;

    *sourceAdapter = adapter;

    return SUCCESS;
}

int32_t UnLoadSourceAdapter(struct CapturerSourceAdapter *sourceAdapter)
{
    if (sourceAdapter == NULL) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    free(sourceAdapter);

    return SUCCESS;
}

const char *GetDeviceClass(int32_t deviceClass)
{
    if (deviceClass == CLASS_TYPE_PRIMARY) {
        return g_deviceClassPrimary;
    } else if (deviceClass == CLASS_TYPE_A2DP) {
        return g_deviceClassA2DP;
    } else if (deviceClass == CLASS_TYPE_FILE) {
        return g_deviceClassFile;
    } else if (deviceClass == CLASS_TYPE_REMOTE) {
        return g_deviceClassRemote;
    } else {
        return "";
    }
}
#ifdef __cplusplus
}
#endif
