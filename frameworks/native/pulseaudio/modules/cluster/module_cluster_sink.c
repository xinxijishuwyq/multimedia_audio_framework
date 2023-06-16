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

#include <pulsecore/sink.h>
#include <pulsecore/module.h>
#include <pulsecore/core-util.h>
#include <pulsecore/modargs.h>
#include <pulsecore/log.h>
#include <pulsecore/namereg.h>

#include "audio_effect_chain_adapter.h"
#include "audio_log.h"

PA_MODULE_AUTHOR("OpenHarmony");
PA_MODULE_DESCRIPTION(_("Cluster module"));
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "sink_name=<name of sink> "
);

struct userdata {
    pa_core *core;
    pa_module *module;
};

static const char * const VALID_MODARGS[] = {
    "sink_name",
    NULL
};

static pa_hook_result_t MoveSinkInputIntoSink(pa_sink_input *si, pa_sink *sink)
{
    if (si->sink != sink) {
        pa_sink_input_move_to(si, sink, false);
    }
    return PA_HOOK_OK;
}

static pa_hook_result_t SinkInputProplistChangedCb(pa_core *c, pa_sink_input *si, struct userdata *u)
{
    pa_sink *effectSink;
    pa_assert(c);
    pa_assert(u);
    const char *sceneMode = pa_proplist_gets(si->proplist, "scene.mode");
    const char *sceneType = pa_proplist_gets(si->proplist, "scene.type");

    bool existFlag = EffectChainManagerExist(sceneType, sceneMode);
    // if EFFECT_NONE mode or effect chain does not exist
    if (pa_safe_streq(sceneMode, "EFFECT_NONE") || !existFlag) {
        return MoveSinkInputIntoSink(si, c->default_sink); //if bypass move to hdi sink
    }

    effectSink = pa_namereg_get(c, sceneType, PA_NAMEREG_SINK);
    if (!effectSink) { // if sink does not exist
        AUDIO_ERR_LOG("Effect sink [%{public}s] sink not found.", sceneType);
        // classify sinkinput to default sink
        MoveSinkInputIntoSink(si, c->default_sink);
    } else {
        // classify sinkinput to effect sink
        MoveSinkInputIntoSink(si, effectSink);
    }

    return PA_HOOK_OK;
}

static pa_hook_result_t ClientProplistChangedCb(pa_core *c, pa_client *client, struct userdata *u)
{
    const char *name = pa_proplist_gets(client->proplist, "application.name");
    if (!pa_safe_streq(name, "PulseAudio Service")) {
        return PA_HOOK_OK;
    }
    uint32_t idx;
    pa_sink_input *si;
    PA_IDXSET_FOREACH(si, c->sink_inputs, idx) {
        SinkInputProplistChangedCb(c, si, u);
    }
    return PA_HOOK_OK;
}

int InitFail(pa_module *m, pa_modargs *ma)
{
    AUDIO_ERR_LOG("Failed to create cluster module");
    if (ma) {
        pa_modargs_free(ma);
    }
    pa__done(m);
    return -1;
}

int pa__init(pa_module *m)
{
    struct userdata *u = NULL;
    pa_modargs *ma = NULL;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, VALID_MODARGS))) {
        AUDIO_ERR_LOG("Failed to parse module arguments.");
        return InitFail(m, ma);
    }

    m->userdata = u = pa_xnew0(struct userdata, 1);
    u->core = m->core;
    u->module = m;

    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SINK_INPUT_PROPLIST_CHANGED], PA_HOOK_LATE,
        (pa_hook_cb_t)SinkInputProplistChangedCb, u);
    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_CLIENT_PROPLIST_CHANGED], PA_HOOK_LATE,
        (pa_hook_cb_t)ClientProplistChangedCb, u);

    pa_modargs_free(ma);

    return 0;
}

int pa__get_n_used(pa_module *m)
{
    struct userdata *u;

    pa_assert(m);
    pa_assert_se(u = m->userdata);

    return 0;
}

void pa__done(pa_module *m)
{
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata)) {
        return;
    }

    pa_xfree(u);
}
