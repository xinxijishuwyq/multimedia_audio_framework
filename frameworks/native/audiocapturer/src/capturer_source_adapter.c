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
#include <string.h>

#include "audio_types.h"

#include "capturer_source_adapter.h"

#include "audio_log.h"

#ifdef __cplusplus
extern "C" {
#endif

const int32_t  SUCCESS = 0;
const int32_t  ERROR = -1;

const int32_t CLASS_TYPE_PRIMARY = 0;
const int32_t CLASS_TYPE_A2DP = 1;
const int32_t CLASS_TYPE_FILE = 2;

const char *g_deviceClassPrimary = "primary";
const char *g_deviceClassA2Dp = "a2dp";
const char *g_deviceClassFile = "file_io";

int32_t g_deviceClass = -1;

static int32_t CapturerSourceInitInner(const SourceAttr *attr)
{
    AUDIO_INFO_LOG("%{public}s: CapturerSourceInitInner", __func__);
    if (attr == NULL) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    if (g_deviceClass == -1) {
        AUDIO_ERR_LOG("%{public}s: Adapter not loaded", __func__);
        return ERROR;
    }

    if (g_deviceClass == CLASS_TYPE_PRIMARY) {
        AUDIO_INFO_LOG("%{public}s: CLASS_TYPE_PRIMARY", __func__);
        return AudioCapturerSourceInit((AudioSourceAttr *)attr);
    } else if (g_deviceClass == CLASS_TYPE_FILE) {
        AUDIO_INFO_LOG("%{public}s: CLASS_TYPE_FILE", __func__);
        return AudioCapturerFileSourceInit(attr->filePath);
    } else {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        return ERROR;
    }
}

int32_t LoadSourceAdapter(const char *device, struct CapturerSourceAdapter **sourceAdapter)
{
    AUDIO_INFO_LOG("%{public}s: LoadSourceAdapter: %{public}s", __func__, device);
    if ((device == NULL) || (sourceAdapter == NULL)) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    struct CapturerSourceAdapter *adapter = (struct CapturerSourceAdapter *)calloc(1, sizeof(*adapter));
    if (adapter == NULL) {
        AUDIO_ERR_LOG("%{public}s: alloc sink adapter failed", __func__);
        return ERROR;
    }

    if (!strcmp(device, g_deviceClassPrimary)) {
        AUDIO_INFO_LOG("%{public}s: primary device", __func__);
        adapter->CapturerSourceInit = CapturerSourceInitInner;
        adapter->CapturerSourceDeInit = AudioCapturerSourceDeInit;
        adapter->CapturerSourceStart = AudioCapturerSourceStart;
        adapter->CapturerSourceStop = AudioCapturerSourceStop;
        adapter->CapturerSourceSetMute = AudioCapturerSourceSetMute;
        adapter->CapturerSourceIsMuteRequired = AudioCapturerSourceIsMuteRequired;
        adapter->CapturerSourceFrame = AudioCapturerSourceFrame;
        adapter->CapturerSourceSetVolume = AudioCapturerSourceSetVolume;
        adapter->CapturerSourceGetVolume = AudioCapturerSourceGetVolume;
        g_deviceClass = CLASS_TYPE_PRIMARY;
    } else if (!strcmp(device, g_deviceClassFile)) {
        AUDIO_INFO_LOG("%{public}s: file source device", __func__);
        adapter->CapturerSourceInit = CapturerSourceInitInner;
        adapter->CapturerSourceDeInit = AudioCapturerFileSourceDeInit;
        adapter->CapturerSourceStart = AudioCapturerFileSourceStart;
        adapter->CapturerSourceStop = AudioCapturerFileSourceStop;
        adapter->CapturerSourceSetMute = AudioCapturerFileSourceSetMute;
        adapter->CapturerSourceIsMuteRequired = AudioCapturerFileSourceIsMuteRequired;
        adapter->CapturerSourceFrame = AudioCapturerFileSourceFrame;
        adapter->CapturerSourceSetVolume = AudioCapturerFileSourceSetVolume;
        adapter->CapturerSourceGetVolume = AudioCapturerFileSourceGetVolume;
        g_deviceClass = CLASS_TYPE_FILE;
    } else {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        free(adapter);
        return ERROR;
    }

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

const char *GetDeviceClass(void)
{
    if (g_deviceClass == CLASS_TYPE_PRIMARY) {
        return g_deviceClassPrimary;
    } else if (g_deviceClass == CLASS_TYPE_FILE) {
        return g_deviceClassFile;
    } else {
        return NULL;
    }
}
#ifdef __cplusplus
}
#endif
