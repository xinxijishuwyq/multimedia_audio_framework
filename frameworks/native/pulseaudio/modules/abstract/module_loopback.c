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

// GCC does not warn for unused *static inline* functions, but clang does.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#undef LOG_TAG
#define LOG_TAG "ModuleLoopback"

#include <stdio.h>

#include <pulse/xmalloc.h>

#include <pulsecore/sink-input.h>
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/namereg.h>
#include <pulsecore/log.h>
#include <pulsecore/core-util.h>

#include <pulse/rtclock.h>
#include <pulse/timeval.h>

#include "audio_log.h"

PA_MODULE_AUTHOR("OpenHarmony");
PA_MODULE_DESCRIPTION(_("Loopback from source to sink"));
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "source=<source to connect to> "
        "sink=<sink to connect to> ");

#define DEFAULT_LATENCY_MSEC 200
#define MAX_LATENCY_MSEC 30000
#define MEMBLOCKQ_MAXLENGTH (1024 * 1024 * 32)
#define MIN_DEVICE_LATENCY (2.5 * PA_USEC_PER_MSEC)
#define DEFAULT_ADJUST_TIME_USEC (10 * PA_USEC_PER_SEC)
#define MAX_FAST_ADJUST_THRESHOLD 100
#define MIN_SAMPLE_RATE 4000
#define DEFAULT_SAMPLE_RATE 8000
#define MULTIPLE_FACTOR 2
#define TRIPLE_FACTOR 3
#define ADJUST_TIME_FOR_CHANGE (333 * PA_USEC_PER_MSEC)
#define TIME_HOUR_TO_SECOND_UNIT 3600
#define MIN_VISIBLE_UNDERRUN_COUNTER 2
#define SLEEP_ADJUST_TIME_MULTIPLE_TIMES 1.05
#define LATENCY_OFFSET_ADJUST_TIMES 0.01
#define LATENCY_INCREMENT_FIVE_MSEC (5 * PA_USEC_PER_MSEC)

typedef struct loopback_msg loopback_msg;

struct userdata {
    pa_core *core;
    pa_module *module;

    loopback_msg *msg;

    pa_sink_input *sink_input;
    pa_source_output *source_output;

    pa_asyncmsgq *asyncmsgq;
    pa_memblockq *memblockq;

    pa_rtpoll_item *rtpoll_item_read, *rtpoll_item_write;

    pa_time_event *time_event;

    /* Variables used to calculate the average time between
     * subsequent calls of AdjustRates() */
    pa_usec_t adjust_time_stamp;
    pa_usec_t real_adjust_time;
    pa_usec_t real_adjust_time_sum;

    /* Values from command line configuration */
    pa_usec_t latency;
    pa_usec_t max_latency;
    pa_usec_t adjust_time;
    pa_usec_t fast_adjust_threshold;

    /* Latency boundaries and current values */
    pa_usec_t min_source_latency;
    pa_usec_t max_source_latency;
    pa_usec_t min_sink_latency;
    pa_usec_t max_sink_latency;
    pa_usec_t configured_sink_latency;
    pa_usec_t configured_source_latency;
    int64_t source_latency_offset;
    int64_t sink_latency_offset;
    pa_usec_t minimum_latency;

    /* lower latency limit found by underruns */
    pa_usec_t underrun_latency_limit;

    /* Various counters */
    uint32_t iteration_counter;
    uint32_t underrun_counter;
    uint32_t adjust_counter;

    bool fixed_alsa_source;
    bool source_sink_changed;

    pa_sample_spec ss;
    pa_channel_map map;

    /* Used for sink input and source output snapshots */
    struct {
        int64_t send_counter;
        int64_t source_latency;
        pa_usec_t source_timestamp;

        int64_t recv_counter;
        size_t loopback_memblockq_length;
        int64_t sink_latency;
        pa_usec_t sink_timestamp;
    } latency_snapshot;

    /* Input thread variable */
    int64_t send_counter;

    /* Output thread variables */
    struct {
        int64_t recv_counter;
        pa_usec_t effective_source_latency;

        /* Copied from main thread */
        pa_usec_t minimum_latency;

        /* Various booleans */
        bool in_pop;
        bool pop_called;
        bool pop_adjust;
        bool first_pop_done;
        bool push_called;
    } output_thread_info;
};

struct loopback_msg {
    pa_msgobject parent;
    struct userdata *userdata;
};

PA_DEFINE_PRIVATE_CLASS(loopback_msg, pa_msgobject);
#define LOOPBACK_MSG(o) (loopback_msg_cast(o))

static const char * const VALID_MODARGS[] = {
    "source",
    "sink",
    NULL,
};

enum {
    SINK_INPUT_MESSAGE_POST = PA_SINK_INPUT_MESSAGE_MAX,
    SINK_INPUT_MESSAGE_REWIND,
    SINK_INPUT_MESSAGE_LATENCY_SNAPSHOT,
    SINK_INPUT_MESSAGE_SOURCE_CHANGED,
    SINK_INPUT_MESSAGE_SET_EFFECTIVE_SOURCE_LATENCY,
    SINK_INPUT_MESSAGE_UPDATE_MIN_LATENCY,
    SINK_INPUT_MESSAGE_FAST_ADJUST,
};

enum {
    SOURCE_OUTPUT_MESSAGE_LATENCY_SNAPSHOT = PA_SOURCE_OUTPUT_MESSAGE_MAX,
};

enum {
    LOOPBACK_MESSAGE_SOURCE_LATENCY_RANGE_CHANGED,
    LOOPBACK_MESSAGE_SINK_LATENCY_RANGE_CHANGED,
    LOOPBACK_MESSAGE_UNDERRUN,
};

static void EnableAdjustTimer(struct userdata *u, bool enable);

/* Called from main context */
static void TearDown(struct userdata *u)
{
    pa_assert(u);
    pa_assert_ctl_context();

    u->adjust_time = 0;
    EnableAdjustTimer(u, false);

    /* Handling the asyncmsgq between the source output and the sink input
     * requires some care. When the source output is unlinked, nothing needs
     * to be done for the asyncmsgq, because the source output is the sending
     * end. But when the sink input is unlinked, we should ensure that the
     * asyncmsgq is emptied, because the messages in the queue hold references
     * to the sink input. Also, we need to ensure that new messages won't be
     * written to the queue after we have emptied it.
     *
     * Emptying the queue can be done in the state_change() callback of the
     * sink input, when the new state is "unlinked".
     *
     * Preventing new messages from being written to the queue can be achieved
     * by unlinking the source output before unlinking the sink input. There
     * are no other writers for that queue, so this is sufficient. */

    if (u->source_output) {
        pa_source_output_unlink(u->source_output);
        pa_source_output_unref(u->source_output);
        u->source_output = NULL;
    }

    if (u->sink_input) {
        pa_sink_input_unlink(u->sink_input);
        pa_sink_input_unref(u->sink_input);
        u->sink_input = NULL;
    }
}

/* rate controller, called from main context
 * - maximum deviation from base rate is less than 1%
 * - can create audible artifacts by changing the rate too quickly
 * - exhibits hunting with USB or Bluetooth sources
 */
static uint32_t RateController(uint32_t baseRate, pa_usec_t adjustTime, int32_t latencyDifferenceUsec)
{
    uint32_t newRate;
    double minCycles;

    /* Calculate best rate to correct the current latency offset, limit at
     * slightly below 1% difference from base_rate */
    minCycles = (double)abs(latencyDifferenceUsec) / adjustTime / LATENCY_OFFSET_ADJUST_TIMES + 1;
    newRate = baseRate * (1.0 + (double)latencyDifferenceUsec / minCycles / adjustTime);

    return newRate;
}

