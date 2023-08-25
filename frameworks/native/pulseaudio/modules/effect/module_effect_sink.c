/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/xmalloc.h>

#include <pulsecore/namereg.h>
#include <pulsecore/sink.h>
#include <pulsecore/module.h>
#include <pulsecore/core-util.h>
#include <pulsecore/modargs.h>
#include <pulsecore/log.h>
#include <pulsecore/rtpoll.h>

#include "securec.h"
#include "audio_effect_chain_adapter.h"
#include "audio_log.h"

PA_MODULE_AUTHOR("OpenHarmony");
PA_MODULE_DESCRIPTION(_("Effect sink"));
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "sink_name=<name for the sink> "
        "rate=<sample rate> "
);

struct userdata {
    pa_module *module;
    pa_sink *sink;
    pa_sink_input *sinkInput;
    char *sceneName;

    pa_sample_spec sampleSpec;
    pa_channel_map sinkMap;
    BufferAttr *bufferAttr;
    pa_memblockq *bufInQ;
    int32_t processLen;
    size_t processSize;
    pa_sample_format_t format;
};

static const char * const VALID_MODARGS[] = {
    "sink_name",
    "rate",
    "scene_name",
    NULL
};

/* Called from I/O thread context */
static int SinkProcessMsg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk)
{
    struct userdata *u = PA_SINK(o)->userdata;
    switch (code) {
        case PA_SINK_MESSAGE_GET_LATENCY:
            /* The sink is _put() before the sink input is, so let's
             * make sure we don't access it yet */
            if (!PA_SINK_IS_LINKED(u->sink->thread_info.state) ||
                !PA_SINK_INPUT_IS_LINKED(u->sinkInput->thread_info.state)) {
                *((int64_t*) data) = 0;
                return 0;
            }
            *((int64_t*) data) =
                /* Get the latency of the masterSink */
                pa_sink_get_latency_within_thread(u->sinkInput->sink, true) +
                /* Add the latency internal to our sink input on top */
                pa_bytes_to_usec(pa_memblockq_get_length(u->bufInQ), &u->sinkInput->sink->sample_spec) +
                pa_bytes_to_usec(pa_memblockq_get_length(u->sinkInput->thread_info.render_memblockq),
                    &u->sinkInput->sink->sample_spec);
            return 0;
        default:
            break;
    }

    return pa_sink_process_msg(o, code, data, offset, chunk);
}

/* Called from main context */
static int SinkSetStateInMainThread(pa_sink *s, pa_sink_state_t state, pa_suspend_cause_t suspend_cause)
{
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    if (!PA_SINK_IS_LINKED(state) || !PA_SINK_INPUT_IS_LINKED(u->sinkInput->state)) {
        return 0;
    }

    pa_sink_input_cork(u->sinkInput, state == PA_SINK_SUSPENDED);
    return 0;
}

/* Called from the IO thread. */
static int SinkSetStateInIoThreadCb(pa_sink *s, pa_sink_state_t new_state, pa_suspend_cause_t new_suspend_cause)
{
    struct userdata *u;

    pa_assert(s);
    pa_assert_se(u = s->userdata);

    /* When set to running or idle for the first time, request a rewind
     * of the masterSink to make sure we are heard immediately */
    if (PA_SINK_IS_OPENED(new_state) && s->thread_info.state == PA_SINK_INIT) {
        pa_log_debug("Requesting rewind due to state change.");
        pa_sink_input_request_rewind(u->sinkInput, 0, false, true, true);
    }

    return 0;
}

/* Called from I/O thread context */
static void SinkRequestRewind(pa_sink *s)
{
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    if (!PA_SINK_IS_LINKED(u->sink->thread_info.state) ||
        !PA_SINK_INPUT_IS_LINKED(u->sinkInput->thread_info.state)) {
            return;
    }

    size_t nbytes = s->thread_info.rewind_nbytes + pa_memblockq_get_length(u->bufInQ);
    pa_sink_input_request_rewind(u->sinkInput, nbytes, true, false, false);
}

