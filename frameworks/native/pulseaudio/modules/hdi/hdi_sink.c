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

#include <config.h>
#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/xmalloc.h>
#include <pulsecore/log.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/sink.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/thread.h>
#include <pulsecore/memblock.c>
#include <pulsecore/mix.h>
#include <pulse/volume.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#include "audio_log.h"
#include "audio_schedule.h"
#include "renderer_sink_adapter.h"
#include "audio_effect_chain_adapter.h"
#include "securec.h"
#include "playback_capturer_adapter.h"

#define DEFAULT_SINK_NAME "hdi_output"
#define DEFAULT_AUDIO_DEVICE_NAME "Speaker"
#define DEFAULT_DEVICE_CLASS "primary"
#define DEFAULT_DEVICE_NETWORKID "LocalDevice"
#define DEFAULT_BUFFER_SIZE 8192
#define MAX_SINK_VOLUME_LEVEL 1.0
#define DEFAULT_WRITE_TIME 1000
#define MIX_BUFFER_LENGTH (pa_page_size())
#define MAX_MIX_CHANNELS 32
#define IN_CHANNEL_NUM_MAX 2
#define OUT_CHANNEL_NUM_MAX 2
#define DEFAULT_FRAMELEN 2048
#define SCENE_TYPE_NUM 7

const char *DEVICE_CLASS_A2DP = "a2dp";
const char *DEVICE_CLASS_REMOTE = "remote";

enum {
    HDI_INIT,
    HDI_DEINIT,
    HDI_START,
    HDI_STOP,
    HDI_RENDER,
    QUIT
};

struct Userdata {
    const char *adapterName;
    uint32_t buffer_size;
    uint32_t fixed_latency;
    uint32_t sink_latency;
    uint32_t render_in_idle_state;
    uint32_t open_mic_speaker;
    const char *deviceNetworkId;
    int32_t deviceType;
    size_t bytes_dropped;
    pa_thread_mq thread_mq;
    pa_memchunk memchunk;
    pa_usec_t block_usec;
    pa_usec_t timestamp;
    pa_thread *thread;
    pa_thread *thread_hdi;
    pa_rtpoll *rtpoll;
    pa_core *core;
    pa_module *module;
    pa_sink *sink;
    pa_sample_spec ss;
    pa_channel_map map;
    bool isHDISinkStarted;
    struct RendererSinkAdapter *sinkAdapter;
    pa_asyncmsgq *dq;
    pa_atomic_t dflag;
    bool test_mode_on;
    uint32_t writeCount;
    uint32_t renderCount;
    pa_usec_t writeTime;
    pa_sample_format_t format;
    BufferAttr *bufferAttr;
    int32_t processLen;
    size_t processSize;
};

static void UserdataFree(struct Userdata *u);
static int32_t PrepareDevice(struct Userdata *u, const char* filePath);

// BEGIN Utility functions
#define FLOAT_EPS 1e-9f
#define MEMBLOCKQ_MAXLENGTH (16*1024*16)
#define OFFSET_BIT_24 3
#define BIT_DEPTH_TWO 2
#define BIT_8 8
#define BIT_16 16
#define BIT_24 24
#define BIT_32 32
static uint32_t Read24Bit(const uint8_t *p)
{
    return ((uint32_t) p[BIT_DEPTH_TWO] << BIT_16) | ((uint32_t) p[1] << BIT_8) | ((uint32_t) p[0]);
}

static void Write24Bit(uint8_t *p, uint32_t u)
{
    p[BIT_DEPTH_TWO] = (uint8_t) (u >> BIT_16);
    p[1] = (uint8_t) (u >> BIT_8);
    p[0] = (uint8_t) u;
}

void ConvertFrom16BitToFloat(unsigned n, const int16_t *a, float *b)
{
    for (; n > 0; n--) {
        *(b++) = *(a++) * (1.0f / (1 << (BIT_16 - 1)));
    }
}

void ConvertFrom24BitToFloat(unsigned n, const uint8_t *a, float *b)
{
    for (; n > 0; n--) {
        int32_t s = Read24Bit(a) << BIT_8;
        *b = s * (1.0f / (1U << (BIT_32 - 1)));
        a += OFFSET_BIT_24;
        b++;
    }
}

void ConvertFrom32BitToFloat(unsigned n, const int32_t *a, float *b)
{
    for (; n > 0; n--) {
        *(b++) = *(a++) * (1.0f / (1U << (BIT_32 - 1)));
    }
}

float CapMax(float v)
{
    float value = v;
    if (v > 1.0f) {
        value = 1.0f - FLOAT_EPS;
    } else if (v < -1.0f) {
        value = -1.0f + FLOAT_EPS;
    }
    return value;
}

void ConvertFromFloatTo16Bit(unsigned n, const float *a, int16_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1 << (BIT_16 - 1));
        *(b++) = (int16_t) v;
    }
}

void ConvertFromFloatTo24Bit(unsigned n, const float *a, uint8_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        Write24Bit(b, ((uint32_t) v) >> BIT_8);
        a++;
        b += OFFSET_BIT_24;
    }
}

void ConvertFromFloatTo32Bit(unsigned n, const float *a, int32_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        *(b++) = (int32_t) v;
    }
}

static void ConvertToFloat(pa_sample_format_t format, unsigned n, void *src, float *dst)
{
    pa_assert(src);
    pa_assert(dst);
    int32_t ret;
    switch (format) {
        case PA_SAMPLE_S16LE:
            ConvertFrom16BitToFloat(n, src, dst);
            break;
        case PA_SAMPLE_S24LE:
            ConvertFrom24BitToFloat(n, src, dst);
            break;
        case PA_SAMPLE_S32LE:
            ConvertFrom32BitToFloat(n, src, dst);
            break;
        default:
            ret = memcpy_s(dst, n, src, n);
            if (ret != 0) {
                float *srcFloat = (float *)src;
                for (uint32_t i = 0; i < n; i++) {
                    dst[i] = srcFloat[i];
                }
            }
            break;
    }
}

