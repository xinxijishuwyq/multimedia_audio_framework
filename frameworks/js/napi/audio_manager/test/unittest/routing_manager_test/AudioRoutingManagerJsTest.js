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

const TAG = "[AudioRoutingManagerJsTest]";
const stringParameter = 'stringParameter';
const numberParameter = 12345678;

describe("AudioRoutingManagerJsTest", function () {

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
   * @tc.name:getPreferOutputDeviceForRendererInfoTest001
   * @tc.desc:Get prefer output device - promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferOutputDeviceForRendererInfo(rendererInfo);
      console.info(`${TAG} getPreferOutputDeviceForRendererInfo SUCCESS`+JSON.stringify(data));
      expect(true).assertTrue();
      done();
    } catch(e) {
      console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
      expect().assertFail();
      done();
    }
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest002
   * @tc.desc:Get prefer output device no parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest002", 0, async function (done) {
      let routingManager = audio.getAudioManager().getRoutingManager();
      try {
        let data = await routingManager.getPreferOutputDeviceForRendererInfo();
        console.error(`${TAG} getPreferOutputDeviceForRendererInfo parameter check ERROR: ${JSON.stringify(data)}`);
        expect().assertFail();
      } catch(e) {
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferOutputDeviceForRendererInfo check no parameter PASS`);
        expect(true).assertTrue();
      }
      done();
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest004
   * @tc.desc:Get prefer output device check number parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest004", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferOutputDeviceForRendererInfo(numberParameter);
      console.error(`${TAG} getPreferOutputDeviceForRendererInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferOutputDeviceForRendererInfo check number parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest004
   * @tc.desc:Get prefer output device check string parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest004", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferOutputDeviceForRendererInfo(stringParameter);
      console.error(`${TAG} getPreferOutputDeviceForRendererInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferOutputDeviceForRendererInfo check string parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest005
   * @tc.desc:Get prefer output device - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest005", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferOutputDeviceForRendererInfo(rendererInfo, (e, data)=>{
        if (e) {
          console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect(false).assertTrue();
          done();
        }
        console.info(`${TAG} getPreferOutputDeviceForRendererInfo SUCCESS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest006
   * @tc.desc:Get prefer output device check number parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest006", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferOutputDeviceForRendererInfo(numberParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferOutputDeviceForRendererInfo check number parameter PASS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferOutputDeviceForRendererInfoTest007
   * @tc.desc:Get prefer output device check string parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferOutputDeviceForRendererInfoTest007", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferOutputDeviceForRendererInfo(stringParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferOutputDeviceForRendererInfo check string parameter PASS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:on_referOutputDeviceForRendererInfoTest001
   * @tc.desc:On prefer output device - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("on_preferOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferOutputDeviceChangeForRendererInfo', rendererInfo, (data)=>{});
      expect(true).assertTrue();
      done();
    } catch (e) {
        console.error(`${TAG} on_referOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
    }
  })

  /*
   * @tc.name:on_referOutputDeviceForRendererInfoTest002
   * @tc.desc:On prefer output device check string parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("on_preferOutputDeviceForRendererInfoTest002", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferOutputDeviceChangeForRendererInfo', stringParameter, (data)=>{});
      console.error(`${TAG} on_referOutputDeviceForRendererInfo with string patameter ERROR: ${e.message}`);
      expect().assertFail();
      done();
    } catch (e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} on_referOutputDeviceForRendererInfo check string parameter ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} on_referOutputDeviceForRendererInfo PASS: ${e.message}`);
      expect(true).assertTrue();
      done();
    }
  })

  /*
   * @tc.name:on_referOutputDeviceForRendererInfoTest003
   * @tc.desc:On prefer output device check number parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("on_preferOutputDeviceForRendererInfoTest003", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferOutputDeviceChangeForRendererInfo', numberParameter, (data)=>{});
      console.error(`${TAG} on_referOutputDeviceForRendererInfo with number patameter ERROR: ${e.message}`);
      expect().assertFail();
      done();
    } catch (e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} on_referOutputDeviceForRendererInfo check number parameter ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} on_referOutputDeviceForRendererInfo PASS: ${e.message}`);
      expect(true).assertTrue();
      done();
    }
  })

  /*
   * @tc.name:off_referOutputDeviceForRendererInfoTest001
   * @tc.desc:Off prefer output device - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("off_preferOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.off('preferOutputDeviceChangeForRendererInfo', (data)=>{});
      console.info(`${TAG} off_referOutputDeviceForRendererInfo SUCCESS`);
      expect(true).assertTrue();
      done();
    } catch (e) {
        console.error(`${TAG} off_referOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
    }
  })

})