/* Called from I/O thread context */
static void SinkUpdateRequestedLatency(pa_sink *s)
{
    struct userdata *u;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    if (!PA_SINK_IS_LINKED(u->sink->thread_info.state) ||
        !PA_SINK_INPUT_IS_LINKED(u->sinkInput->thread_info.state)) {
        return;
    }

    /* Just hand this one over to the masterSink */
    pa_sink_input_set_requested_latency_within_thread(u->sinkInput, pa_sink_get_requested_latency_within_thread(s));
}

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
    int ret;
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
            ret = memcpy_s(src, n, dst, n);
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
    int ret;
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
            ret = memcpy_s(src, n, dst, n);
            if (ret != 0) {
                float *dstFloat = (float *)dst;
                for (uint32_t i = 0; i < n; i++) {
                    dstFloat[i] = src[i];
                }
            }
            break;
    }
}

static size_t MemblockqMissing(pa_memblockq *bq)
{
    size_t l, tlength;
    pa_assert(bq);

    tlength = pa_memblockq_get_tlength(bq);
    if ((l = pa_memblockq_get_length(bq)) >= tlength) {
        return 0;
    }

    l = tlength - l;
    return l >= pa_memblockq_get_minreq(bq) ? l : 0;
}
// END Utility functions

static void EffectProcess(pa_sink_input *si, struct userdata *u)
{
    const char *sn;
    sn = pa_proplist_gets(si->proplist, "scene.name");
    if (sn != NULL) {
        char *sceneName = pa_sprintf_malloc("%s", sn);
        EffectChainManagerProcess(sceneName, u->bufferAttr);
    } else {
        EffectChainManagerProcess(si->origin_sink->name, u->bufferAttr);
    }
}

/* Called from I/O thread context */
static int SinkInputPopCb(pa_sink_input *si, size_t nbytes, pa_memchunk *chunk)
{
    struct userdata *u;
    size_t bytesMissing;
    pa_memchunk tchunk;

    pa_sink_input_assert_ref(si);
    pa_assert(chunk);
    pa_assert_se(u = si->userdata);

    if (!PA_SINK_IS_LINKED(u->sink->thread_info.state)) {
        return -1;
    }

    while ((bytesMissing = MemblockqMissing(u->bufInQ)) != 0) {
        pa_memchunk nchunk;
        pa_sink_render(u->sink, bytesMissing, &nchunk);
        pa_memblockq_push(u->bufInQ, &nchunk);
        pa_memblock_release(nchunk.memblock);
        pa_memblock_unref(nchunk.memblock);
    }

    if (!PA_SINK_IS_RUNNING(u->sink->thread_info.state)) {
        return -1;
    }
    
    size_t targetLength = pa_memblockq_get_tlength(u->bufInQ);
    int iterNum = pa_memblockq_get_length(u->bufInQ) / targetLength;
    chunk->index = 0;
    chunk->length = targetLength * iterNum;
    chunk->memblock = pa_memblock_new(si->sink->core->mempool, chunk->length);
    short *dst = pa_memblock_acquire_chunk(chunk);
    for (int k = 0; k < iterNum; k++) {
        if (pa_memblockq_peek_fixed_size(u->bufInQ, targetLength, &tchunk) != 0) {
            break;
        }
        if (tchunk.length < targetLength) {
            break;
        }
        pa_memblockq_drop(u->bufInQ, tchunk.length);

        void *src;
        float *bufIn, *bufOut;
        src = pa_memblock_acquire_chunk(&tchunk);
        bufIn = u->bufferAttr->bufIn;
        bufOut = u->bufferAttr->bufOut;
        
        ConvertToFloat(u->format, u->processLen, src, bufIn);
        pa_memblock_release(tchunk.memblock);
        pa_memblock_unref(tchunk.memblock);

        EffectProcess(si, u);

        ConvertFromFloat(u->format, u->processLen, bufOut, dst);
        
        dst += u->processSize;
    }

    pa_memblock_release(chunk->memblock);
    
    return 0;
}
// END QUEUE

/* Called from I/O thread context */
static void SinkInputProcessRewindCb(pa_sink_input *i, size_t nbytes)
{
    size_t amount = 0;
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    /* If the sink is not yet linked, there is nothing to rewind */
    if (!PA_SINK_IS_LINKED(u->sink->thread_info.state)) {
        return;
    }

    if (u->sink->thread_info.rewind_nbytes > 0) {
        size_t maxRewrite = nbytes + pa_memblockq_get_length(u->bufInQ);
        amount = PA_MIN(u->sink->thread_info.rewind_nbytes, maxRewrite);
        u->sink->thread_info.rewind_nbytes = 0;
        if (amount > 0) {
            pa_memblockq_seek(u->bufInQ, -(int64_t)amount, PA_SEEK_RELATIVE, true);
        }
    }

    pa_sink_process_rewind(u->sink, amount);
}