static void ConvertFromFloat(pa_sample_format_t format, unsigned n, float *src, void *dst)
{
    pa_assert(src);
    pa_assert(dst);
    int32_t ret;
    switch (format) {
        case PA_SAMPLE_S16LE:
            ConvertFromFloatTo16Bit(n, src, dst);
            break;
        case PA_SAMPLE_S24LE:
            ConvertFromFloatTo24Bit(n, src, dst);
            break;
        case PA_SAMPLE_S32LE:
            ConvertFromFloatTo32Bit(n, src, dst);
            break;
        default:
            ret = memcpy_s(dst, n, src, n);
            if (ret != 0) {
                float *dstFloat = (float *)dst;
                for (uint32_t i = 0; i < n; i++) {
                    dstFloat[i] = src[i];
                }
            }
            break;
    }
}

static ssize_t RenderWrite(struct Userdata *u, pa_memchunk *pchunk)
{
    size_t index, length;
    ssize_t count = 0;
    void *p = NULL;

    pa_assert(pchunk);

    index = pchunk->index;
    length = pchunk->length;
    p = pa_memblock_acquire(pchunk->memblock);
    pa_assert(p);

    while (true) {
        uint64_t writeLen = 0;

        int32_t ret = u->sinkAdapter->RendererRenderFrame(u->sinkAdapter, ((char *)p + index),
            (uint64_t)length, &writeLen);
        if (writeLen > length) {
            AUDIO_ERR_LOG("Error writeLen > actual bytes. Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        }
        if (writeLen == 0) {
            AUDIO_ERR_LOG("Failed to render Length: %{public}zu, Written: %{public}" PRIu64 " bytes, %{public}d ret",
                length, writeLen, ret);
            count = -1 - count;
            break;
        } else {
            count += writeLen;
            index += writeLen;
            length -= writeLen;
            if (length <= 0) {
                break;
            }
        }
    }
    pa_memblock_release(pchunk->memblock);
    pa_memblock_unref(pchunk->memblock);

    return count;
}

static ssize_t TestModeRenderWrite(struct Userdata *u, pa_memchunk *pchunk)
{
    size_t index, length;
    ssize_t count = 0;
    void *p = NULL;

    pa_assert(pchunk);

    index = pchunk->index;
    length = pchunk->length;
    p = pa_memblock_acquire(pchunk->memblock);
    pa_assert(p);

    if (*((int32_t*)p) > 0) {
        AUDIO_DEBUG_LOG("RenderWrite Write: %{public}d", ++u->writeCount);
    }
    AUDIO_DEBUG_LOG("RenderWrite Write renderCount: %{public}d", ++u->renderCount);

    while (true) {
        uint64_t writeLen = 0;

        int32_t ret = u->sinkAdapter->RendererRenderFrame(u->sinkAdapter, ((char *)p + index),
            (uint64_t)length, &writeLen);
        if (writeLen > length) {
            AUDIO_ERR_LOG("Error writeLen > actual bytes. Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        }
        if (writeLen == 0) {
            AUDIO_ERR_LOG("Failed to render Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        } else {
            count += writeLen;
            index += writeLen;
            length -= writeLen;
            if (length <= 0) {
                break;
            }
        }
    }
    pa_memblock_release(pchunk->memblock);
    pa_memblock_unref(pchunk->memblock);

    return count;
}

bool IsInnerCapturer(pa_sink_input *sinkIn, struct Userdata *u)
{
    pa_assert(sinkIn);
    pa_assert(u);

    if (u != NULL && !GetInnerCapturerState()) {
        return false;
    }

    const char *usageStr = pa_proplist_gets(sinkIn->proplist, "stream.usage");
    const char *privacyTypeStr = pa_proplist_gets(sinkIn->proplist, "stream.privacyType");
    int32_t usage = -1;
    int32_t privacyType = -1;
    bool usageSupport = false;
    bool privacySupport = true;

    if (privacyTypeStr != NULL) {
        pa_atoi(privacyTypeStr, &privacyType);
        privacySupport = IsPrivacySupportInnerCapturer(privacyType);
    }

    if (usageStr != NULL) {
        pa_atoi(usageStr, &usage);
        usageSupport = IsStreamSupportInnerCapturer(usage);
    }
    return privacySupport && usageSupport;
}

static unsigned SinkRenderPrimaryClusterCap(pa_sink *si, size_t *length, pa_mix_info *infoIn, unsigned maxinfo)
{
    pa_sink_input *sinkIn;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(infoIn);

    unsigned n = 0;
    void *state = NULL;
    size_t mixlength = *length;
    struct Userdata *u = si->userdata;
    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL)) && maxinfo > 0) {
        if (IsInnerCapturer(sinkIn, u)) {
            pa_sink_input_assert_ref(sinkIn);

            pa_sink_input_peek(sinkIn, *length, &infoIn->chunk, &infoIn->volume);

            if (mixlength == 0 || infoIn->chunk.length < mixlength)
                mixlength = infoIn->chunk.length;

            if (pa_memblock_is_silence(infoIn->chunk.memblock)) {
                pa_memblock_unref(infoIn->chunk.memblock);
                continue;
            }

            infoIn->userdata = pa_sink_input_ref(sinkIn);
            pa_assert(infoIn->chunk.memblock);
            pa_assert(infoIn->chunk.length > 0);

            infoIn++;
            n++;
            maxinfo--;
        }
    }

    if (mixlength > 0) {
        *length = mixlength;
    }

    return n;
}

static void SinkRenderPrimaryMix(pa_sink *si, size_t length, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    if (n == 0) {
        if (chunkIn->length > length)
            chunkIn->length = length;

        pa_silence_memchunk(chunkIn, &si->sample_spec);
    } else if (n == 1) {
        pa_cvolume volume;

        if (chunkIn->length > length)
            chunkIn->length = length;

        pa_sw_cvolume_multiply(&volume, &si->thread_info.soft_volume, &infoIn[0].volume);

        if (si->thread_info.soft_muted || pa_cvolume_is_muted(&volume)) {
            pa_silence_memchunk(chunkIn, &si->sample_spec);
        } else {
            pa_memchunk tmpChunk;

            tmpChunk = infoIn[0].chunk;
            pa_memblock_ref(tmpChunk.memblock);

            if (tmpChunk.length > length)
                tmpChunk.length = length;

            if (!pa_cvolume_is_norm(&volume)) {
                pa_memchunk_make_writable(&tmpChunk, 0);
                pa_volume_memchunk(&tmpChunk, &si->sample_spec, &volume);
            }

            pa_memchunk_memcpy(chunkIn, &tmpChunk);
            pa_memblock_unref(tmpChunk.memblock);
        }
    } else {
        void *ptr;

        ptr = pa_memblock_acquire(chunkIn->memblock);

        chunkIn->length = pa_mix(infoIn, n,
                                (uint8_t*) ptr + chunkIn->index, length,
                                &si->sample_spec,
                                &si->thread_info.soft_volume,
                                si->thread_info.soft_muted);

        pa_memblock_release(chunkIn->memblock);
    }
}

static void SinkRenderPrimaryInputsDropCap(pa_sink *si, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);

    /* We optimize for the case where the order of the inputs has not changed */

    pa_mix_info *infoCur = NULL;
    bool isCaptureSilently = IsCaptureSilently();
    pa_sink_input *sceneSinkInput;
    for (int32_t k = 0; k < n; k++) {
        if (isCaptureSilently) {
            sceneSinkInput = infoIn[k].userdata;
            pa_sink_input_assert_ref(sceneSinkInput);
            pa_sink_input_drop(sceneSinkInput, chunkIn->length);
        }

        infoCur = infoIn + k;
        if (infoCur) {
            if (infoCur->chunk.memblock) {
                pa_memblock_unref(infoCur->chunk.memblock);
                pa_memchunk_reset(&infoCur->chunk);
            }

            pa_sink_input_unref(infoCur->userdata);

            if (isCaptureSilently) {
                infoCur->userdata = NULL;
            }
        }
    }
}

int32_t SinkRenderPrimaryPeekCap(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_mix_info infoIn[MAX_MIX_CHANNELS];
    unsigned n;
    size_t length, blockSizeMax;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(s->thread_info.state));
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    length = chunkIn->length;
    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    n = SinkRenderPrimaryClusterCap(si, &length, infoIn, MAX_MIX_CHANNELS);
    SinkRenderPrimaryMix(si, length, infoIn, n, chunkIn);

    SinkRenderPrimaryInputsDropCap(si, infoIn, n, chunkIn);
    pa_sink_unref(si);

    return n;
}

