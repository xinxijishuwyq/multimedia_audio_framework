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

const char *g_deviceClassPrimary = "primary";
const char *g_deviceClassA2Dp = "a2dp";
const char *g_deviceClassFile = "file_io";

int32_t g_deviceClass = -1;

static int32_t RendererSinkInitInner(const SinkAttr *attr)
{
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
        return AudioRendererSinkInit((AudioSinkAttr *)attr);
    } else if (g_deviceClass == CLASS_TYPE_A2DP) {
        AUDIO_INFO_LOG("%{public}s: CLASS_TYPE_A2DP", __func__);
        BluetoothSinkAttr bluetoothSinkAttr;
        bluetoothSinkAttr.format = attr->format;
        bluetoothSinkAttr.sampleFmt = attr->sampleFmt;
        bluetoothSinkAttr.sampleRate = attr->sampleRate;
        bluetoothSinkAttr.channel = attr->channel;
        bluetoothSinkAttr.volume = attr->volume;
        return BluetoothRendererSinkInit(&bluetoothSinkAttr);
    } else if (g_deviceClass == CLASS_TYPE_FILE) {
        AUDIO_INFO_LOG("%{public}s: CLASS_TYPE_FILE", __func__);
        return AudioRendererFileSinkInit(attr->filePath);
    } else {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        return ERROR;
    }
}

int32_t LoadSinkAdapter(const char *device, struct RendererSinkAdapter **sinkAdapter)
{
    AUDIO_INFO_LOG("%{public}s: LoadSinkAdapter: %{public}s", __func__, device);
    if ((device == NULL) || (sinkAdapter == NULL)) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    struct RendererSinkAdapter *adapter = (struct RendererSinkAdapter *)calloc(1, sizeof(*adapter));
    if (adapter == NULL) {
        AUDIO_ERR_LOG("%{public}s: alloc sink adapter failed", __func__);
        return ERROR;
    }

    if (!strcmp(device, g_deviceClassPrimary)) {
        adapter->RendererSinkInit = RendererSinkInitInner;
        adapter->RendererSinkDeInit = AudioRendererSinkDeInit;
        adapter->RendererSinkStart = AudioRendererSinkStart;
        adapter->RendererSinkStop = AudioRendererSinkStop;
        adapter->RendererSinkPause = AudioRendererSinkPause;
        adapter->RendererSinkResume = AudioRendererSinkResume;
        adapter->RendererRenderFrame = AudioRendererRenderFrame;
        adapter->RendererSinkSetVolume = AudioRendererSinkSetVolume;
        adapter->RendererSinkGetLatency = AudioRendererSinkGetLatency;
        adapter->RendererSinkGetTransactionId = AudioRendererSinkGetTransactionId;
        g_deviceClass = CLASS_TYPE_PRIMARY;
    } else if (!strcmp(device, g_deviceClassA2Dp)) {
        adapter->RendererSinkInit = RendererSinkInitInner;
        adapter->RendererSinkDeInit = BluetoothRendererSinkDeInit;
        adapter->RendererSinkStart = BluetoothRendererSinkStart;
        adapter->RendererSinkStop = BluetoothRendererSinkStop;
        adapter->RendererSinkPause = BluetoothRendererSinkPause;
        adapter->RendererSinkResume = BluetoothRendererSinkResume;
        adapter->RendererRenderFrame = BluetoothRendererRenderFrame;
        adapter->RendererSinkSetVolume = BluetoothRendererSinkSetVolume;
        adapter->RendererSinkGetLatency = BluetoothRendererSinkGetLatency;
        adapter->RendererSinkGetTransactionId = BluetoothRendererSinkGetTransactionId;
        g_deviceClass = CLASS_TYPE_A2DP;
    } else if (!strcmp(device, g_deviceClassFile)) {
        adapter->RendererSinkInit = RendererSinkInitInner;
        adapter->RendererSinkDeInit = AudioRendererFileSinkDeInit;
        adapter->RendererSinkStart = AudioRendererFileSinkStart;
        adapter->RendererSinkStop = AudioRendererFileSinkStop;
        adapter->RendererSinkPause = AudioRendererFileSinkPause;
        adapter->RendererSinkResume = AudioRendererFileSinkResume;
        adapter->RendererRenderFrame = AudioRendererFileSinkRenderFrame;
        adapter->RendererSinkSetVolume = AudioRendererFileSinkSetVolume;
        adapter->RendererSinkGetLatency = AudioRendererFileSinkGetLatency;
        adapter->RendererSinkGetTransactionId = AudioRendererFileSinkGetTransactionId;
        g_deviceClass = CLASS_TYPE_FILE;
    } else {
        AUDIO_ERR_LOG("%{public}s: Device not supported", __func__);
        free(adapter);
        return ERROR;
    }

    *sinkAdapter = adapter;

    return SUCCESS;
}

int32_t UnLoadSinkAdapter(struct RendererSinkAdapter *sinkAdapter)
{
    if (sinkAdapter == NULL) {
        AUDIO_ERR_LOG("%{public}s: Invalid parameter", __func__);
        return ERROR;
    }

    free(sinkAdapter);

    return SUCCESS;
}

const char *GetDeviceClass(void)
{
    if (g_deviceClass == CLASS_TYPE_PRIMARY) {
        return g_deviceClassPrimary;
    } else if (g_deviceClass == CLASS_TYPE_A2DP) {
        return g_deviceClassA2Dp;
    } else if (g_deviceClass == CLASS_TYPE_FILE) {
        return g_deviceClassFile;
    } else {
        return NULL;
    }
}
#ifdef __cplusplus
}
#endif