/* Called from I/O thread context */
static void SinkInputUpdateMaxRewindCb(pa_sink_input *i, size_t nbytes)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_set_max_rewind_within_thread(u->sink, nbytes);
}

/* Called from I/O thread context */
static void SinkInputUpdateMaxRequestCb(pa_sink_input *i, size_t nbytes)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_set_max_request_within_thread(u->sink, nbytes);
}

/* Called from I/O thread context */
static void SinkInputUpdateSinkLatencyRangeCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_set_latency_range_within_thread(u->sink, i->sink->thread_info.min_latency,
        i->sink->thread_info.max_latency);
}

/* Called from I/O thread context */
static void SinkInputUpdateSinkFixedLatencyCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_set_fixed_latency_within_thread(u->sink, i->sink->thread_info.fixed_latency);
}

/* Called from I/O thread context */
static void SinkInputAttachCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    pa_sink_set_rtpoll(u->sink, i->sink->thread_info.rtpoll);
    pa_sink_set_latency_range_within_thread(u->sink, i->sink->thread_info.min_latency,
        i->sink->thread_info.max_latency);
    pa_sink_set_fixed_latency_within_thread(u->sink, i->sink->thread_info.fixed_latency);
    pa_sink_set_max_request_within_thread(u->sink, pa_sink_input_get_max_request(i));

    pa_sink_set_max_rewind_within_thread(u->sink, pa_sink_input_get_max_rewind(i));

    if (PA_SINK_IS_LINKED(u->sink->thread_info.state)) {
        pa_sink_attach_within_thread(u->sink);
    }
}

/* Called from I/O thread context */
static void SinkInputDetachCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (PA_SINK_IS_LINKED(u->sink->thread_info.state)) {
        pa_sink_detach_within_thread(u->sink);
    }

    pa_sink_set_rtpoll(u->sink, NULL);
}

/* Called from main context */
static void SinkInputKillCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    /* The order here matters! We first kill the sink so that streams
     * can properly be moved away while the sink input is still connected
     * to the master. */
    pa_sink_input_cork(u->sinkInput, true);
    pa_sink_unlink(u->sink);
    pa_sink_input_unlink(u->sinkInput);

    pa_sink_input_unref(u->sinkInput);
    u->sinkInput = NULL;

    pa_sink_unref(u->sink);
    u->sink = NULL;

    pa_module_unload_request(u->module, true);
}

/* Called from main context */
static void SinkInputMovingCb(pa_sink_input *i, pa_sink *dest)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (!dest) {
        pa_sink_set_asyncmsgq(u->sink, NULL);
        return;
    }

    pa_sink_set_asyncmsgq(u->sink, dest->asyncmsgq);
    pa_sink_update_flags(u->sink, PA_SINK_LATENCY | PA_SINK_DYNAMIC_LATENCY, dest->flags);

    const char *k;
    pa_proplist *pl;
    pl = pa_proplist_new();
    k = pa_proplist_gets(dest->proplist, PA_PROP_DEVICE_DESCRIPTION);
    pa_proplist_setf(pl, PA_PROP_DEVICE_DESCRIPTION, "Remapped %s", k ? k : dest->name);
    pa_proplist_sets(pl, PA_PROP_DEVICE_MASTER_DEVICE, dest->name);
    pa_sink_update_proplist(u->sink, PA_UPDATE_REPLACE, pl);
    pa_proplist_free(pl);
}

int InitFail(pa_module *m, pa_modargs *ma)
{
    AUDIO_ERR_LOG("Failed to create effect sink");
    if (ma) {
        pa_modargs_free(ma);
    }
    pa__done(m);
    return -1;
}