int32_t SinkRenderPrimaryGetDataCap(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_memchunk chunk;
    size_t l, d;
    int32_t nSinkInput;
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    l = chunkIn->length;
    d = 0;
    while (l > 0) {
        chunk = *chunkIn;
        chunk.index += d;
        chunk.length -= d;

        nSinkInput = SinkRenderPrimaryPeekCap(si, &chunk);

        d += chunk.length;
        l -= chunk.length;
    }

    pa_sink_unref(si);

    return nSinkInput;
}

static void SinkRenderPrimaryInputsDrop(pa_sink *si, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    unsigned nUnreffed = 0;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);

    /* We optimize for the case where the order of the inputs has not changed */
    pa_mix_info *infoCur = NULL;
    pa_sink_input *sceneSinkInput;
    for (int32_t k = 0; k < n; k++) {
        sceneSinkInput = infoIn[k].userdata;
        pa_sink_input_assert_ref(sceneSinkInput);

        /* Drop read data */
        pa_sink_input_drop(sceneSinkInput, chunkIn->length);
        infoCur = infoIn + k;
        if (infoCur) {
            if (infoCur->chunk.memblock) {
                pa_memblock_unref(infoCur->chunk.memblock);
                pa_memchunk_reset(&infoCur->chunk);
            }

            pa_sink_input_unref(infoCur->userdata);
            infoCur->userdata = NULL;

            nUnreffed += 1;
        }
    }
    /* Now drop references to entries that are included in the
     * pa_mix_info array but don't exist anymore */

    if (nUnreffed < n) {
        for (; n > 0; infoIn++, n--) {
            if (infoIn->userdata)
                pa_sink_input_unref(infoIn->userdata);
            if (infoIn->chunk.memblock)
                pa_memblock_unref(infoIn->chunk.memblock);
        }
    }
}

static unsigned SinkRenderPrimaryCluster(pa_sink *si, size_t *length, pa_mix_info *infoIn,
    unsigned maxinfo, char *sceneType)
{
    pa_sink_input *sinkIn;
    unsigned n = 0;
    void *state = NULL;
    size_t mixlength = *length;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(infoIn);

    struct Userdata *u = si->userdata;
    bool isCaptureSilently = IsCaptureSilently();
    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL)) && maxinfo > 0) {
        const char *sinkSceneType = pa_proplist_gets(sinkIn->proplist, "scene.type");
        const char *sinkSceneMode = pa_proplist_gets(sinkIn->proplist, "scene.mode");
        bool existFlag = EffectChainManagerExist(sinkSceneType, sinkSceneMode);
        if (IsInnerCapturer(sinkIn, u) && isCaptureSilently) {
            continue;
        } else if ((pa_safe_streq(sinkSceneType, sceneType) && existFlag) ||
            (pa_safe_streq(sceneType, "EFFECT_NONE") && (!existFlag))) {
            pa_sink_input_assert_ref(sinkIn);

            pa_sink_input_peek(sinkIn, *length, &infoIn->chunk, &infoIn->volume);

            if (mixlength == 0 || infoIn->chunk.length < mixlength)
                mixlength = infoIn->chunk.length;

            if (pa_memblock_is_silence(infoIn->chunk.memblock)) {
                pa_memblock_unref(infoIn->chunk.memblock);
                continue;
            }

            infoIn->userdata = pa_sink_input_ref(sinkIn);
            pa_assert(infoIn->chunk.memblock);
            pa_assert(infoIn->chunk.length > 0);

            infoIn++;
            n++;
            maxinfo--;
        }
    }

    if (mixlength > 0) {
        *length = mixlength;
    }

    return n;
}

