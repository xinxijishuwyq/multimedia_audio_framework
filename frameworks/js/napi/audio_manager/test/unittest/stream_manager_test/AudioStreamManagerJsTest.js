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

const TAG = "[AudioStreamManagerJsTest]";

describe("AudioStreamManagerJsTest", function () {
  let AudioStreamInfo = {
    samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_48000,
    channels: audio.AudioChannel.CHANNEL_2,
    sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S32LE,
    encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
  }

  let AudioRendererInfo = {
      content: audio.ContentType.CONTENT_TYPE_RINGTONE,
      usage: audio.StreamUsage.STREAM_USAGE_NOTIFICATION_RINGTONE,
      rendererFlags: 0
  }

  let AudioRendererOptions = {
      streamInfo: AudioStreamInfo,
      rendererInfo: AudioRendererInfo
  }

  beforeAll(async function () {
    console.info(TAG + "beforeAll called");
  })

  afterAll(function () {
    console.info(TAG + 'afterAll called')
  })

  beforeEach(function () {
    console.info(TAG + 'beforeEach called')
  })

  afterEach(function () {
    console.info(TAG + 'afterEach called')
  })

  function sleep(time) {
    return new Promise((resolve) => setTimeout(resolve, time));
  }

  /*
   * @tc.name:getCurrentAudioRendererInfoArray001
   * @tc.desc:Get getCurrentAudioRendererInfoArray
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray001", 0, async function (done) {
    let audioRenderer = null;
    let audioStreamManager = null;
    try {
      audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      audioStreamManager = audio.getAudioManager().getStreamManager();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray001 ERROR: ${e.message}`);
      expect().assertFail();
      done();
      return;
    }

    audioStreamManager.getCurrentAudioRendererInfoArray(async (err, audioRendererInfos) => {
      if (err) {
        console.error(`${TAG} getCurrentAudioRendererInfoArray001 ERROR: ${JSON.stringify(err)}`);
        expect(false).assertTrue();
        await audioRenderer.release();
        done();
        return;
      }

      console.info("getCurrentAudioRendererInfoArray001:"+JSON.stringify(audioRendererInfos));
      expect(audioRendererInfos.length).assertLarger(0);
      expect(1).assertEqual(audioRendererInfos[0].rendererState);
      expect(audioRendererInfos[0].deviceDescriptors[0].displayName!==""
        && audioRendererInfos[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

      await audioRenderer.release();
      done();
    })
  });

  /*
   * @tc.name:getCurrentAudioRendererInfoArray002
   * @tc.desc:Get getCurrentAudioRendererInfoArray--promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray002", 0, async function (done) {

    try {
      let audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      let audioStreamManager = audio.getAudioManager().getStreamManager();
      let audioRendererInfos= await audioStreamManager.getCurrentAudioRendererInfoArray();
      console.info(`${TAG} getCurrentAudioRendererInfoArray002 SUCCESS`+JSON.stringify(audioRendererInfos));
      expect(audioRendererInfos.length).assertLarger(0);
      expect(1).assertEqual(audioRendererInfos[0].rendererState);
      expect(audioRendererInfos[0].deviceDescriptors[0].displayName!==""
        && audioRendererInfos[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

      await audioRenderer.release();
      done();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray002 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
      return;
    }
  });

  /*
   * @tc.name:getCurrentAudioRendererInfoArray003
   * @tc.desc:Get getCurrentAudioRendererInfoArray
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray003", 0, async function (done) {

    let audioRenderer = null;
    let audioStreamManager = null;
    try {
      audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      audioStreamManager = audio.getAudioManager().getStreamManager();
      await audioRenderer.start();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray003 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
      return;
    }

    audioStreamManager.getCurrentAudioRendererInfoArray(async (err, audioRendererInfos) => {
      if (err) {
        console.error(`${TAG} getCurrentAudioRendererInfoArray003 ERROR: ${JSON.stringify(err)}`);
        expect(false).assertTrue();
        await audioRenderer.release();
        done();
        return;
      }
      console.info("AudioRendererChangeInfoArray++++:"+JSON.stringify(audioRendererInfos));
      expect(audioRendererInfos.length).assertLarger(0);
      expect(2).assertEqual(audioRendererInfos[0].rendererState);
      expect(audioRendererInfos[0].deviceDescriptors[0].displayName!==""
        && audioRendererInfos[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

      await audioRenderer.release();
      done();
    })
  });

  /*
   * @tc.name:getCurrentAudioRendererInfoArray004
   * @tc.desc:Get getCurrentAudioRendererInfoArray--promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray004", 0, async function (done) {
    let audioRenderer = null;
    let audioStreamManager = null;
    try {
      audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      audioStreamManager = audio.getAudioManager().getStreamManager();
      await audioRenderer.start();
      let audioRendererInfos = await audioStreamManager.getCurrentAudioRendererInfoArray();
      console.info("getCurrentAudioRendererInfoArray004:"+JSON.stringify(audioRendererInfos));
      expect(audioRendererInfos.length).assertLarger(0);
      expect(2).assertEqual(audioRendererInfos[0].rendererState);
      expect(audioRendererInfos[0].deviceDescriptors[0].displayName!==""
        && audioRendererInfos[0].deviceDescriptors[0].displayName!==undefined).assertTrue();
      sleep(1)
      await audioRenderer.release();
      done();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray004 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
      return;
    }
  });

  /*
   * @tc.name:getCurrentAudioRendererInfoArray005
   * @tc.desc:Get getCurrentAudioRendererInfoArray
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray005", 0, async function (done) {
    let audioRenderer = null;
    let audioStreamManager = null;
    try {
      audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      audioStreamManager = audio.getAudioManager().getStreamManager();
      await audioRenderer.start();
      await audioRenderer.stop();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray005 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
      return;
    }

    audioStreamManager.getCurrentAudioRendererInfoArray(async (err, AudioRendererInfoArray) => {
      if (err) {
        console.error(`${TAG} first getCurrentAudioRendererInfoArray ERROR: ${JSON.stringify(err)}`);
        expect(false).assertTrue();
        await audioRenderer.release();
        done();
        return;
      }
      console.info("getCurrentAudioRendererInfoArray005:"+JSON.stringify(AudioRendererInfoArray));
      expect(AudioRendererInfoArray.length).assertLarger(0);
      expect(3).assertEqual(AudioRendererInfoArray[0].rendererState);
      expect(AudioRendererInfoArray[0].deviceDescriptors[0].displayName !== ""
        && AudioRendererInfoArray[0].deviceDescriptors[0].displayName !== undefined).assertTrue();

      await audioRenderer.release();

      audioStreamManager.getCurrentAudioRendererInfoArray(async (err, AudioRendererInfos) => {
        if (err) {
          console.error(`${TAG} sencond getCurrentAudioRendererInfoArray ERROR: ${JSON.stringify(err)}`);
          expect(false).assertTrue();
          await audioRenderer.release();
          done();
          return;
        }
        expect(AudioRendererInfos.length).assertEqual(0);
      })
      done();
    })
  });

  /*
   * @tc.name:getCurrentAudioRendererInfoArray006
   * @tc.desc:Get getCurrentAudioRendererInfoArray--promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray006", 0, async function (done) {
    let audioRenderer = null;
    let audioStreamManager = null;
    try {
      audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      audioStreamManager = audio.getAudioManager().getStreamManager();
      await audioRenderer.start();
      await audioRenderer.stop();
      let audioRendererInfos = await audioStreamManager.getCurrentAudioRendererInfoArray();
      expect(audioRendererInfos.length).assertLarger(0);
      expect(3).assertEqual(audioRendererInfos[0].rendererState);
      expect(audioRendererInfos[0].deviceDescriptors[0].displayName!==""
        && audioRendererInfos[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

      await audioRenderer.release();
      audioRendererInfos = await audioStreamManager.getCurrentAudioRendererInfoArray();
      expect(audioRendererInfos.length).assertEqual(0);
      done();
    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray006 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
      return;
    }
  });

  /*
   * @tc.name:AudioRendererChange007
   * @tc.desc:Get AudioRendererChange info
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray007", 0, async function (done) {
    let count = 0;
    try {
      let audioStreamManager = audio.getAudioManager().getStreamManager();
      audioStreamManager.on('audioRendererChange', (AudioRendererChangeInfoArray) =>{

        expect(AudioRendererChangeInfoArray.length).assertLarger(0);
        count ++;
        expect(AudioRendererChangeInfoArray[0].deviceDescriptors[0].displayName!==""
        || AudioRendererChangeInfoArray[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

        if (count == 2) {
          done();
        }
      })

      let audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      await audioRenderer.release();

    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray007 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
    }
  });

  /*
   * @tc.name:AudioRendererChange008
   * @tc.desc:Get AudioRendererChange info
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getCurrentAudioRendererInfoArray008", 0, async function (done) {
    let count = 0;
    try {
      let audioStreamManager = audio.getAudioManager().getStreamManager();
      audioStreamManager.on('audioRendererChange', (AudioRendererChangeInfoArray) =>{

        expect(AudioRendererChangeInfoArray.length).assertLarger(0);
        count ++;
        expect(AudioRendererChangeInfoArray[0].deviceDescriptors[0].displayName!==""
        && AudioRendererChangeInfoArray[0].deviceDescriptors[0].displayName!==undefined).assertTrue();

        if (count == 3) {
          done();
        }
      })

      let audioRenderer = await audio.createAudioRenderer(AudioRendererOptions);
      await audioRenderer.start();
      await audioRenderer.release();

    } catch(e) {
      console.error(`${TAG} getCurrentAudioRendererInfoArray007 ERROR: ${e.message}`);
      expect().assertFail();
      await audioRenderer.release();
      done();
    }
  });
})