int CreateSink(pa_module *m, pa_modargs *ma, pa_sink *masterSink, struct userdata *u)
{
    pa_sample_spec ss;
    pa_channel_map sinkMap;
    pa_sink_new_data sinkData;
    const char *scene;

    /* Create sink */
    ss = m->core->default_sample_spec;
    sinkMap = m->core->default_channel_map;
    if (pa_modargs_get_sample_spec_and_channel_map(ma, &ss, &sinkMap, PA_CHANNEL_MAP_DEFAULT) < 0) {
        AUDIO_ERR_LOG("Invalid sample format specification or channel map");
        return -1;
    }

    pa_sink_new_data_init(&sinkData);
    sinkData.driver = __FILE__;
    sinkData.module = m;
    if (!(sinkData.name = pa_xstrdup(pa_modargs_get_value(ma, "sink_name", NULL)))) {
        sinkData.name = pa_sprintf_malloc("%s.effected", masterSink->name);
    }

    scene = pa_modargs_get_value(ma, "scene_name", NULL);
    if (scene != NULL) {
        u->sceneName = pa_sprintf_malloc("%s", scene);
    }
    pa_sink_new_data_set_sample_spec(&sinkData, &ss);
    pa_sink_new_data_set_channel_map(&sinkData, &sinkMap);
    pa_proplist_sets(sinkData.proplist, PA_PROP_DEVICE_MASTER_DEVICE, masterSink->name);
    pa_proplist_sets(sinkData.proplist, PA_PROP_DEVICE_CLASS, "filter");
    pa_proplist_sets(sinkData.proplist, PA_PROP_DEVICE_STRING, "N/A");
    const char *k;
    k = pa_proplist_gets(masterSink->proplist, PA_PROP_DEVICE_DESCRIPTION);
    pa_proplist_setf(sinkData.proplist, PA_PROP_DEVICE_DESCRIPTION, "Remapped %s", k ? k : masterSink->name);

    u->sink = pa_sink_new(m->core, &sinkData, masterSink->flags & (PA_SINK_LATENCY | PA_SINK_DYNAMIC_LATENCY));
    pa_sink_new_data_done(&sinkData);

    if (!u->sink) {
        return -1;
    }

    u->sink->parent.process_msg = SinkProcessMsg;
    u->sink->set_state_in_main_thread = SinkSetStateInMainThread;
    u->sink->set_state_in_io_thread = SinkSetStateInIoThreadCb;
    u->sink->update_requested_latency = SinkUpdateRequestedLatency;
    u->sink->request_rewind = SinkRequestRewind;
    u->sink->userdata = u;
    u->sampleSpec = ss;
    u->sinkMap = sinkMap;

    pa_sink_set_asyncmsgq(u->sink, masterSink->asyncmsgq);
    return 0;
}

int CreateSinkInput(pa_module *m, pa_modargs *ma, pa_sink *masterSink, struct userdata *u)
{
    pa_resample_method_t resampleMethod = PA_RESAMPLER_TRIVIAL;
    bool remix = true;
    pa_sink_input_new_data sinkInputData;
    /* Create sink input */
    pa_sink_input_new_data_init(&sinkInputData);
    sinkInputData.driver = __FILE__;
    sinkInputData.module = m;
    pa_sink_input_new_data_set_sink(&sinkInputData, masterSink, false, true);
    sinkInputData.origin_sink = u->sink;
    const char *name = pa_sprintf_malloc("%s effected Stream", u->sink->name);
    pa_proplist_sets(sinkInputData.proplist, PA_PROP_MEDIA_NAME, name);
    pa_proplist_sets(sinkInputData.proplist, PA_PROP_MEDIA_ROLE, "filter");
    pa_proplist_sets(sinkInputData.proplist, "scene.type", "N/A");
    pa_proplist_sets(sinkInputData.proplist, "scene.mode", "N/A");
    if (u->sceneName != NULL) {
        pa_proplist_sets(sinkInputData.proplist, "scene.name", u->sceneName);
    }
    pa_sink_input_new_data_set_sample_spec(&sinkInputData, &u->sampleSpec);
    pa_sink_input_new_data_set_channel_map(&sinkInputData, &u->sinkMap);
    sinkInputData.flags = (remix ? 0 : PA_SINK_INPUT_NO_REMIX) | PA_SINK_INPUT_START_CORKED;
    sinkInputData.resample_method = resampleMethod;

    pa_sink_input_new(&u->sinkInput, m->core, &sinkInputData);
    pa_sink_input_new_data_done(&sinkInputData);

    if (!u->sinkInput) {
        return -1;
    }

    u->sinkInput->pop = SinkInputPopCb;
    u->sinkInput->process_rewind = SinkInputProcessRewindCb;
    u->sinkInput->update_max_rewind = SinkInputUpdateMaxRewindCb;
    u->sinkInput->update_max_request = SinkInputUpdateMaxRequestCb;
    u->sinkInput->update_sink_latency_range = SinkInputUpdateSinkLatencyRangeCb;
    u->sinkInput->update_sink_fixed_latency = SinkInputUpdateSinkFixedLatencyCb;
    u->sinkInput->attach = SinkInputAttachCb;
    u->sinkInput->detach = SinkInputDetachCb;
    u->sinkInput->kill = SinkInputKillCb;
    u->sinkInput->moving = SinkInputMovingCb;
    u->sinkInput->userdata = u;

    u->sink->input_to_master = u->sinkInput;
    return 0;
}