int32_t SinkRenderPrimaryPeek(pa_sink *si, pa_memchunk *chunkIn, char *sceneType)
{
    pa_mix_info info[MAX_MIX_CHANNELS];
    unsigned n;
    size_t length, blockSizeMax;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(s->thread_info.state));
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    length = chunkIn->length;
    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    n = SinkRenderPrimaryCluster(si, &length, info, MAX_MIX_CHANNELS, sceneType);
    SinkRenderPrimaryMix(si, length, info, n, chunkIn);

    SinkRenderPrimaryInputsDrop(si, info, n, chunkIn);
    pa_sink_unref(si);

    return n;
}

int32_t SinkRenderPrimaryGetData(pa_sink *si, pa_memchunk *chunkIn, char *sceneType)
{
    pa_memchunk chunk;
    size_t l, d;
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    pa_assert(chunkIn);
    pa_assert(chunkIn->memblock);
    pa_assert(chunkIn->length > 0);
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    l = chunkIn->length;
    d = 0;

    int32_t nSinkInput;
    while (l > 0) {
        chunk = *chunkIn;
        chunk.index += d;
        chunk.length -= d;

        nSinkInput = SinkRenderPrimaryPeek(si, &chunk, sceneType);

        d += chunk.length;
        l -= chunk.length;
    }

    pa_sink_unref(si);

    return nSinkInput;
}

static void SinkRenderPrimaryProcess(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    pa_memchunk capResult;
    capResult.memblock = pa_memblock_new(si->core->mempool, length);
    capResult.index = 0;
    capResult.length = length;
    SinkRenderPrimaryGetDataCap(si, &capResult);

    if (si->monitor_source && PA_SOURCE_IS_LINKED(si->monitor_source->thread_info.state)) {
        pa_source_post(si->monitor_source, &capResult);
    }
    char *sceneTypeSet[SCENE_TYPE_NUM] = {"SCENE_MUSIC", "SCENE_GAME", "SCENE_MOVIE",
        "SCENE_SPEECH", "SCENE_RING", "SCENE_OTHERS", "EFFECT_NONE"};
    struct Userdata *u;
    pa_assert_se(u = si->userdata);
    size_t memsetLen = sizeof(float) * DEFAULT_FRAMELEN * IN_CHANNEL_NUM_MAX;
    memset_s(u->bufferAttr->tempBufIn, memsetLen, 0, memsetLen);
    memset_s(u->bufferAttr->tempBufOut, memsetLen, 0, memsetLen);
    int32_t bitSize = pa_sample_size_of_format(u->format);
    int32_t frameLen = bitSize > 0 ? (int32_t)(length / bitSize) : 0;
    int32_t nSinkInput;
    chunkIn->memblock = pa_memblock_new(si->core->mempool, length);
    for (int32_t i = 0; i < SCENE_TYPE_NUM; i++) {
        chunkIn->index = 0;
        chunkIn->length = length;
        nSinkInput = SinkRenderPrimaryGetData(si, chunkIn, sceneTypeSet[i]);
        if (nSinkInput == 0) {
            continue;
        }
        chunkIn->index = 0;
        chunkIn->length = length;
        void *src = pa_memblock_acquire_chunk(chunkIn);

        ConvertToFloat(u->format, frameLen, src, u->bufferAttr->tempBufIn);
        memcpy_s(u->bufferAttr->bufIn, frameLen * sizeof(float), u->bufferAttr->tempBufIn, frameLen * sizeof(float));
        u->bufferAttr->frameLen = frameLen / u->bufferAttr->numChanOut;
        EffectChainManagerProcess(sceneTypeSet[i], u->bufferAttr);
        for (int32_t k = 0; k < frameLen; k++) {
            u->bufferAttr->tempBufOut[k] += u->bufferAttr->bufOut[k];
        }
        pa_memblock_release(chunkIn->memblock);
    }
    void *dst = pa_memblock_acquire_chunk(chunkIn);
    for (int32_t i = 0; i < frameLen; i++) {
        u->bufferAttr->tempBufOut[i] = u->bufferAttr->tempBufOut[i] > 0.99f ? 0.99f : u->bufferAttr->tempBufOut[i];
        u->bufferAttr->tempBufOut[i] = u->bufferAttr->tempBufOut[i] < -0.99f ? -0.99f : u->bufferAttr->tempBufOut[i];
    }
    ConvertFromFloat(u->format, frameLen, u->bufferAttr->tempBufOut, dst);

    chunkIn->index = 0;
    chunkIn->length = length;
    pa_memblock_release(chunkIn->memblock);
    pa_memblock_unref(capResult.memblock);
}

void SinkRenderPrimary(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    pa_assert(length > 0);
    pa_assert(pa_frame_aligned(length, &si->sample_spec));
    pa_assert(chunkIn);

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    pa_sink_ref(si);

    size_t blockSizeMax;

    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    pa_assert(pa_frame_aligned(length, &si->sample_spec));
    pa_assert(chunkIn);

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        chunkIn->memblock = pa_memblock_ref(si->silence.memblock);
        chunkIn->index = si->silence.index;
        chunkIn->length = PA_MIN(si->silence.length, length);
        return;
    }

    if (length <= 0)
        length = pa_frame_align(MIX_BUFFER_LENGTH, &si->sample_spec);

    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    SinkRenderPrimaryProcess(si, length, chunkIn);

    pa_sink_unref(si);
}

