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

#include "linear_pos_time_model.h"

#include <cinttypes>

#include "audio_errors.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int64_t NANO_COUNT_PER_SECOND = 1000000000;
    static constexpr int32_t MAX_SUPPORT_SAMPLE_RETE = 384000;
}
LinearPosTimeModel::LinearPosTimeModel()
{
    AUDIO_INFO_LOG("New LinearPosTimeModel");
}

bool LinearPosTimeModel::ConfigSampleRate(int32_t sampleRate)
{
    AUDIO_INFO_LOG("ConfigSampleRate:%{public}d", sampleRate);
    if (isConfiged) {
        AUDIO_ERR_LOG("SampleRate already set:%{public}d", sampleRate_);
        return false;
    }
    sampleRate_ = sampleRate;
    if (sampleRate_ <= 0 || sampleRate_ > MAX_SUPPORT_SAMPLE_RETE) {
        AUDIO_ERR_LOG("Invalid sample rate!");
        return false;
    } else {
        nanoTimePerFrame_ = NANO_COUNT_PER_SECOND / sampleRate;
    }
    isConfiged = true;
    return true;
}

void LinearPosTimeModel::ResetFrameStamp(uint64_t frame, int64_t nanoTime)
{
    AUDIO_INFO_LOG("Reset frame:%{public}" PRIu64" with time:%{public}" PRId64".", frame, nanoTime);
    stampFrame_ = frame;
    stampNanoTime_ = nanoTime;
    return;
}

void LinearPosTimeModel::UpdataFrameStamp(uint64_t frame, int64_t nanoTime)
{
    AUDIO_INFO_LOG("Updata frame:%{public}" PRIu64" with time:%{public}" PRId64".", frame, nanoTime);
    // todo calclulate for ap-dsp-delta-time.
    stampFrame_ = frame;
    stampNanoTime_ = nanoTime;
    return;
}

void LinearPosTimeModel::SetSpanCount(uint64_t spanCountInFrame)
{
    AUDIO_INFO_LOG("New spanCountInFrame:%{public}" PRIu64".", spanCountInFrame);
    spanCountInFrame_ = spanCountInFrame;
    return;
}

int64_t LinearPosTimeModel::GetTimeOfPos(uint64_t posInFrame)
{
    int64_t deltaFrame = 0;
    int64_t invalidTime = -1;
    if (!isConfiged) {
        AUDIO_ERR_LOG("SampleRate is not configed!");
        return invalidTime;
    }
    if (posInFrame >= stampFrame_) {
        CHECK_AND_RETURN_RET_LOG((posInFrame - stampFrame_ < sampleRate_), invalidTime, "posInFrame %{public}" PRIu64""
            " is too large, stampFrame: %{public}" PRIu64"", posInFrame, stampFrame_);
        deltaFrame = posInFrame - stampFrame_;
        return stampNanoTime_ + deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;
    } else {
        CHECK_AND_RETURN_RET_LOG((stampFrame_ - posInFrame < sampleRate_), invalidTime, "posInFrame %{public}" PRIu64""
            " is too small, stampFrame: %{public}" PRIu64"", posInFrame, stampFrame_);
        deltaFrame = stampFrame_ - posInFrame;
        return stampNanoTime_ - deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;
    }
    return invalidTime;
}
} // namespace AudioStandard
} // namespace OHOS