/* Called from main thread.
 * It has been a matter of discussion how to correctly calculate the minimum
 * latency that module-loopback can deliver with a given source and sink.
 * The calculation has been placed in a separate function so that the definition
 * can easily be changed. The resulting estimate is not very exact because it
 * depends on the reported latency ranges. In cases were the lower bounds of
 * source and sink latency are not reported correctly (USB) the result will
 * be wrong. */
static void UpdateMinimumLatency(struct userdata *u, pa_sink *sink, bool printMsg)
{
    if (u->underrun_latency_limit) {
        /* If we already detected a real latency limit because of underruns, use it */
        u->minimum_latency = u->underrun_latency_limit;
    } else {
        /* Calculate latency limit from latency ranges */

        u->minimum_latency = u->min_sink_latency;
        if (u->fixed_alsa_source) {
            /* If we are using an alsa source with fixed latency, we will get a wakeup when
             * one fragment is filled, and then we empty the source buffer, so the source
             * latency never grows much beyond one fragment (assuming that the CPU doesn't
             * cause a bottleneck). */
            u->minimum_latency += u->core->default_fragment_size_msec * PA_USEC_PER_MSEC;
        } else {
            /* In all other cases the source will deliver new data at latest after one source latency.
             * Make sure there is enough data available that the sink can keep on playing until new
             * data is pushed. */
            u->minimum_latency += u->min_source_latency;
        }

        /* Multiply by 1.1 as a safety margin for delays that are proportional to the buffer sizes */
        u->minimum_latency *= 1.1;

        /* Add 1.5 ms as a safety margin for delays not related to the buffer sizes */
        u->minimum_latency += 1.5 * PA_USEC_PER_MSEC;
    }

    /* Add the latency offsets */
    if (-(u->sink_latency_offset + u->source_latency_offset) <= (int64_t)u->minimum_latency) {
        u->minimum_latency += u->sink_latency_offset + u->source_latency_offset;
    } else {
        u->minimum_latency = 0;
    }

    /* If the sink is valid, send a message to update the minimum latency to
     * the output thread, else set the variable directly */
    if (sink) {
        pa_asyncmsgq_send(sink->asyncmsgq, PA_MSGOBJECT(u->sink_input), SINK_INPUT_MESSAGE_UPDATE_MIN_LATENCY,
            NULL, u->minimum_latency, NULL);
    } else {
        u->output_thread_info.minimum_latency = u->minimum_latency;
    }

    if (printMsg) {
        AUDIO_DEBUG_LOG("Minimum possible end to end latency: %0.2f ms", (double)u->minimum_latency / PA_USEC_PER_MSEC);
        if (u->latency < u->minimum_latency) {
            AUDIO_WARNING_LOG("Configured latency of %0.2f ms is smaller than minimum latency, using minimum instead",
                (double)u->latency / PA_USEC_PER_MSEC);
        }
    }
}

static int CalculateAdjustTime(struct userdata *u, uint32_t *baseRate, int32_t *latencyDifference)
{
    pa_usec_t now = pa_rtclock_now();
    pa_usec_t timePassed = now - u->adjust_time_stamp;
    if (!u->source_sink_changed && timePassed < u->adjust_time * SLEEP_ADJUST_TIME_MULTIPLE_TIMES) {
        u->adjust_counter++;
        u->real_adjust_time_sum += timePassed;
        u->real_adjust_time = u->real_adjust_time_sum / u->adjust_counter;
    }
    u->adjust_time_stamp = now;

    /* Rates and latencies */
    uint32_t oldRate = u->sink_input->sample_spec.rate;
    *baseRate = u->source_output->sample_spec.rate;

    size_t buffer = u->latency_snapshot.loopback_memblockq_length;
    if (u->latency_snapshot.recv_counter <= u->latency_snapshot.send_counter) {
        buffer += (size_t) (u->latency_snapshot.send_counter - u->latency_snapshot.recv_counter);
    } else {
        buffer = PA_CLIP_SUB(buffer, (size_t) (u->latency_snapshot.recv_counter - u->latency_snapshot.send_counter));
    }

    pa_usec_t currentBufferLatency = pa_bytes_to_usec(buffer, &u->sink_input->sample_spec);
    pa_usec_t snapshotDelay = u->latency_snapshot.source_timestamp - u->latency_snapshot.sink_timestamp;
    int64_t currentSourceSinkLatency = u->latency_snapshot.sink_latency +
        u->latency_snapshot.source_latency - snapshotDelay;

    /* Current latency */
    int64_t currentLatency = currentSourceSinkLatency + currentBufferLatency;

    /* Latency at base rate */
    int64_t latencyAtOptimumRate = currentSourceSinkLatency + currentBufferLatency * oldRate / (*baseRate);

    pa_usec_t finalLatency = PA_MAX(u->latency, u->minimum_latency);
    *latencyDifference = (int32_t)(latencyAtOptimumRate - finalLatency);

    AUDIO_DEBUG_LOG("Loopback overall latency is %0.2f ms + %0.2f ms + %0.2f ms = %0.2f ms",
        (double) u->latency_snapshot.sink_latency / PA_USEC_PER_MSEC,
        (double) currentBufferLatency / PA_USEC_PER_MSEC,
        (double) u->latency_snapshot.source_latency / PA_USEC_PER_MSEC,
        (double) currentLatency / PA_USEC_PER_MSEC);

    AUDIO_DEBUG_LOG("Loopback latency at base rate is %0.2f ms", (double)latencyAtOptimumRate / PA_USEC_PER_MSEC);

    /* Drop or insert samples if fast_adjust_threshold_msec was specified and the latency difference is too large. */
    if (u->fast_adjust_threshold > 0 && ((pa_usec_t)(abs(*latencyDifference))) > u->fast_adjust_threshold) {
        AUDIO_DEBUG_LOG ("Latency difference larger than %" PRIu64 " msec, skipping or inserting samples.",
            u->fast_adjust_threshold / PA_USEC_PER_MSEC);

        pa_asyncmsgq_send(u->sink_input->sink->asyncmsgq, PA_MSGOBJECT(u->sink_input),
            SINK_INPUT_MESSAGE_FAST_ADJUST, NULL, currentSourceSinkLatency, NULL);

        /* Skip real adjust time calculation on next iteration. */
        u->source_sink_changed = true;
        return -1;
    }
    return 0;
}