static void ProcessRenderUseTiming(struct Userdata *u, pa_usec_t now)
{
    pa_assert(u);

    // Fill the buffer up the latency size
    pa_memchunk chunk;

    // Change from pa_sink_render to pa_sink_render_full for alignment issue in 3516
    SinkRenderPrimary(u->sink, u->sink->thread_info.max_request, &chunk);
    pa_assert(chunk.length > 0);

    pa_asyncmsgq_post(u->dq, NULL, HDI_RENDER, NULL, 0, &chunk, NULL);
    u->timestamp += pa_bytes_to_usec(chunk.length, &u->sink->sample_spec);
}

static void ThreadFuncRendererTimer(void *userdata)
{
    // set audio thread priority
    ScheduleReportData(getpid(), gettid(), "pulseaudio");

    struct Userdata *u = userdata;

    pa_assert(u);

    pa_thread_mq_install(&u->thread_mq);

    u->timestamp = pa_rtclock_now();

    while (true) {
        pa_usec_t now = 0;
        int32_t ret;
        
        bool flag = (u->render_in_idle_state && PA_SINK_IS_OPENED(u->sink->thread_info.state)) ||
            (!u->render_in_idle_state && PA_SINK_IS_RUNNING(u->sink->thread_info.state));
        if (flag) {
            now = pa_rtclock_now();
        }

        if (PA_UNLIKELY(u->sink->thread_info.rewind_requested)) {
            pa_sink_process_rewind(u->sink, 0);
        }

        if (flag) {
            if (u->timestamp <= now && pa_atomic_load(&u->dflag) == 0) {
                pa_atomic_add(&u->dflag, 1);
                ProcessRenderUseTiming(u, now);
            }
            pa_usec_t blockTime = pa_bytes_to_usec(u->sink->thread_info.max_request, &u->sink->sample_spec);
            int64_t sleepForUsec = PA_MIN(blockTime - (pa_rtclock_now() - now), u->writeTime);
            sleepForUsec = PA_MAX(sleepForUsec, 0);
            pa_rtpoll_set_timer_relative(u->rtpoll, (pa_usec_t)sleepForUsec);
        } else {
            pa_rtpoll_set_timer_disabled(u->rtpoll);
        }

        // Hmm, nothing to do. Let's sleep
        if ((ret = pa_rtpoll_run(u->rtpoll)) < 0) {
            // If this was no regular exit from the loop we have to continue
            // processing messages until we received PA_MESSAGE_SHUTDOWN
            pa_asyncmsgq_post(u->thread_mq.outq, PA_MSGOBJECT(u->core), PA_CORE_MESSAGE_UNLOAD_MODULE,
                u->module, 0, NULL, NULL);
            pa_asyncmsgq_wait_for(u->thread_mq.inq, PA_MESSAGE_SHUTDOWN);
            break;
        }

        if (ret == 0) {
            AUDIO_INFO_LOG("Thread (use timing) shutting down");
            break;
        }
    }
}

static void ThreadFuncWriteHDI(void *userdata)
{
    // set audio thread priority
    ScheduleReportData(getpid(), gettid(), "pulseaudio");

    struct Userdata *u = userdata;
    pa_assert(u);

    int32_t quit = 0;

    do {
        int32_t code = 0;
        pa_memchunk chunk;

        pa_assert_se(pa_asyncmsgq_get(u->dq, NULL, &code, NULL, NULL, &chunk, 1) == 0);

        switch (code) {
            case HDI_RENDER: {
                pa_usec_t now = pa_rtclock_now();
                if (RenderWrite(u, &chunk) < 0) {
                    u->bytes_dropped += chunk.length;
                    AUDIO_ERR_LOG("RenderWrite failed");
                }
                if (pa_atomic_load(&u->dflag) == 1) {
                    pa_atomic_sub(&u->dflag, 1);
                }
                u->writeTime = pa_rtclock_now() - now;
                break;
            }
            case QUIT:
                quit = 1;
                break;
            default:
                break;
        }
        pa_asyncmsgq_done(u->dq, 0);
    } while (!quit);
}

static void TestModeThreadFuncWriteHDI(void *userdata)
{
    // set audio thread priority
    ScheduleReportData(getpid(), gettid(), "pulseaudio");

    struct Userdata *u = userdata;
    pa_assert(u);

    int32_t quit = 0;

    do {
        int32_t code = 0;
        pa_memchunk chunk;

        pa_assert_se(pa_asyncmsgq_get(u->dq, NULL, &code, NULL, NULL, &chunk, 1) == 0);

        switch (code) {
            case HDI_RENDER:
                if (TestModeRenderWrite(u, &chunk) < 0) {
                    u->bytes_dropped += chunk.length;
                    AUDIO_ERR_LOG("TestModeRenderWrite failed");
                }
                if (pa_atomic_load(&u->dflag) == 1) {
                    pa_atomic_sub(&u->dflag, 1);
                }
                break;
            case QUIT:
                quit = 1;
                break;
            default:
                break;
        }
        pa_asyncmsgq_done(u->dq, 0);
    } while (!quit);
}

static void SinkUpdateRequestedLatencyCb(pa_sink *s)
{
    struct Userdata *u = NULL;
    size_t nbytes;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    u->block_usec = pa_sink_get_requested_latency_within_thread(s);

    if (u->block_usec == (pa_usec_t) - 1)
        u->block_usec = s->thread_info.max_latency;

    nbytes = pa_usec_to_bytes(u->block_usec, &s->sample_spec);
    pa_sink_set_max_request_within_thread(s, nbytes);
}

