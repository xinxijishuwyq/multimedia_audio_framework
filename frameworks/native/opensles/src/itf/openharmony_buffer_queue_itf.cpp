/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <common.h>

using namespace OHOS::AudioStandard;

SLresult Enqueue(SLOHBufferQueueItf self, const void *buffer, SLuint32 size)
{
    IOHBufferQueue *thiz = (IOHBufferQueue *)self;
    AudioPlayerAdapter::GetInstance()->EnqueueAdapter(thiz->mId, buffer, size);
    return SL_RESULT_SUCCESS;
}

SLresult Clear(SLOHBufferQueueItf self)
{
    IOHBufferQueue *thiz = (IOHBufferQueue *)self;
    AudioPlayerAdapter::GetInstance()->ClearAdapter(thiz->mId);
    return SL_RESULT_SUCCESS;
}

SLresult GetState(SLOHBufferQueueItf self, SLOHBufferQueueState *state)
{
    IOHBufferQueue *thiz = (IOHBufferQueue *)self;
    AudioPlayerAdapter::GetInstance()->GetStateAdapter(thiz->mId, state);
    return SL_RESULT_SUCCESS;
}

SLresult GetBuffer(SLOHBufferQueueItf self, SLuint8 **buffer, SLuint32 &size)
{
    IOHBufferQueue *thiz = (IOHBufferQueue *)self;
    AudioPlayerAdapter::GetInstance()->GetBufferAdapter(thiz->mId, buffer, size);
    return SL_RESULT_SUCCESS;
}

SLresult RegisterCallback(SLOHBufferQueueItf self,
    SlOHBufferQueueCallback callback, void *pContext)
{
    AudioPlayerAdapter::GetInstance()->RegisterCallbackAdapter(self, callback, pContext);
    return SL_RESULT_SUCCESS;
}

static const struct SLOHBufferQueueItf_ IOHBufferQueueItf = {
    Enqueue,
    Clear,
    GetState,
    GetBuffer,
    RegisterCallback
};

void IOHBufferQueueInit(void *self, SLuint32 id)
{
    IOHBufferQueue *thiz = (IOHBufferQueue *) self;
    thiz->mItf = &IOHBufferQueueItf;
    thiz->mId = id;
    thiz->mState = SL_PLAYSTATE_STOPPED;
}