/* Called from main context */
static void AdjustRates(struct userdata *u)
{
    uint32_t baseRate;
    int32_t latencyDifference;

    pa_assert(u);
    pa_assert_ctl_context();

    /* Runtime and counters since last change of source or sink
     * or source/sink latency */
    uint32_t runHours = u->iteration_counter * u->real_adjust_time / PA_USEC_PER_SEC / TIME_HOUR_TO_SECOND_UNIT;
    u->iteration_counter += 1;

    /* If we are seeing underruns then the latency is too small */
    if (u->underrun_counter > MIN_VISIBLE_UNDERRUN_COUNTER) {
        pa_usec_t target_latency;

        target_latency = PA_MAX(u->latency, u->minimum_latency) + LATENCY_INCREMENT_FIVE_MSEC;
        if (u->max_latency == 0 || target_latency < u->max_latency) {
            u->underrun_latency_limit =
                PA_CLIP_SUB((int64_t)target_latency, u->sink_latency_offset + u->source_latency_offset);
            AUDIO_WARNING_LOG("Too many underruns, increasing latency to %0.2f ms",
                (double)target_latency / PA_USEC_PER_MSEC);
        } else {
            u->underrun_latency_limit = PA_CLIP_SUB((int64_t)u->max_latency,
                u->sink_latency_offset + u->source_latency_offset);
            AUDIO_WARNING_LOG("Too many underruns, configured maximum latency of %0.2f ms is reached",
                (double)u->max_latency / PA_USEC_PER_MSEC);
            AUDIO_WARNING_LOG("Consider increasing the max_latency_msec");
        }

        UpdateMinimumLatency(u, u->sink_input->sink, false);
        u->underrun_counter = 0;
    }

    /* Allow one underrun per hour */
    if (u->iteration_counter * u->real_adjust_time / PA_USEC_PER_SEC / TIME_HOUR_TO_SECOND_UNIT > runHours) {
        u->underrun_counter = PA_CLIP_SUB(u->underrun_counter, 1u);
        AUDIO_DEBUG_LOG("Underrun counter: %u", u->underrun_counter);
    }

    /* Calculate real adjust time if source or sink did not change and if the system has
     * not been suspended. If the time between two calls is more than 5% longer than the
     * configured adjust time, we assume that the system has been sleeping and skip the
     * calculation for this iteration. */
    if (CalculateAdjustTime(u, &baseRate, &latencyDifference) != 0) {
        return;
    }

    /* Calculate new rate */
    uint32_t newRate = RateController(baseRate, u->real_adjust_time, latencyDifference);

    u->source_sink_changed = false;

    /* Set rate */
    pa_sink_input_set_rate(u->sink_input, newRate);
    AUDIO_DEBUG_LOG("[%s] Updated sampling rate to %lu Hz.", u->sink_input->sink->name, (unsigned long) newRate);
}

/* Called from main context */
static void TimeCallback(pa_mainloop_api *a, pa_time_event *e, const struct timeval *t, void *userdata)
{
    struct userdata *u = userdata;

    pa_assert(u);
    pa_assert(a);
    pa_assert(u->time_event == e);

    /* Restart timer right away */
    pa_core_rttime_restart(u->core, u->time_event, pa_rtclock_now() + u->adjust_time);

    /* Get sink and source latency snapshot */
    pa_asyncmsgq_send(u->sink_input->sink->asyncmsgq,
        PA_MSGOBJECT(u->sink_input), SINK_INPUT_MESSAGE_LATENCY_SNAPSHOT, NULL, 0, NULL);
    pa_asyncmsgq_send(u->source_output->source->asyncmsgq,
        PA_MSGOBJECT(u->source_output), SOURCE_OUTPUT_MESSAGE_LATENCY_SNAPSHOT, NULL, 0, NULL);

    AdjustRates(u);
}

/* Called from main context
 * When source or sink changes,
 * give it a third of a second to settle down, then call AdjustRates for the first time */
static void EnableAdjustTimer(struct userdata *u, bool enable)
{
    if (enable) {
        if (!u->adjust_time) {
            return;
        }
        if (u->time_event) {
            u->core->mainloop->time_free(u->time_event);
        }

        u->time_event = pa_core_rttime_new(u->core, pa_rtclock_now() + ADJUST_TIME_FOR_CHANGE, TimeCallback, u);
    } else {
        if (!u->time_event) {
            return;
        }

        u->core->mainloop->time_free(u->time_event);
        u->time_event = NULL;
    }
}

/* Called from main context */
static void UpdateAdjustTimer(struct userdata *u)
{
    if (u->sink_input->state == PA_SINK_INPUT_CORKED || u->source_output->state == PA_SOURCE_OUTPUT_CORKED) {
        EnableAdjustTimer(u, false);
    } else {
        EnableAdjustTimer(u, true);
    }
}

static void UpdateFixedLatency(struct userdata *u, pa_source *source)
{
    if (u == NULL || source == NULL) {
        return;
    }
    u->min_source_latency = pa_source_get_fixed_latency(source);
    u->max_source_latency = u->min_source_latency;
    const char *s;
    if ((s = pa_proplist_gets(source->proplist, PA_PROP_DEVICE_API))) {
        if (pa_streq(s, "alsa")) {
            u->fixed_alsa_source = true;
        }
    }
}

/* Called from main thread
 * Calculates minimum and maximum possible latency for source and sink */
static void UpdateLatencyBoundaries(struct userdata *u, pa_source *source, pa_sink *sink)
{
    if (source) {
        /* Source latencies */
        u->fixed_alsa_source = false;
        if (source->flags & PA_SOURCE_DYNAMIC_LATENCY) {
            pa_source_get_latency_range(source, &u->min_source_latency, &u->max_source_latency);
        } else {
            UpdateFixedLatency(u, source);
        }
        /* Source offset */
        u->source_latency_offset = source->port_latency_offset;

        /* Latencies below 2.5 ms cause problems, limit source latency if possible */
        if (u->max_source_latency >= MIN_DEVICE_LATENCY) {
            u->min_source_latency = PA_MAX(u->min_source_latency, MIN_DEVICE_LATENCY);
        } else {
            u->min_source_latency = u->max_source_latency;
        }
    }

    if (sink) {
        /* Sink latencies */
        if (sink->flags & PA_SINK_DYNAMIC_LATENCY) {
            pa_sink_get_latency_range(sink, &u->min_sink_latency, &u->max_sink_latency);
        } else {
            u->min_sink_latency = pa_sink_get_fixed_latency(sink);
            u->max_sink_latency = u->min_sink_latency;
        }
        /* Sink offset */
        u->sink_latency_offset = sink->port_latency_offset;

        /* Latencies below 2.5 ms cause problems, limit sink latency if possible */
        if (u->max_sink_latency >= MIN_DEVICE_LATENCY) {
            u->min_sink_latency = PA_MAX(u->min_sink_latency, MIN_DEVICE_LATENCY);
        } else {
            u->min_sink_latency = u->max_sink_latency;
        }
    }

    UpdateMinimumLatency(u, sink, true);
}

/* Called from output context
 * Sets the memblockq to the configured latency corrected by latency_offset_usec */
static void MemblockqAdjust(struct userdata *u, int64_t latencyOffsetUsec, bool allowPush)
{
    size_t bufferCorrection;
    int64_t requestedBufferLatency;
    pa_usec_t finalLatency = PA_MAX(u->latency, u->output_thread_info.minimum_latency);

    /* If source or sink have some large negative latency offset, we might want to
     * hold more than final_latency in the memblockq */
    requestedBufferLatency = (int64_t)finalLatency - latencyOffsetUsec;

    /* Keep at least one sink latency in the queue to make sure that the sink
     * never underruns initially */
    pa_usec_t requestedSinkLatency = pa_sink_get_requested_latency_within_thread(u->sink_input->sink);
    if (requestedBufferLatency < (int64_t)requestedSinkLatency)
        requestedBufferLatency = requestedSinkLatency;

    size_t requestedMemblockqLength = pa_usec_to_bytes(requestedBufferLatency, &u->sink_input->sample_spec);
    size_t currentMemblockqLength = pa_memblockq_get_length(u->memblockq);
    if (currentMemblockqLength > requestedMemblockqLength) {
        /* Drop audio from queue */
        bufferCorrection = currentMemblockqLength - requestedMemblockqLength;
        AUDIO_DEBUG_LOG("Dropping %" PRIu64 " usec of audio from queue",
            pa_bytes_to_usec(bufferCorrection, &u->sink_input->sample_spec));
        pa_memblockq_drop(u->memblockq, bufferCorrection);
    } else if (currentMemblockqLength < requestedMemblockqLength && allowPush) {
        /* Add silence to queue */
        bufferCorrection = requestedMemblockqLength - currentMemblockqLength;
        AUDIO_DEBUG_LOG("Adding %" PRIu64 " usec of silence to queue",
            pa_bytes_to_usec(bufferCorrection, &u->sink_input->sample_spec));
        pa_memblockq_seek(u->memblockq, (int64_t)bufferCorrection, PA_SEEK_RELATIVE, true);
    }
}