static int32_t SinkProcessMsg(pa_msgobject *o, int32_t code, void *data, int64_t offset,
                          pa_memchunk *chunk)
{
    AUDIO_DEBUG_LOG("SinkProcessMsg: code: %{public}d", code);
    struct Userdata *u = PA_SINK(o)->userdata;
    pa_assert(u);

    switch (code) {
        case PA_SINK_MESSAGE_GET_LATENCY: {
            if (u->sink_latency) {
                *((uint64_t *)data) = u->sink_latency * PA_USEC_PER_MSEC;
            } else {
                uint64_t latency;
                uint32_t hdiLatency;

                // Tries to fetch latency from HDI else will make an estimate based
                // on samples to be rendered based on the timestamp and current time
                if (u->sinkAdapter->RendererSinkGetLatency(u->sinkAdapter, &hdiLatency) == 0) {
                    latency = (PA_USEC_PER_MSEC * hdiLatency);
                } else {
                    pa_usec_t now = pa_rtclock_now();
                    latency = (now - u->timestamp);
                }

                *((uint64_t *)data) = latency;
            }
            return 0;
        }
        default:
            break;
    }
    return pa_sink_process_msg(o, code, data, offset, chunk);
}

static char *GetStateInfo(pa_sink_state_t state)
{
    switch (state) {
        case PA_SINK_INVALID_STATE:
            return "INVALID";
        case PA_SINK_RUNNING:
            return "RUNNING";
        case PA_SINK_IDLE:
            return "IDLE";
        case PA_SINK_SUSPENDED:
            return "SUSPENDED";
        case PA_SINK_INIT:
            return "INIT";
        case PA_SINK_UNLINKED:
            return "UNLINKED";
        default:
            return "error state";
    }
}

static int32_t RemoteSinkStateChange(pa_sink *s, pa_sink_state_t newState)
{
    struct Userdata *u = s->userdata;
    if (s->thread_info.state == PA_SINK_INIT && newState == PA_SINK_IDLE) {
        AUDIO_INFO_LOG("First start.");
    }

    if (s->thread_info.state == PA_SINK_SUSPENDED && PA_SINK_IS_OPENED(newState)) {
        u->timestamp = pa_rtclock_now();
        if (u->isHDISinkStarted) {
            return 0;
        }

        if (u->sinkAdapter->RendererSinkStart(u->sinkAdapter)) {
            AUDIO_ERR_LOG("audiorenderer control start failed!");
        } else {
            u->isHDISinkStarted = true;
            u->render_in_idle_state = 1; // enable to reduce noise from idle to running.
            u->writeCount = 0;
            u->renderCount = 0;
            AUDIO_INFO_LOG("Successfully restarted remote renderer");
        }
    }
    if (PA_SINK_IS_OPENED(s->thread_info.state) && newState == PA_SINK_SUSPENDED) {
        // Continuously dropping data (clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("HDI-sink continuously dropping data - clear statistics (%zu -> 0 bytes dropped)",
                           u->bytes_dropped);
            u->bytes_dropped = 0;
        }

        if (u->isHDISinkStarted) {
            u->sinkAdapter->RendererSinkStop(u->sinkAdapter);
            AUDIO_INFO_LOG("Stopped HDI renderer");
            u->isHDISinkStarted = false;
        }
    }

    return 0;
}

// Called from the IO thread.
static int32_t SinkSetStateInIoThreadCb(pa_sink *s, pa_sink_state_t newState,
                                    pa_suspend_cause_t newSuspendCause)
{
    struct Userdata *u = NULL;

    pa_assert(s);
    pa_assert_se(u = s->userdata);

    AUDIO_INFO_LOG("Sink[%{public}s] state change:[%{public}s]-->[%{public}s]",
        GetDeviceClass(u->sinkAdapter->deviceClass), GetStateInfo(s->thread_info.state), GetStateInfo(newState));

    if (!strcmp(GetDeviceClass(u->sinkAdapter->deviceClass), DEVICE_CLASS_REMOTE)) {
        return RemoteSinkStateChange(s, newState);
    }

    if (!strcmp(GetDeviceClass(u->sinkAdapter->deviceClass), DEVICE_CLASS_A2DP)) {
        if (s->thread_info.state == PA_SINK_IDLE && newState == PA_SINK_RUNNING) {
            u->sinkAdapter->RendererSinkResume(u->sinkAdapter);
        } else if (s->thread_info.state == PA_SINK_RUNNING && newState == PA_SINK_IDLE) {
            u->sinkAdapter->RendererSinkPause(u->sinkAdapter);
        }
    }

    if (s->thread_info.state == PA_SINK_SUSPENDED || s->thread_info.state == PA_SINK_INIT) {
        if (!PA_SINK_IS_OPENED(newState)) {
            return 0;
        }

        u->timestamp = pa_rtclock_now();
        if (u->isHDISinkStarted) {
            return 0;
        }

        AUDIO_INFO_LOG("Restart with rate:%{public}d,channels:%{public}d", u->ss.rate, u->ss.channels);
        if (u->sinkAdapter->RendererSinkStart(u->sinkAdapter)) {
            AUDIO_ERR_LOG("audiorenderer control start failed!");
            u->sinkAdapter->RendererSinkDeInit(u->sinkAdapter);
        } else {
            u->isHDISinkStarted = true;
            u->writeCount = 0;
            u->renderCount = 0;
            AUDIO_INFO_LOG("Successfully restarted HDI renderer");
        }
    } else if (PA_SINK_IS_OPENED(s->thread_info.state)) {
        if (newState != PA_SINK_SUSPENDED) {
            return 0;
        }
        // Continuously dropping data (clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("HDI-sink continuously dropping data - clear statistics (%zu -> 0 bytes dropped)",
                           u->bytes_dropped);
            u->bytes_dropped = 0;
        }

        if (u->isHDISinkStarted) {
            u->sinkAdapter->RendererSinkStop(u->sinkAdapter);
            AUDIO_INFO_LOG("Stopped HDI renderer");
            u->isHDISinkStarted = false;
        }
    }

    return 0;
}

