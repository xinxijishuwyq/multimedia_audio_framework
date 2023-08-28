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

import audio from '@ohos.multimedia.audio';
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'

const TAG = "[AudioCapturerJsUnitTest]";

describe("AudioCapturerJsUnitTest", function() {
    let audioStreamInfo = {
        samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_48000,
        channels: audio.AudioChannel.CHANNEL_1,
        sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
        encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    }
    let audioCapturerInfo = {
        source: audio.SourceType.SOURCE_TYPE_MIC,
        capturerFlags: 0
    }
    let audioCapturerOptions = {
        streamInfo: audioStreamInfo,
        rendererInfo: audioCapturerInfo
    }

    let audioCapturer;

    beforeAll(async function () {
        // input testsuit setup step, setup invoked before all testcases
        try {
            audioCapturer = audio.createAudioCapturerSync(audioCapturerOptions);
            console.info(`${TAG}: AudioCapturer created SUCCESS, state: ${audioCapturer.state}`);
        } catch (err) {
            console.error(`${TAG}: AudioCapturer created ERROR: ${err.message}`);
        }
        console.info(TAG + 'beforeAll called')
    })

    afterAll(function () {

        // input testsuit teardown step, teardown invoked after all testcases
        audioCapturer.release().then(() => {
            console.info(`${TAG}: AudioCapturer release : SUCCESS`);
        }).catch((err) => {
            console.info(`${TAG}: AudioCapturer release :ERROR : ${err.message}`);
        });
        console.info(TAG + 'afterAll called')
    })

    beforeEach(function () {

        // input testcase setup step, setup invoked before each testcases
        console.info(TAG + 'beforeEach called')
    })

    afterEach(function () {

        // input testcase teardown step, teardown invoked after each testcases
        console.info(TAG + 'afterEach called')
    })

    /*
     * @tc.name:SUB_AUDIO_CREATE_AUDIO_CAPUTURER_SYNC_001
     * @tc.desc:createAudioCapturerSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it("SUB_AUDIO_CREATE_AUDIO_CAPUTURER_SYNC_001", 0, async function (done) {
        try {
            let value = audio.createAudioCapturerSync(audioCapturerOptions);
            console.info(`SUB_AUDIO_CREATE_AUDIO_CAPUTURER_SYNC_001 SUCCESS: ${value}.`);
            expect(typeof value).assertEqual('object');
            done();
        } catch (err) {
            console.error(`SUB_AUDIO_CREATE_AUDIO_CAPUTURER_SYNC_001 ERROR: ${err}`);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SUB_AUDIO_CAPTURER_GET_STREAM_INFO_SYNC_TEST_001
     * @tc.desc:getStreamInfoSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it('SUB_AUDIO_CAPTURER_GET_STREAM_INFO_SYNC_TEST_001', 0, async function (done) {
        try {
            let data = audioCapturer.getStreamInfoSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_STREAM_INFO_SYNC_TEST_001 SUCCESS: ${data}`);
            expect(data.samplingRate).assertEqual(audio.AudioSamplingRate.SAMPLE_RATE_48000);
            expect(data.channels).assertEqual(audio.AudioChannel.CHANNEL_1);
            expect(data.sampleFormat).assertEqual(audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE);
            expect(data.encodingType).assertEqual(audio.AudioEncodingType.ENCODING_TYPE_RAW);
            done();
        } catch (err) {
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_STREAM_INFO_SYNC_TEST_001 ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SUB_AUDIO_CAPTURER_GET_CAPTURER_INFO_SYNC_TEST_001
     * @tc.desc:getCapturerInfoSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it('SUB_AUDIO_CAPTURER_GET_CAPTURER_INFO_SYNC_TEST_001', 0, async function (done) {
        try {
            let data = audioCapturer.getCapturerInfoSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_CAPTURER_INFO_SYNC_TEST_001 SUCCESS: ${data}`);
            expect(data.source).assertEqual(audio.SourceType.SOURCE_TYPE_MIC);
            expect(data.capturerFlags).assertEqual(0);
            done();
        } catch (err) {
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_CAPTURER_INFO_SYNC_TEST_001 ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SUB_AUDIO_CAPTURER_GET_AUDIO_STREAM_ID_SYNC_TEST_001
     * @tc.desc:getAudioStreamIdSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it('SUB_AUDIO_CAPTURER_GET_AUDIO_STREAM_ID_SYNC_TEST_001', 0, async function (done) {
        try {
            let data = audioCapturer.getAudioStreamIdSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_AUDIO_STREAM_ID_SYNC_TEST_001 SUCCESS: ${data}`);
            expect(typeof data).assertEqual('number');
            done();
        } catch (err) {
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_AUDIO_STREAM_ID_SYNC_TEST_001 ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SUB_AUDIO_CAPTURER_GET_BUFFER_SIZE_SYNC_TEST_001
     * @tc.desc:getBufferSizeSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it('SUB_AUDIO_CAPTURER_GET_BUFFER_SIZE_SYNC_TEST_001', 0, async function (done) {
        try {
            let data = audioCapturer.getBufferSizeSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_BUFFER_SIZE_SYNC_TEST_001 SUCCESS: ${data}`);
            expect(typeof data).assertEqual('number');
            done();
        } catch (err) {
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_BUFFER_SIZE_SYNC_TEST_001 ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
        }
    })

    /*
     * @tc.name:SUB_AUDIO_CAPTURER_GET_AUDIO_TIME_SYNC_TEST_001
     * @tc.desc:getAudioTimeSync success
     * @tc.type: FUNC
     * @tc.require: I7V04L
     */
    it('SUB_AUDIO_CAPTURER_GET_AUDIO_TIME_SYNC_TEST_001', 0, async function (done) {
        try {
            let audioCapturer = audio.createAudioCapturerSync(audioCapturerOptions);
            let data = audioCapturer.getAudioTimeSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_AUDIO_TIME_SYNC_TEST_001 SUCCESS, before start: ${data}`);
            expect(data).assertEqual(0);

            await audioCapturer.start();
            data = audioCapturer.getAudioTimeSync();
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_AUDIO_TIME_SYNC_TEST_001 SUCCESS, after start: ${data}`);
            expect(data).assertLarger(0);

            await audioCapturer.stop();
            await audioCapturer.release();
            done();
        } catch (err) {
            console.info(`${TAG}: SUB_AUDIO_CAPTURER_GET_AUDIO_TIME_SYNC_TEST_001 ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
        }
    })
})