/* Called from input thread context */
static void SourceOutputPushCb(pa_source_output *o, const pa_memchunk *chunk)
{
    struct userdata *u;
    pa_usec_t pushTime;
    int64_t currentSourceLatency;

    pa_source_output_assert_ref(o);
    pa_source_output_assert_io_context(o);
    pa_assert_se(u = o->userdata);

    /* Send current source latency and timestamp with the message */
    pushTime = pa_rtclock_now();
    currentSourceLatency = pa_source_get_latency_within_thread(u->source_output->source, true);

    pa_asyncmsgq_post(u->asyncmsgq, PA_MSGOBJECT(u->sink_input), SINK_INPUT_MESSAGE_POST,
        PA_INT_TO_PTR(currentSourceLatency), pushTime, chunk, NULL);
    u->send_counter += (int64_t) chunk->length;
}

/* Called from input thread context */
static void SourceOutputProcessRewindCb(pa_source_output *o, size_t nbytes)
{
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_source_output_assert_io_context(o);
    pa_assert_se(u = o->userdata);

    pa_asyncmsgq_post(u->asyncmsgq, PA_MSGOBJECT(u->sink_input), SINK_INPUT_MESSAGE_REWIND,
        NULL, (int64_t) nbytes, NULL, NULL);
    u->send_counter -= (int64_t) nbytes;
}

/* Called from input thread context */
static int SourceOutputProcessMsgCb(pa_msgobject *obj, int code, void *data, int64_t offset, pa_memchunk *chunk)
{
    struct userdata *u = PA_SOURCE_OUTPUT(obj)->userdata;

    if (code == SOURCE_OUTPUT_MESSAGE_LATENCY_SNAPSHOT) {
        size_t length;
        length = pa_memblockq_get_length(u->source_output->thread_info.delay_memblockq);

        u->latency_snapshot.send_counter = u->send_counter;
        /* Add content of delay memblockq to the source latency */
        u->latency_snapshot.source_latency = pa_source_get_latency_within_thread(u->source_output->source, true) +
            pa_bytes_to_usec(length, &u->source_output->source->sample_spec);
        u->latency_snapshot.source_timestamp = pa_rtclock_now();

        return 0;
    }
    return pa_source_output_process_msg(obj, code, data, offset, chunk);
}

/* Called from main thread.
 * Get current effective latency of the source. If the source is in use with
 * smaller latency than the configured latency, it will continue running with
 * the smaller value when the source output is switched to the source. */
static void UpdateEffectiveSourceLatency(struct userdata *u, pa_source *source, pa_sink *sink)
{
    pa_usec_t effectiveSourceLatency;

    effectiveSourceLatency = u->configured_source_latency;

    if (source) {
        effectiveSourceLatency = pa_source_get_requested_latency(source);
        if (effectiveSourceLatency == 0 || effectiveSourceLatency > u->configured_source_latency)
            effectiveSourceLatency = u->configured_source_latency;
    }

    /* If the sink is valid, send a message to the output thread, else set the variable directly */
    if (sink) {
        pa_asyncmsgq_send(sink->asyncmsgq, PA_MSGOBJECT(u->sink_input),
            SINK_INPUT_MESSAGE_SET_EFFECTIVE_SOURCE_LATENCY, NULL, (int64_t)effectiveSourceLatency, NULL);
    } else {
        u->output_thread_info.effective_source_latency = effectiveSourceLatency;
    }
}

/* Called from main thread.
 * Set source output latency to one third of the overall latency if possible.
 * The choice of one third is rather arbitrary somewhere between the minimum
 * possible latency which would cause a lot of CPU load and half the configured
 * latency which would quickly lead to underruns */
static void SetSourceOutputLatency(struct userdata *u, pa_source *source)
{
    pa_usec_t latency;

    pa_usec_t requestedLatency = u->latency / TRIPLE_FACTOR;

    /* Normally we try to configure sink and source latency equally. If the
     * sink latency cannot match the requested source latency try to set the
     * source latency to a smaller value to avoid underruns */
    if (u->min_sink_latency > requestedLatency) {
        latency = PA_MAX(u->latency, u->minimum_latency);
        requestedLatency = (latency - u->min_sink_latency) / MULTIPLE_FACTOR;
    }

    latency = PA_CLAMP(requestedLatency, u->min_source_latency, u->max_source_latency);
    u->configured_source_latency = pa_source_output_set_requested_latency(u->source_output, latency);
    if (u->configured_source_latency != requestedLatency)
        AUDIO_WARNING_LOG("Cannot set requested source latency of %0.2f ms, adjusting to %0.2f ms",
            (double)requestedLatency / PA_USEC_PER_MSEC, (double)u->configured_source_latency / PA_USEC_PER_MSEC);
}

/* Called from input thread context */
static void SourceOutputAttachCb(pa_source_output *o)
{
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_source_output_assert_io_context(o);
    pa_assert_se(u = o->userdata);

    u->rtpoll_item_write = pa_rtpoll_item_new_asyncmsgq_write(
        o->source->thread_info.rtpoll,
        PA_RTPOLL_LATE,
        u->asyncmsgq);
}

/* Called from input thread context */
static void SourceOutputDetachCb(pa_source_output *o)
{
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_source_output_assert_io_context(o);
    pa_assert_se(u = o->userdata);

    if (u->rtpoll_item_write) {
        pa_rtpoll_item_free(u->rtpoll_item_write);
        u->rtpoll_item_write = NULL;
    }
}

/* Called from main thread */
static void SourceOutputKillCb(pa_source_output *o)
{
    struct userdata *u;

    pa_source_output_assert_ref(o);
    pa_assert_ctl_context();
    pa_assert_se(u = o->userdata);

    TearDown(u);
    pa_module_unload_request(u->module, true);
}

/* Called from main thread */
static void SourceOutputSuspendCb(pa_source_output *o, pa_source_state_t oldState,
    pa_suspend_cause_t oldSuspendCause)
{
    struct userdata *u;
    bool suspended;

    pa_source_output_assert_ref(o);
    pa_assert_ctl_context();
    pa_assert_se(u = o->userdata);

    /* State has not changed, nothing to do */
    if (oldState == o->source->state) {
        return;
    }

    suspended = (o->source->state == PA_SOURCE_SUSPENDED);

    /* If the source has been suspended, we need to handle this like
     * a source change when the source is resumed */
    if (suspended) {
        if (u->sink_input->sink) {
            pa_asyncmsgq_send(u->sink_input->sink->asyncmsgq, PA_MSGOBJECT(u->sink_input),
                SINK_INPUT_MESSAGE_SOURCE_CHANGED, NULL, 0, NULL);
        } else {
            u->output_thread_info.push_called = false;
        }
    } else {
        /* Get effective source latency on unsuspend */
        UpdateEffectiveSourceLatency(u, u->source_output->source, u->sink_input->sink);
    }

    pa_sink_input_cork(u->sink_input, suspended);

    UpdateAdjustTimer(u);
}

/* Called from input thread context */
static void UpdateSourceLatencyRangeCb(pa_source_output *i)
{
    struct userdata *u;

    pa_source_output_assert_ref(i);
    pa_source_output_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    /* Source latency may have changed */
    pa_asyncmsgq_post(pa_thread_mq_get()->outq, PA_MSGOBJECT(u->msg),
        LOOPBACK_MESSAGE_SOURCE_LATENCY_RANGE_CHANGED, NULL, 0, NULL, NULL);
}

