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

#ifndef ST_NATIVE_AUDIOCAPTURER_H
#define ST_NATIVE_AUDIOCAPTURER_H

#include <time.h>
#include "native_audiostream_base.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Request to release the capturer stream.
 *
 * @since 10
 * @permission ohos.permission.MICROPHONE
 *
 * @param capturer reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_Release(OH_AudioCapturer* capturer);

/*
 * Request to start the capturer stream.
 *
 * @since 10
 * @permission ohos.permission.MICROPHONE
 *
 * @param capturer reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_Start(OH_AudioCapturer* capturer);

/*
 * Request to pause the capturer stream.
 *
 * @since 10
 * @permission ohos.permission.MICROPHONE
 *
 * @param capturer reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_Pause(OH_AudioCapturer* capturer);

/*
 * Request to stop the capturer stream.
 *
 * @since 10
 * @permission ohos.permission.MICROPHONE
 *
 * @param capturer reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_Stop(OH_AudioCapturer* capturer);

/*
 * Request to flush the capturer stream.
 *
 * @since 10
 *
 * @param capturer reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_Flush(OH_AudioCapturer* capturer);

/*
 * Query the current state of the capturer client.
 *
 * This function will return the capturer state without updating the state.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param state Pointer to a variable that will be set for the state value.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetCurrentState(OH_AudioCapturer* capturer, OH_AudioStream_State* state);

/*
 * Query the latency mode of the capturer client.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param latencyMode Pointer to a variable that will be set for the latency mode.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetLatencyMode(OH_AudioCapturer* capturer,
    OH_AudioStream_LatencyMode* latencyMode);

/*
 * Query the stream id of the capturer client.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param stramId Pointer to a variable that will be set for the stream id.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetStreamId(OH_AudioCapturer* capturer, uint32_t* streamId);

/*
 * Query the sample rate value of the capturer client.
 *
 * This function will return the capturer sample rate value without updating the state.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param rate The state value to be updated
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetSamplingRate(OH_AudioCapturer* capturer, int32_t* rate);

/*
 * Query the channel count of the capturer client.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param channelCount Pointer to a variable that will be set for the channel count.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetChannelCount(OH_AudioCapturer* capturer, int32_t* channelCount);

/*
 * Query the sample format of the capturer client.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param sampleFormat Pointer to a variable that will be set for the sample format.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetSampleFormat(OH_AudioCapturer* capturer,
    OH_AudioStream_SampleFormat* sampleFormat);

/*
 * Query the encoding type of the capturer client.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param encodingType Pointer to a variable that will be set for the encoding type.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetEncodingType(OH_AudioCapturer* capturer,
    OH_AudioStream_EncodingType* encodingType);

/*
 * Query the capturer info of the capturer client.
 *
 * The rendere info includes {@link OH_AudioStream_Usage} value and {@link OH_AudioStream_Content} value.
 *
 * @since 10
 *
 * @param capturer Reference created by OH_AudioStreamBuilder_GenerateCapturer()
 * @param usage Pointer to a variable that will be set for the stream usage.
 * @param content Pointer to a variable that will be set for the stream content.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioCapturer_GetCapturerInfo(OH_AudioCapturer* capturer,
    OH_AudioStream_SourceType* sourceType);
#ifdef __cplusplus
}
#endif
#endif // ST_NATIVE_AUDIORENDERER_H