int ConfigSinkInput(struct userdata *u)
{
    // Set buffer attributes
    int32_t frameLen = EffectChainManagerGetFrameLen();
    u->format = u->sampleSpec.format;
    u->processLen = u->sampleSpec.channels * frameLen;
    u->processSize = u->processLen * sizeof(float);
    int ret;

    u->bufferAttr = pa_xnew0(BufferAttr, 1);
    pa_assert_se(u->bufferAttr->bufIn = (float *)malloc(u->processSize));
    pa_assert_se(u->bufferAttr->bufOut = (float *)malloc(u->processSize));
    u->bufferAttr->samplingRate = u->sampleSpec.rate;
    u->bufferAttr->frameLen = frameLen;
    u->bufferAttr->numChanIn = u->sampleSpec.channels;
    u->bufferAttr->numChanOut = u->sampleSpec.channels;
    if (u->sceneName != NULL) {
        ret = EffectChainManagerCreate(u->sceneName, u->bufferAttr);
    } else {
        ret = EffectChainManagerCreate(u->sink->name, u->bufferAttr);
    }
    if (ret != 0) {
        return -1;
    }

    int32_t bitSize = pa_sample_size_of_format(u->sampleSpec.format);
    size_t targetSize = u->sampleSpec.channels * frameLen * bitSize;
    u->bufInQ = pa_memblockq_new("module-effect-sink bufInQ", 0, MEMBLOCKQ_MAXLENGTH, targetSize, &u->sampleSpec,
        1, 1, 0, NULL);
    return 0;
}

int pa__init(pa_module *m)
{
    struct userdata *u;
    pa_modargs *ma;
    pa_sink *masterSink;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, VALID_MODARGS))) {
        AUDIO_ERR_LOG("Failed to parse module arguments.");
        return InitFail(m, ma);
    }

    if (!(masterSink = pa_namereg_get(m->core, pa_modargs_get_value(ma, "master", NULL), PA_NAMEREG_SINK))) {
        AUDIO_ERR_LOG("MasterSink not found");
        return InitFail(m, ma);
    }

    u = pa_xnew0(struct userdata, 1);
    u->module = m;
    m->userdata = u;

    if (CreateSink(m, ma, masterSink, u) != 0) {
        return InitFail(m, ma);
    }

    if (CreateSinkInput(m, ma, masterSink, u) != 0 || ConfigSinkInput(u) != 0) {
        return InitFail(m, ma);
    }

    pa_sink_input_put(u->sinkInput);
    pa_sink_put(u->sink);
    pa_sink_input_cork(u->sinkInput, false);

    pa_modargs_free(ma);
    return 0;
}

int pa__get_n_used(pa_module *m)
{
    struct userdata *u;

    pa_assert(m);
    pa_assert_se(u = m->userdata);

    return pa_sink_linked_by(u->sink);
}

void pa__done(pa_module *m)
{
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata)) {
        return;
    }

    if (u->sinkInput) {
        pa_sink_input_cork(u->sinkInput, true);
    }

    if (u->sink) {
        pa_sink_unlink(u->sink);
    }

    if (u->sinkInput) {
        pa_sink_input_unlink(u->sinkInput);
        pa_sink_input_unref(u->sinkInput);
    }

    if (u->sink) {
        pa_sink_unref(u->sink);
    }

    pa_xfree(u);
}