/* Called from output thread context */
static int SinkInputPopCb(pa_sink_input *i, size_t nbytes, pa_memchunk *chunk)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);
    pa_assert(chunk);

    /* It seems necessary to handle outstanding push messages here, though it is not clear
     * why. Removing this part leads to underruns when low latencies are configured. */
    u->output_thread_info.in_pop = true;
    while (pa_asyncmsgq_process_one(u->asyncmsgq) > 0) {
        ;
    }
    u->output_thread_info.in_pop = false;

    /* While pop has not been called, latency adjustments in SINK_INPUT_MESSAGE_POST are
     * enabled. Disable them on second pop and enable the final adjustment during the
     * next push. The adjustment must be done on the next push, because there is no way
     * to retrieve the source latency here. We are waiting for the second pop, because
     * the first pop may be called before the sink is actually started. */
    if (!u->output_thread_info.pop_called && u->output_thread_info.first_pop_done) {
        u->output_thread_info.pop_adjust = true;
        u->output_thread_info.pop_called = true;
    }
    u->output_thread_info.first_pop_done = true;

    if (pa_memblockq_peek(u->memblockq, chunk) < 0) {
        return -1;
    }

    chunk->length = PA_MIN(chunk->length, nbytes);
    pa_memblockq_drop(u->memblockq, chunk->length);

    /* Adjust the memblockq to ensure that there is
     * enough data in the queue to avoid underruns. */
    if (!u->output_thread_info.push_called) {
        MemblockqAdjust(u, 0, true);
    }

    return 0;
}

/* Called from output thread context */
static void SinkInputProcessRewindCb(pa_sink_input *i, size_t nbytes)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    pa_memblockq_rewind(u->memblockq, nbytes);
}

static void ProcessSinkInputMessagePost(struct userdata *u, void *data, int64_t offset, pa_memchunk *chunk)
{
    pa_memblockq_push_align(u->memblockq, chunk);

    /* If push has not been called yet, latency adjustments in SinkInputPopCb()
     * are enabled. Disable them on first push and correct the memblockq. If pop
     * has not been called yet, wait until the pop_cb() requests the adjustment */
    if (u->output_thread_info.pop_called && (!u->output_thread_info.push_called ||
        u->output_thread_info.pop_adjust)) {
        int64_t time_delta;

        /* This is the source latency at the time push was called */
        time_delta = PA_PTR_TO_INT(data);
        /* Add the time between push and post */
        time_delta += pa_rtclock_now() - (pa_usec_t) offset;
        /* Add the sink latency */
        time_delta += pa_sink_get_latency_within_thread(u->sink_input->sink, true);

        /* The source latency report includes the audio in the chunk,
         * but since we already pushed the chunk to the memblockq, we need
         * to subtract the chunk size from the source latency so that it
         * won't be counted towards both the memblockq latency and the
         * source latency.
         *
         * Sometimes the alsa source reports way too low latency (might
         * be a bug in the alsa source code). This seems to happen when
         * there's an overrun. As an attempt to detect overruns, we
         * check if the chunk size is larger than the configured source
         * latency. If so, we assume that the source should have pushed
         * a chunk whose size equals the configured latency, so we
         * modify time_delta only by that amount, which makes
         * MemblockqAdjust() drop more data than it would otherwise.
         * This seems to work quite well, but it's possible that the
         * next push also contains too much data, and in that case the
         * resulting latency will be wrong. */
        if (pa_bytes_to_usec(chunk->length,
            &u->sink_input->sample_spec) > u->output_thread_info.effective_source_latency) {
            time_delta -= (int64_t)u->output_thread_info.effective_source_latency;
        } else {
            time_delta -= (int64_t)pa_bytes_to_usec(chunk->length, &u->sink_input->sample_spec);
        }

        /* We allow pushing silence here to fix up the latency. This
            * might lead to a gap in the stream */
        MemblockqAdjust(u, time_delta, true);

        u->output_thread_info.pop_adjust = false;
        u->output_thread_info.push_called = true;
    }

    /* If pop has not been called yet, make sure the latency does not grow too much.
        * Don't push any silence here, because we already have new data in the queue */
    if (!u->output_thread_info.pop_called) {
        MemblockqAdjust(u, 0, false);
    }

    /* Is this the end of an underrun? Then let's start things
     * right-away */
    if (u->sink_input->sink->thread_info.state != PA_SINK_SUSPENDED &&
        u->sink_input->thread_info.underrun_for > 0 &&
        pa_memblockq_is_readable(u->memblockq)) {
        pa_asyncmsgq_post(pa_thread_mq_get()->outq, PA_MSGOBJECT(u->msg), LOOPBACK_MESSAGE_UNDERRUN,
            NULL, 0, NULL, NULL);
        /* If called from within the pop callback skip the rewind */
        if (!u->output_thread_info.in_pop) {
            AUDIO_DEBUG_LOG("Requesting rewind due to end of underrun.");
            pa_sink_input_request_rewind(u->sink_input,
                (size_t) (u->sink_input->thread_info.underrun_for ==
                (size_t) -1 ? 0 : u->sink_input->thread_info.underrun_for),
                false, true, false);
        }
    }

    u->output_thread_info.recv_counter += (int64_t) chunk->length;
}

/* Called from output thread context */
static int SinkInputProcessMsgCb(pa_msgobject *obj, int code, void *data, int64_t offset, pa_memchunk *chunk)
{
    struct userdata *u = PA_SINK_INPUT(obj)->userdata;

    pa_sink_input_assert_io_context(u->sink_input);

    switch (code) {
        case PA_SINK_INPUT_MESSAGE_GET_LATENCY: {
            pa_usec_t *r = data;
            *r = pa_bytes_to_usec(pa_memblockq_get_length(u->memblockq), &u->sink_input->sample_spec);

            /* Fall through, the default handler will add in the extra
             * latency added by the resampler */
            break;
        }

        case SINK_INPUT_MESSAGE_POST:
            ProcessSinkInputMessagePost(u, data, offset, chunk);
            return 0;

        case SINK_INPUT_MESSAGE_REWIND:
            /* Do not try to rewind if no data was pushed yet */
            if (u->output_thread_info.push_called) {
                pa_memblockq_seek(u->memblockq, -offset, PA_SEEK_RELATIVE, true);
            }
            u->output_thread_info.recv_counter -= offset;
            return 0;

        case SINK_INPUT_MESSAGE_LATENCY_SNAPSHOT: {
            size_t length;
            length = pa_memblockq_get_length(u->sink_input->thread_info.render_memblockq);

            u->latency_snapshot.recv_counter = u->output_thread_info.recv_counter;
            u->latency_snapshot.loopback_memblockq_length = pa_memblockq_get_length(u->memblockq);
            /* Add content of render memblockq to sink latency */
            u->latency_snapshot.sink_latency = pa_sink_get_latency_within_thread(u->sink_input->sink, true) +
                pa_bytes_to_usec(length, &u->sink_input->sink->sample_spec);
            u->latency_snapshot.sink_timestamp = pa_rtclock_now();
            return 0;
        }

        case SINK_INPUT_MESSAGE_SOURCE_CHANGED:
            u->output_thread_info.push_called = false;
            return 0;

        case SINK_INPUT_MESSAGE_SET_EFFECTIVE_SOURCE_LATENCY:
            u->output_thread_info.effective_source_latency = (pa_usec_t)offset;
            return 0;

        case SINK_INPUT_MESSAGE_UPDATE_MIN_LATENCY:
            u->output_thread_info.minimum_latency = (pa_usec_t)offset;
            return 0;

        case SINK_INPUT_MESSAGE_FAST_ADJUST:
            MemblockqAdjust(u, offset, true);
            return 0;

        default:
            break;
    }

    return pa_sink_input_process_msg(obj, code, data, offset, chunk);
}
/* Called from main thread.
 * Set sink input latency to one third of the overall latency if possible.
 * The choice of one third is rather arbitrary somewhere between the minimum
 * possible latency which would cause a lot of CPU load and half the configured
 * latency which would quickly lead to underruns. */