static enum SampleFormat ConvertToFwkFormat(pa_sample_format_t format)
{
    enum SampleFormat fwkFormat;
    switch (format) {
        case PA_SAMPLE_U8:
            fwkFormat = SAMPLE_U8;
            break;
        case PA_SAMPLE_S16LE:
            fwkFormat = SAMPLE_S16LE;
            break;
        case PA_SAMPLE_S24LE:
            fwkFormat = SAMPLE_S24LE;
            break;
        case PA_SAMPLE_S32LE:
            fwkFormat = SAMPLE_S32LE;
            break;
        default:
            fwkFormat = INVALID_WIDTH;
            break;
    }

    return fwkFormat;
}

static int32_t PrepareDevice(struct Userdata *u, const char* filePath)
{
    SinkAttr sample_attrs;
    int32_t ret;

    enum SampleFormat format = ConvertToFwkFormat(u->ss.format);
    sample_attrs.format = format;
    sample_attrs.sampleFmt = format;
    AUDIO_DEBUG_LOG("audiorenderer format: %d", sample_attrs.format);
    sample_attrs.adapterName = u->adapterName;
    sample_attrs.openMicSpeaker = u->open_mic_speaker;
    sample_attrs.sampleRate = u->ss.rate;
    sample_attrs.channel = u->ss.channels;
    sample_attrs.volume = MAX_SINK_VOLUME_LEVEL;
    sample_attrs.filePath = filePath;
    sample_attrs.deviceNetworkId = u->deviceNetworkId;
    sample_attrs.deviceType =  u->deviceType;

    ret = u->sinkAdapter->RendererSinkInit(u->sinkAdapter, &sample_attrs);
    if (ret != 0) {
        AUDIO_ERR_LOG("audiorenderer Init failed!");
        return -1;
    }

    // call start in io thread for remote device.
    if (strcmp(GetDeviceClass(u->sinkAdapter->deviceClass), DEVICE_CLASS_REMOTE)) {
        ret = u->sinkAdapter->RendererSinkStart(u->sinkAdapter);
    }

    if (ret != 0) {
        AUDIO_ERR_LOG("audiorenderer control start failed!");
        u->sinkAdapter->RendererSinkDeInit(u->sinkAdapter);
        return -1;
    }

    return 0;
}

static void PaHdiSinkUserdataInit(struct Userdata *u)
{
    u->format = u->ss.format;
    u->processLen = IN_CHANNEL_NUM_MAX * DEFAULT_FRAMELEN;
    u->processSize = u->processLen * sizeof(float);
    u->bufferAttr = pa_xnew0(BufferAttr, 1);
    pa_assert_se(u->bufferAttr->bufIn = (float *)malloc(u->processSize));
    pa_assert_se(u->bufferAttr->bufOut = (float *)malloc(u->processSize));
    pa_assert_se(u->bufferAttr->tempBufIn = (float *)malloc(u->processSize));
    pa_assert_se(u->bufferAttr->tempBufOut = (float *)malloc(u->processSize));
    u->bufferAttr->samplingRate = u->ss.rate;
    u->bufferAttr->frameLen = DEFAULT_FRAMELEN;
    u->bufferAttr->numChanIn = u->ss.channels;
    u->bufferAttr->numChanOut = u->ss.channels;
}

static pa_sink* PaHdiSinkInit(struct Userdata *u, pa_modargs *ma, const char *driver)
{
    // set audio thread priority
    ScheduleReportData(getpid(), gettid(), "pulseaudio");

    pa_sink_new_data data;
    pa_module *m;
    pa_sink *sink = NULL;

    m = u->module;
    u->ss = m->core->default_sample_spec;
    u->map = m->core->default_channel_map;
    if (pa_modargs_get_sample_spec_and_channel_map(ma, &u->ss, &u->map, PA_CHANNEL_MAP_DEFAULT) < 0) {
        AUDIO_ERR_LOG("Failed to parse sample specification and channel map");
        goto fail;
    }

    AUDIO_INFO_LOG("Initializing HDI rendering device with rate: %{public}d, channels: %{public}d",
        u->ss.rate, u->ss.channels);
    if (PrepareDevice(u, pa_modargs_get_value(ma, "file_path", "")) < 0)
        goto fail;

    u->isHDISinkStarted = true;
    AUDIO_DEBUG_LOG("Initialization of HDI rendering device[%{public}s] completed", u->adapterName);
    pa_sink_new_data_init(&data);
    data.driver = driver;
    data.module = m;

    PaHdiSinkUserdataInit(u);
    pa_sink_new_data_set_name(&data, pa_modargs_get_value(ma, "sink_name", DEFAULT_SINK_NAME));
    pa_sink_new_data_set_sample_spec(&data, &u->ss);
    pa_sink_new_data_set_channel_map(&data, &u->map);
    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_STRING,
        (u->adapterName ? u->adapterName : DEFAULT_AUDIO_DEVICE_NAME));
    pa_proplist_setf(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "HDI sink is %s",
        (u->adapterName ? u->adapterName : DEFAULT_AUDIO_DEVICE_NAME));

    if (pa_modargs_get_proplist(ma, "sink_properties", data.proplist, PA_UPDATE_REPLACE) < 0) {
        AUDIO_ERR_LOG("Invalid properties");
        pa_sink_new_data_done(&data);
        goto fail;
    }

    sink = pa_sink_new(m->core, &data,
                       PA_SINK_HARDWARE | PA_SINK_LATENCY | PA_SINK_DYNAMIC_LATENCY);
    pa_sink_new_data_done(&data);

    return sink;

fail:
    return NULL;
}