static void SetSinkInputLatency(struct userdata *u, pa_sink *sink)
{
    pa_usec_t latency;

    pa_usec_t requestedLatency = u->latency / TRIPLE_FACTOR;

    /* Normally we try to configure sink and source latency equally. If the
     * source latency cannot match the requested sink latency try to set the
     * sink latency to a smaller value to avoid underruns */
    if (u->min_source_latency > requestedLatency) {
        latency = PA_MAX(u->latency, u->minimum_latency);
        requestedLatency = (latency - u->min_source_latency) / MULTIPLE_FACTOR;
    }

    latency = PA_CLAMP(requestedLatency, u->min_sink_latency, u->max_sink_latency);
    u->configured_sink_latency = pa_sink_input_set_requested_latency(u->sink_input, latency);
    if (u->configured_sink_latency != requestedLatency) {
        AUDIO_WARNING_LOG("Cannot set requested sink latency of %0.2f ms, adjusting to %0.2f ms",
            (double)requestedLatency / PA_USEC_PER_MSEC, (double)u->configured_sink_latency / PA_USEC_PER_MSEC);
    }
}

/* Called from output thread context */
static void SinkInputAttachCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    u->rtpoll_item_read = pa_rtpoll_item_new_asyncmsgq_read(
        i->sink->thread_info.rtpoll,
        PA_RTPOLL_LATE,
        u->asyncmsgq);

    pa_memblockq_set_prebuf(u->memblockq, pa_sink_input_get_max_request(i) * MULTIPLE_FACTOR);
    pa_memblockq_set_maxrewind(u->memblockq, pa_sink_input_get_max_rewind(i));
}

/* Called from output thread context */
static void SinkInputDetachCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    if (u->rtpoll_item_read) {
        pa_rtpoll_item_free(u->rtpoll_item_read);
        u->rtpoll_item_read = NULL;
    }
}

/* Called from output thread context */
static void SinkInputUpdateMaxRewindCb(pa_sink_input *i, size_t nbytes)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    pa_memblockq_set_maxrewind(u->memblockq, nbytes);
}

/* Called from output thread context */
static void SinkInputUpdateMaxRequestCb(pa_sink_input *i, size_t nbytes)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    pa_memblockq_set_prebuf(u->memblockq, nbytes * MULTIPLE_FACTOR);
    AUDIO_DEBUG_LOG("Max request changed");
}

/* Called from main thread */
static void SinkInputKillCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_ctl_context();
    pa_assert_se(u = i->userdata);

    TearDown(u);
    pa_module_unload_request(u->module, true);
}

/* Called from the output thread context */
static void SinkInputStateChangeCb(pa_sink_input *i, pa_sink_input_state_t state)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_assert_se(u = i->userdata);

    if (state == PA_SINK_INPUT_UNLINKED) {
        pa_asyncmsgq_flush(u->asyncmsgq, false);
    }
}

/* Called from main thread */
static void SinkInputSuspendCb(pa_sink_input *i, pa_sink_state_t oldState, pa_suspend_cause_t oldSuspendCause)
{
    struct userdata *u;
    bool suspended;

    pa_sink_input_assert_ref(i);
    pa_assert_ctl_context();
    pa_assert_se(u = i->userdata);

    /* State has not changed, nothing to do */
    if (oldState == i->sink->state) {
        return;
    }

    suspended = (i->sink->state == PA_SINK_SUSPENDED);

    /* If the sink has been suspended, we need to handle this like
     * a sink change when the sink is resumed. Because the sink
     * is suspended, we can set the variables directly. */
    if (suspended) {
        u->output_thread_info.pop_called = false;
        u->output_thread_info.first_pop_done = false;
    } else {
        /* Set effective source latency on unsuspend */
        UpdateEffectiveSourceLatency(u, u->source_output->source, u->sink_input->sink);
    }

    pa_source_output_cork(u->source_output, suspended);

    UpdateAdjustTimer(u);
}

/* Called from output thread context */
static void UpdateSinkLatencyRangeCb(pa_sink_input *i)
{
    struct userdata *u;

    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert_se(u = i->userdata);

    /* Sink latency may have changed */
    pa_asyncmsgq_post(pa_thread_mq_get()->outq, PA_MSGOBJECT(u->msg),
        LOOPBACK_MESSAGE_SINK_LATENCY_RANGE_CHANGED, NULL, 0, NULL, NULL);
}

/* Called from main context */
static int LoopbackProcessMsgCb(pa_msgobject *o, int code, void *userdata, int64_t offset, pa_memchunk *chunk)
{
    struct loopback_msg *msg;
    struct userdata *u;
    pa_usec_t currentLatency;

    pa_assert(o);
    pa_assert_ctl_context();

    msg = LOOPBACK_MSG(o);
    pa_assert_se(u = msg->userdata);

    switch (code) {
        case LOOPBACK_MESSAGE_SOURCE_LATENCY_RANGE_CHANGED:
            UpdateEffectiveSourceLatency(u, u->source_output->source, u->sink_input->sink);
            currentLatency = pa_source_get_requested_latency(u->source_output->source);
            if (currentLatency > u->configured_source_latency) {
                /* The minimum latency has changed to a value larger than the configured latency, so
                 * the source latency has been increased. The case that the minimum latency changes
                 * back to a smaller value is not handled because this never happens with the current
                 * source implementations. */
                AUDIO_WARNING_LOG("Source minimum latency increased to %0.2f ms",
                    (double)currentLatency / PA_USEC_PER_MSEC);
                u->configured_source_latency = currentLatency;
                UpdateLatencyBoundaries(u, u->source_output->source, u->sink_input->sink);
                /* We re-start counting when the latency has changed */
                u->iteration_counter = 0;
                u->underrun_counter = 0;
            }
            return 0;

        case LOOPBACK_MESSAGE_SINK_LATENCY_RANGE_CHANGED:
            currentLatency = pa_sink_get_requested_latency(u->sink_input->sink);
            if (currentLatency > u->configured_sink_latency) {
                /* The minimum latency has changed to a value larger than the configured latency, so
                 * the sink latency has been increased. The case that the minimum latency changes back
                 * to a smaller value is not handled because this never happens with the current sink
                 * implementations. */
                AUDIO_WARNING_LOG("Sink minimum latency increased to %0.2f ms",
                    (double)currentLatency / PA_USEC_PER_MSEC);
                u->configured_sink_latency = currentLatency;
                UpdateLatencyBoundaries(u, u->source_output->source, u->sink_input->sink);
                /* We re-start counting when the latency has changed */
                u->iteration_counter = 0;
                u->underrun_counter = 0;
            }
            return 0;

        case LOOPBACK_MESSAGE_UNDERRUN:
            u->underrun_counter++;
            AUDIO_DEBUG_LOG("Underrun detected, counter incremented to %u", u->underrun_counter);
            return 0;
        default:
            break;
    }

    return 0;
}