pa_sink *PaHdiSinkNew(pa_module *m, pa_modargs *ma, const char *driver)
{
    struct Userdata *u = NULL;
    char *paThreadName = NULL;
    char *hdiThreadName = NULL;

    pa_assert(m);
    pa_assert(ma);

    u = pa_xnew0(struct Userdata, 1);
    pa_assert(u);
    u->core = m->core;
    u->module = m;

    pa_memchunk_reset(&u->memchunk);
    u->rtpoll = pa_rtpoll_new();

    if (pa_thread_mq_init(&u->thread_mq, m->core->mainloop, u->rtpoll) < 0) {
        AUDIO_ERR_LOG("pa_thread_mq_init() failed.");
        goto fail;
    }

    AUDIO_DEBUG_LOG("Load sink adapter");
    int32_t ret = LoadSinkAdapter(pa_modargs_get_value(ma, "device_class", DEFAULT_DEVICE_CLASS),
        pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID), &u->sinkAdapter);
    if (ret) {
        AUDIO_ERR_LOG("Load adapter failed");
        goto fail;
    }
    if (pa_modargs_get_value_u32(ma, "fixed_latency", &u->fixed_latency) < 0) {
        AUDIO_ERR_LOG("Failed to parse fixed latency argument.");
        goto fail;
    }
    if (pa_modargs_get_value_s32(ma, "device_type", &u->deviceType) < 0) {
        AUDIO_ERR_LOG("Failed to parse deviceType argument.");
        goto fail;
    }

    u->adapterName = pa_modargs_get_value(ma, "adapter_name", DEFAULT_DEVICE_CLASS);
    u->sink_latency = 0;
    if (pa_modargs_get_value_u32(ma, "sink_latency", &u->sink_latency) < 0) {
        AUDIO_ERR_LOG("No sink_latency argument.");
    }

    u->deviceNetworkId = pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID);

    if (pa_modargs_get_value_u32(ma, "render_in_idle_state", &u->render_in_idle_state) < 0) {
        AUDIO_ERR_LOG("Failed to parse render_in_idle_state  argument.");
        goto fail;
    }

    if (pa_modargs_get_value_u32(ma, "open_mic_speaker", &u->open_mic_speaker) < 0) {
        AUDIO_ERR_LOG("Failed to parse open_mic_speaker argument.");
        goto fail;
    }

    u->test_mode_on = false;
    if (pa_modargs_get_value_boolean(ma, "test_mode_on", &u->test_mode_on) < 0) {
        AUDIO_INFO_LOG("No test_mode_on arg. Normal mode it is.");
    }

    pa_atomic_store(&u->dflag, 0);
    u->dq = pa_asyncmsgq_new(0);

    u->sink = PaHdiSinkInit(u, ma, driver);
    if (!u->sink) {
        AUDIO_ERR_LOG("Failed to create sink object");
        goto fail;
    }

    u->sink->parent.process_msg = SinkProcessMsg;
    u->sink->set_state_in_io_thread = SinkSetStateInIoThreadCb;
    if (!u->fixed_latency) {
        u->sink->update_requested_latency = SinkUpdateRequestedLatencyCb;
    }
    u->sink->userdata = u;

    pa_sink_set_asyncmsgq(u->sink, u->thread_mq.inq);
    pa_sink_set_rtpoll(u->sink, u->rtpoll);

    u->bytes_dropped = 0;
    u->buffer_size = DEFAULT_BUFFER_SIZE;
    if (pa_modargs_get_value_u32(ma, "buffer_size", &u->buffer_size) < 0) {
        AUDIO_ERR_LOG("Failed to parse buffer_size argument.");
        goto fail;
    }

    u->block_usec = pa_bytes_to_usec(u->buffer_size, &u->sink->sample_spec);

    if (u->fixed_latency) {
        pa_sink_set_fixed_latency(u->sink, u->block_usec);
    } else {
        pa_sink_set_latency_range(u->sink, 0, u->block_usec);
    }

    pa_sink_set_max_request(u->sink, u->buffer_size);

    paThreadName = "write-pa";
    if (!(u->thread = pa_thread_new(paThreadName, ThreadFuncRendererTimer, u))) {
        AUDIO_ERR_LOG("Failed to write-pa thread.");
        goto fail;
    }

    if (u->test_mode_on) {
        u->writeCount = 0;
        u->renderCount = 0;
        hdiThreadName = "test-mode-write-hdi";
        if (!(u->thread_hdi = pa_thread_new(hdiThreadName, TestModeThreadFuncWriteHDI, u))) {
            AUDIO_ERR_LOG("Failed to test-mode-write-hdi thread.");
            goto fail;
        }
    } else {
        hdiThreadName = "write-hdi";
        if (!(u->thread_hdi = pa_thread_new(hdiThreadName, ThreadFuncWriteHDI, u))) {
            AUDIO_ERR_LOG("Failed to write-hdi thread.");
            goto fail;
        }
    }

    u->writeTime = DEFAULT_WRITE_TIME;

    pa_sink_put(u->sink);

    return u->sink;
fail:
    UserdataFree(u);

    return NULL;
}

static void UserdataFree(struct Userdata *u)
{
    pa_assert(u);

    if (u->sink)
        pa_sink_unlink(u->sink);

    if (u->thread) {
        pa_asyncmsgq_send(u->thread_mq.inq, NULL, PA_MESSAGE_SHUTDOWN, NULL, 0, NULL);
        pa_thread_free(u->thread);
    }

    if (u->thread_hdi) {
        pa_asyncmsgq_post(u->dq, NULL, QUIT, NULL, 0, NULL, NULL);
        pa_thread_free(u->thread_hdi);
    }

    pa_thread_mq_done(&u->thread_mq);

    if (u->sink)
        pa_sink_unref(u->sink);

    if (u->memchunk.memblock)
        pa_memblock_unref(u->memchunk.memblock);

    if (u->rtpoll)
        pa_rtpoll_free(u->rtpoll);

    if (u->sinkAdapter) {
        u->sinkAdapter->RendererSinkStop(u->sinkAdapter);
        u->sinkAdapter->RendererSinkDeInit(u->sinkAdapter);
        UnLoadSinkAdapter(u->sinkAdapter);
    }

    pa_xfree(u);
}

void PaHdiSinkFree(pa_sink *s)
{
    struct Userdata *u = NULL;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    UserdataFree(u);
}