static pa_hook_result_t SinkPortLatencyOffsetChangedCb(pa_core *core, pa_sink *sink, struct userdata *u)
{
    if (sink != u->sink_input->sink) {
        return PA_HOOK_OK;
    }

    u->sink_latency_offset = sink->port_latency_offset;
    UpdateMinimumLatency(u, sink, true);

    return PA_HOOK_OK;
}

static pa_hook_result_t SourcePortLatencyOffsetChangedCb(pa_core *core, pa_source *source, struct userdata *u)
{
    if (source != u->source_output->source) {
        return PA_HOOK_OK;
    }

    u->source_latency_offset = source->port_latency_offset;
    UpdateMinimumLatency(u, u->sink_input->sink, true);

    return PA_HOOK_OK;
}

int InitFailed(pa_module *m, pa_modargs *ma)
{
    AUDIO_ERR_LOG("Init Loopback failed.");
    if (ma) {
        pa_modargs_free(ma);
    }

    pa__done(m);

    return -1;
}

static int InitSampleSpecAndChannelMap(pa_modargs *ma, struct userdata *u, pa_sink *sink, pa_source *source)
{
    if (source) {
        u->ss = source->sample_spec;
        u->map = source->channel_map;
    } else if (sink) {
        u->ss = sink->sample_spec;
        u->map = sink->channel_map;
    } else {
        /* Dummy stream format, needed because pa_sink_input_new()
         * requires valid sample spec and channel map even when all the FIX_*
         * stream flags are specified. pa_sink_input_new() should be changed
         * to ignore the sample spec and channel map when the FIX_* flags are
         * present. */
        u->ss.format = PA_SAMPLE_U8;
        u->ss.rate = DEFAULT_SAMPLE_RATE;
        u->ss.channels = 1;
        u->map.channels = 1;
        u->map.map[0] = PA_CHANNEL_POSITION_MONO;
    }

    if (pa_modargs_get_sample_spec_and_channel_map(ma, &(u->ss), &(u->map), PA_CHANNEL_MAP_DEFAULT) < 0) {
        AUDIO_ERR_LOG("Invalid sample format specification or channel map");
        return -1;
    }

    if (u->ss.rate < MIN_SAMPLE_RATE || u->ss.rate > PA_RATE_MAX) {
        AUDIO_ERR_LOG("Invalid rate specification, valid range is 4000 Hz to %i Hz", PA_RATE_MAX);
        return -1;
    }

    return 0;
}

static void InitUserData(struct userdata *u)
{
    uint32_t latencyMsec;
    uint32_t maxLatencyMsec;
    uint32_t fastAdjustThreshold;
    uint32_t adjustTimeSec;
    latencyMsec = DEFAULT_LATENCY_MSEC;
    fastAdjustThreshold = 0;
    maxLatencyMsec = 0;

    u->latency = (pa_usec_t) latencyMsec * PA_USEC_PER_MSEC;
    u->max_latency = (pa_usec_t) maxLatencyMsec * PA_USEC_PER_MSEC;
    u->output_thread_info.pop_called = false;
    u->output_thread_info.pop_adjust = false;
    u->output_thread_info.push_called = false;
    u->iteration_counter = 0;
    u->underrun_counter = 0;
    u->underrun_latency_limit = 0;
    u->source_sink_changed = true;
    u->real_adjust_time_sum = 0;
    u->adjust_counter = 0;
    u->fast_adjust_threshold = fastAdjustThreshold * PA_USEC_PER_MSEC;

    adjustTimeSec = DEFAULT_ADJUST_TIME_USEC / PA_USEC_PER_SEC;
    if (adjustTimeSec != DEFAULT_ADJUST_TIME_USEC / PA_USEC_PER_SEC) {
        u->adjust_time = adjustTimeSec * PA_USEC_PER_SEC;
    } else {
        u->adjust_time = DEFAULT_ADJUST_TIME_USEC;
    }

    u->real_adjust_time = u->adjust_time;
}

static int CreateSinkInput(pa_module *m, pa_modargs *ma, struct userdata *u, pa_sink *sink)
{
    pa_sink_input_new_data sinkInputData;

    pa_sink_input_new_data_init(&sinkInputData);
    sinkInputData.driver = __FILE__;
    sinkInputData.module = m;

    if (sink) {
        pa_sink_input_new_data_set_sink(&sinkInputData, sink, false, true);
    }

    if (pa_modargs_get_proplist(ma, "sink_input_properties", sinkInputData.proplist, PA_UPDATE_REPLACE) < 0) {
        AUDIO_ERR_LOG("Failed to parse the sink_input_properties value.");
        pa_sink_input_new_data_done(&sinkInputData);
        return -1;
    }

    if (!pa_proplist_contains(sinkInputData.proplist, PA_PROP_MEDIA_ROLE)) {
        pa_proplist_sets(sinkInputData.proplist, PA_PROP_MEDIA_ROLE, "abstract");
    }

    pa_sink_input_new_data_set_sample_spec(&sinkInputData, &(u->ss));
    pa_sink_input_new_data_set_channel_map(&sinkInputData, &(u->map));
    sinkInputData.flags = PA_SINK_INPUT_VARIABLE_RATE | PA_SINK_INPUT_START_CORKED | PA_SINK_INPUT_DONT_MOVE;

    pa_sink_input_new(&u->sink_input, m->core, &sinkInputData);
    pa_sink_input_new_data_done(&sinkInputData);

    CHECK_AND_RETURN_RET_LOG(u->sink_input, -1, "Create sink input failed");
    return 0;
}

static void AssignmentParameterUserdata(struct userdata *userdata)
{
    userdata->sink_input->parent.process_msg = SinkInputProcessMsgCb;
    userdata->sink_input->pop = SinkInputPopCb;
    userdata->sink_input->process_rewind = SinkInputProcessRewindCb;
    userdata->sink_input->kill = SinkInputKillCb;
    userdata->sink_input->state_change = SinkInputStateChangeCb;
    userdata->sink_input->attach = SinkInputAttachCb;
    userdata->sink_input->detach = SinkInputDetachCb;
    userdata->sink_input->update_max_rewind = SinkInputUpdateMaxRewindCb;
    userdata->sink_input->update_max_request = SinkInputUpdateMaxRequestCb;
    userdata->sink_input->suspend = SinkInputSuspendCb;
    userdata->sink_input->update_sink_latency_range = UpdateSinkLatencyRangeCb;
    userdata->sink_input->update_sink_fixed_latency = UpdateSinkLatencyRangeCb;
    userdata->sink_input->userdata = userdata;
}

static int ConfigSinkInput(struct userdata *u, const pa_source *source)
{
    const char *n;
    pa_memchunk silence;

    AssignmentParameterUserdata(u);

    UpdateLatencyBoundaries(u, u->source_output->source, u->sink_input->sink);
    SetSinkInputLatency(u, u->sink_input->sink);
    SetSourceOutputLatency(u, u->source_output->source);

    pa_sink_input_get_silence(u->sink_input, &silence);
    u->memblockq = pa_memblockq_new(
        "module-loopback memblockq",
        0,                      /* idx */
        MEMBLOCKQ_MAXLENGTH,    /* maxlength */
        MEMBLOCKQ_MAXLENGTH,    /* tlength */
        &(u->ss),                    /* sample_spec */
        0,                      /* prebuf */
        0,                      /* minreq */
        0,                      /* maxrewind */
        &silence);              /* silence frame */
    pa_memblock_unref(silence.memblock);
    /* Fill the memblockq with silence */
    pa_memblockq_seek(u->memblockq, pa_usec_to_bytes(u->latency, &u->sink_input->sample_spec), PA_SEEK_RELATIVE, true);

    u->asyncmsgq = pa_asyncmsgq_new(0);
    CHECK_AND_RETURN_RET_LOG(u->asyncmsgq, -1, "pa_asyncmsgq_new() failed.");

    if (!pa_proplist_contains(u->source_output->proplist, PA_PROP_MEDIA_NAME)) {
        pa_proplist_setf(u->source_output->proplist, PA_PROP_MEDIA_NAME, "Loopback to %s",
            pa_strnull(pa_proplist_gets(u->sink_input->sink->proplist, PA_PROP_DEVICE_DESCRIPTION)));
    }

    if (!pa_proplist_contains(u->source_output->proplist, PA_PROP_MEDIA_ICON_NAME) &&
        (n = pa_proplist_gets(u->sink_input->sink->proplist, PA_PROP_DEVICE_ICON_NAME))) {
        pa_proplist_sets(u->source_output->proplist, PA_PROP_MEDIA_ICON_NAME, n);
    }

    if (!pa_proplist_contains(u->sink_input->proplist, PA_PROP_MEDIA_NAME)) {
        pa_proplist_setf(u->sink_input->proplist, PA_PROP_MEDIA_NAME, "Loopback from %s",
            pa_strnull(pa_proplist_gets(u->source_output->source->proplist, PA_PROP_DEVICE_DESCRIPTION)));
    }

    if (source && !pa_proplist_contains(u->sink_input->proplist, PA_PROP_MEDIA_ICON_NAME) &&
        (n = pa_proplist_gets(u->source_output->source->proplist, PA_PROP_DEVICE_ICON_NAME))) {
        pa_proplist_sets(u->sink_input->proplist, PA_PROP_MEDIA_ICON_NAME, n);
    }

    return 0;
}

static int CreateSourceOutput(pa_module *m, pa_modargs *ma, struct userdata *u, pa_source *source)
{
    pa_source_output_new_data sourceOutputData;
    pa_sample_spec ss = u->ss;
    pa_channel_map map = u->map;

    pa_source_output_new_data_init(&sourceOutputData);
    sourceOutputData.driver = __FILE__;
    sourceOutputData.module = m;
    if (source) {
        pa_source_output_new_data_set_source(&sourceOutputData, source, false, true);
    }

    if (pa_modargs_get_proplist(ma, "source_output_properties", sourceOutputData.proplist, PA_UPDATE_REPLACE) < 0) {
        AUDIO_ERR_LOG("Failed to parse the source_output_properties value.");
        pa_source_output_new_data_done(&sourceOutputData);
        return -1;
    }

    if (!pa_proplist_contains(sourceOutputData.proplist, PA_PROP_MEDIA_ROLE))
        pa_proplist_sets(sourceOutputData.proplist, PA_PROP_MEDIA_ROLE, "abstract");

    pa_source_output_new_data_set_sample_spec(&sourceOutputData, &ss);
    pa_source_output_new_data_set_channel_map(&sourceOutputData, &map);
    sourceOutputData.flags = PA_SOURCE_OUTPUT_START_CORKED | PA_SOURCE_OUTPUT_DONT_MOVE;

    AUDIO_DEBUG_LOG("sourceOutputData.driver:%{public}s, module:%{pulic}s, source_output addr:%{public}p",
        sourceOutputData.driver, sourceOutputData.module->name, &u->source_output);

    pa_source_output_new(&u->source_output, m->core, &sourceOutputData);
    AUDIO_DEBUG_LOG("pa_source_output_new DONE");
    pa_source_output_new_data_done(&sourceOutputData);

    CHECK_AND_RETURN_RET_LOG(u->source_output, -1, "Create source output failed");
    return 0;
}

static void ConfigSourceOutput(struct userdata *u)
{
    u->source_output->parent.process_msg = SourceOutputProcessMsgCb;
    u->source_output->push = SourceOutputPushCb;
    u->source_output->process_rewind = SourceOutputProcessRewindCb;
    u->source_output->kill = SourceOutputKillCb;
    u->source_output->attach = SourceOutputAttachCb;
    u->source_output->detach = SourceOutputDetachCb;
    u->source_output->suspend = SourceOutputSuspendCb;
    u->source_output->update_source_latency_range = UpdateSourceLatencyRangeCb;
    u->source_output->update_source_fixed_latency = UpdateSourceLatencyRangeCb;
    u->source_output->userdata = u;

    /* If format, rate or channels were originally unset, they are set now
     * after the pa_source_output_new() call. */
    u->ss = u->source_output->sample_spec;
    u->map = u->source_output->channel_map;
}

static void ConfigLoopback(pa_module *m, struct userdata *u)
{
    /* Hooks to track changes of latency offsets */
    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SINK_PORT_LATENCY_OFFSET_CHANGED],
        PA_HOOK_NORMAL, (pa_hook_cb_t) SinkPortLatencyOffsetChangedCb, u);
    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SOURCE_PORT_LATENCY_OFFSET_CHANGED],
        PA_HOOK_NORMAL, (pa_hook_cb_t) SourcePortLatencyOffsetChangedCb, u);

    /* Setup message handler for main thread */
    u->msg = pa_msgobject_new(loopback_msg);
    u->msg->parent.process_msg = LoopbackProcessMsgCb;
    u->msg->userdata = u;

    /* The output thread is not yet running, set effective_source_latency directly */
    UpdateEffectiveSourceLatency(u, u->source_output->source, NULL);

    pa_sink_input_put(u->sink_input);
    pa_source_output_put(u->source_output);

    if (u->source_output->source->state != PA_SOURCE_SUSPENDED) {
        pa_sink_input_cork(u->sink_input, false);
    }

    if (u->sink_input->sink->state != PA_SINK_SUSPENDED) {
        pa_source_output_cork(u->source_output, false);
    }
}

int pa__init(pa_module *m)
{
    pa_modargs *ma = NULL;
    struct userdata *u;
    pa_sink *sink = NULL;
    pa_source *source = NULL;
    const char *n;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, VALID_MODARGS))) {
        AUDIO_ERR_LOG("Failed to parse module arguments");
        return InitFailed(m, ma);
    }

    n = pa_modargs_get_value(ma, "source", NULL);
    source = pa_namereg_get(m->core, n, PA_NAMEREG_SOURCE);
    CHECK_AND_RETURN_RET_LOG(!n || source, InitFailed(m, ma), "No such source.");

    n = pa_modargs_get_value(ma, "sink", NULL);
    sink = pa_namereg_get(m->core, n, PA_NAMEREG_SINK);
    CHECK_AND_RETURN_RET_LOG(!n || sink, InitFailed(m, ma), "No such sink.");

    m->userdata = u = pa_xnew0(struct userdata, 1);
    u->core = m->core;
    u->module = m;

    if (InitSampleSpecAndChannelMap(ma, u, sink, source) != 0) {
        return InitFailed(m, ma);
    }
    InitUserData(u);

    if (CreateSourceOutput(m, ma, u, source) != 0) {
        return InitFailed(m, ma);
    }
    ConfigSourceOutput(u);

    if (CreateSinkInput(m, ma, u, sink) != 0 || ConfigSinkInput(u, source) != 0) {
        return InitFailed(m, ma);
    }

    ConfigLoopback(m, u);

    pa_modargs_free(ma);
    return 0;
}

void pa__done(pa_module*m)
{
    struct userdata *u;
    pa_assert(m);

    if (!(u = m->userdata)) {
        return;
    }

    TearDown(u);

    if (u->memblockq) {
        pa_memblockq_free(u->memblockq);
    }
    if (u->asyncmsgq) {
        pa_asyncmsgq_unref(u->asyncmsgq);
    }

    pa_xfree(u);
}
