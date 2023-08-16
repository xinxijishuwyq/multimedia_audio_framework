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
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest001
   * @tc.desc:Get prefer output device - promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredOutputDeviceForRendererInfo(rendererInfo);
      console.info(`${TAG} getPreferredOutputDeviceForRendererInfo SUCCESS`+JSON.stringify(data));
      expect(true).assertTrue();
      done();
    } catch(e) {
      console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
      expect().assertFail();
      done();
    }
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest002
   * @tc.desc:Get prefer output device no parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest002", 0, async function (done) {
      let routingManager = audio.getAudioManager().getRoutingManager();
      try {
        let data = await routingManager.getPreferredOutputDeviceForRendererInfo();
        console.error(`${TAG} getPreferredOutputDeviceForRendererInfo parameter check ERROR: ${JSON.stringify(data)}`);
        expect().assertFail();
      } catch(e) {
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredOutputDeviceForRendererInfo check no parameter PASS`);
        expect(true).assertTrue();
      }
      done();
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest004
   * @tc.desc:Get prefer output device check number parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest004", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredOutputDeviceForRendererInfo(numberParameter);
      console.error(`${TAG} getPreferredOutputDeviceForRendererInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferredOutputDeviceForRendererInfo check number parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest004
   * @tc.desc:Get prefer output device check string parameter- promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest004", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredOutputDeviceForRendererInfo(stringParameter);
      console.error(`${TAG} getPreferredOutputDeviceForRendererInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferredOutputDeviceForRendererInfo check string parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest005
   * @tc.desc:Get prefer output device - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest005", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredOutputDeviceForRendererInfo(rendererInfo, (e, data)=>{
        if (e) {
          console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect(false).assertTrue();
          done();
        }
        console.info(`${TAG} getPreferredOutputDeviceForRendererInfo SUCCESS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest006
   * @tc.desc:Get prefer output device check number parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest006", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredOutputDeviceForRendererInfo(numberParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredOutputDeviceForRendererInfo check number parameter PASS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferredOutputDeviceForRendererInfoTest007
   * @tc.desc:Get prefer output device check string parameter- callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getPreferredOutputDeviceForRendererInfoTest007", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredOutputDeviceForRendererInfo(stringParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredOutputDeviceForRendererInfo check string parameter PASS`);
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
  it("on_preferredOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let rendererInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags : 0 }
    
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredOutputDeviceChangeForRendererInfo', rendererInfo, (data)=>{});
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
  it("on_preferredOutputDeviceForRendererInfoTest002", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredOutputDeviceChangeForRendererInfo', stringParameter, (data)=>{});
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
  it("on_preferredOutputDeviceForRendererInfoTest003", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredOutputDeviceChangeForRendererInfo', numberParameter, (data)=>{});
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
  it("off_preferredOutputDeviceForRendererInfoTest001", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.off('preferredOutputDeviceChangeForRendererInfo', (data)=>{});
      console.info(`${TAG} off_referOutputDeviceForRendererInfo SUCCESS`);
      expect(true).assertTrue();
      done();
    } catch (e) {
        console.error(`${TAG} off_referOutputDeviceForRendererInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
    }
  })

  /*
   * @tc.name:getdevice001
   * @tc.desc:getdevice - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getdevice001", 0, async function (done) {
    let routingManager = null;
    try {
      routingManager = audio.getAudioManager().getRoutingManager();
      expect(true).assertTrue();
      done();
    } catch (e) {
      console.error(`${TAG} getdevice001 ERROR: ${e.message}`);
      expect().assertFail();
      done();
      return;
    }

    routingManager.getDevices(audio.DeviceFlag.INPUT_DEVICES_FLAG, (err, AudioDeviceDescriptors)=>{
      if (err) {
        console.error(`${TAG} first getDevices ERROR: ${JSON.stringify(err)}`);
        expect(false).assertTrue();
        done();
        return;
      }
      console.info(`${TAG} getDevices001 SUCCESS:`+ JSON.stringify(AudioDeviceDescriptors));
      expect(AudioDeviceDescriptors.length).assertLarger(0);
      for (let i = 0; i < AudioDeviceDescriptors.length; i++) {
        expect(AudioDeviceDescriptors[i].displayName!==""
        && AudioDeviceDescriptors[i].displayName!==undefined).assertTrue();
      }
      done();
    })
  });

  /*
   * @tc.name:getdevice002
   * @tc.desc:getdevice - promise
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("getdevice002", 0, async function (done) {
    try {
      let routingManager = audio.getAudioManager().getRoutingManager();
      let AudioDeviceDescriptors = await routingManager.getDevices(audio.DeviceFlag.INPUT_DEVICES_FLAG);
      console.info(`${TAG} getDevices002 SUCCESS:`+ JSON.stringify(AudioDeviceDescriptors));
      expect(AudioDeviceDescriptors.length).assertLarger(0);
      for (let i = 0; i < AudioDeviceDescriptors.length; i++) {
        expect(AudioDeviceDescriptors[i].displayName!==""
        && AudioDeviceDescriptors[i].displayName!==undefined).assertTrue();
      }
      done();
    } catch (e) {
      console.error(`${TAG} getdevice002 ERROR: ${e.message}`);
      expect().assertFail();
      done();
    }
  });

  /*
   * @tc.name:setCommunicationDevice001
   * @tc.desc:setCommunicationDevice - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("setCommunicationDevice001", 0, async function (done) {
    let audioManager = audio.getAudioManager();
    let rendererInfo = {
      content: audio.ContentType.CONTENT_TYPE_SPEECH,
      usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
      rendererFlags: 0
    };
    let flag = false;
    console.info(`${TAG} setCommunicationDevice001 start`);
    audioManager.setAudioScene(audio.AudioScene.AUDIO_SCENE_PHONE_CALL, (err) => {
      console.info(`${TAG} setAudioScene enter`);
      if (err) {
        console.error(`${TAG} setAudioScene ERROR: ${err.message}`);
        expect(false).assertTrue();
        done();
        return;
      }
      console.info(`${TAG} setAudioScene success`);
      let routingManager = audioManager.getRoutingManager();
      routingManager.getDevices(audio.DeviceFlag.OUTPUT_DEVICES_FLAG, (err, value) => {
        console.info(`${TAG} getDevices return: ` + JSON.stringify(value));
        if (err) {
          console.error(`${TAG} getDevices ERROR: ${err.message}`);
          expect(false).assertTrue();
          done();
          return;
        }
        console.info(`${TAG} getDevices value.length: ` + JSON.stringify(value.length));
        for (let i = 0; i < value.length; i++) {
          if (value[i].deviceType == audio.DeviceType.EARPIECE) {
            flag = true;
            break;
          }
        }
        console.info(`${TAG} getDevices flag: ` + flag);
        if (!flag) {
          console.error(`${TAG} This device does not have a eapiece`);
          expect(true).assertTrue();
          done();
          return;
        }
        routingManager.getPreferredOutputDeviceForRendererInfo(rendererInfo, (err, value) => {
          console.info(`${TAG} getPreferredOutputDeviceForRendererInfo return: ` + JSON.stringify(value));
          if (err) {
            console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
            return;
          }
          if (value[0].deviceType != audio.DeviceType.EARPIECE) {
            console.error(`${TAG} getPrefer device is not EARPIECE`);
            expect(false).assertTrue();
            done();
            return;
          }
          routingManager.setCommunicationDevice(audio.CommunicationDeviceType.SPEAKER, false, (err) => {
            console.info(`${TAG} setCommunicationDevice enter`);
            if (err) {
              console.error(`${TAG} setCommunicationDevice ERROR: ${err.message}`);
              expect(false).assertTrue();
              done();
              return;
            }
            routingManager.isCommunicationDeviceActive(audio.CommunicationDeviceType.SPEAKER, (err, value) => {
              console.info(`${TAG} isCommunicationDeviceActive return: `+ JSON.stringify(value));
              if (err) {
                console.error(`${TAG} isCommunicationDeviceActive ERROR: ${err.message}`);
                expect(false).assertTrue();
                done();
                return;
              }
              if (value) {
                console.error(`${TAG} isCommunicationDeviceActive reurn true`);
                expect(false).assertTrue();
                done();
                return;
              }
              expect(true).assertTrue();
              done();
              return;
            });
          });
        });
      });
    });
  });

  /*
   * @tc.name:setCommunicationDevice002
   * @tc.desc:setCommunicationDevice - callback
   * @tc.type: FUNC
   * @tc.require: I6C9VA
   */
  it("setCommunicationDevice002", 0, async function (done) {
    let audioManager = audio.getAudioManager();
    let rendererInfo = {
      content: audio.ContentType.CONTENT_TYPE_SPEECH,
      usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
      rendererFlags: 0
    };
    let flag = false;
    console.info(`${TAG} setCommunicationDevice002 start`);
    audioManager.setAudioScene(audio.AudioScene.AUDIO_SCENE_PHONE_CALL, (err) => {
      console.info(`${TAG} setAudioScene enter`);
      if (err) {
        console.error(`${TAG} setAudioScene ERROR: ${err.message}`);
        expect(false).assertTrue();
        done();
        return;
      }
      let routingManager = audioManager.getRoutingManager();
      routingManager.getDevices(audio.DeviceFlag.OUTPUT_DEVICES_FLAG, (err, value) => {
        console.info(`${TAG} getDevices return: ` + JSON.stringify(value));
        if (err) {
          console.error(`${TAG} getDevices ERROR: ${err.message}`);
          expect(false).assertTrue();
          done();
          return;
        }
        console.info(`${TAG} getDevices value.length: ` + JSON.stringify(value.length));
        for (let i = 0; i < value.length; i++) {
          if (value[i].deviceType == audio.DeviceType.EARPIECE) {
            flag = true;
            break;
          }
        }
        console.info(`${TAG} getDevices flag: ` + flag);
        if (!flag) {
          console.error(`${TAG} This device does not have a earpiece`);
          expect(true).assertTrue();
          done();
          return;
        }
        routingManager.getPreferredOutputDeviceForRendererInfo(rendererInfo, (err, value) => {
          console.info(`${TAG} getPreferredOutputDeviceForRendererInfo return: ` + JSON.stringify(value));
          if (err) {
            console.error(`${TAG} getPreferredOutputDeviceForRendererInfo ERROR: ${err.message}`);
            expect(false).assertTrue();
            done();
            return;
          }
          if (value[0].deviceType != audio.DeviceType.EARPIECE) {
            console.error(`${TAG} getPrefer device is not EARPIECE`);
            expect(false).assertTrue();
            done();
            return;
          }
          routingManager.setCommunicationDevice(audio.CommunicationDeviceType.SPEAKER, true, (err) => {
            console.info(`${TAG} setCommunicationDevice enter`);
            if (err) {
              console.error(`${TAG} setCommunicationDevice ERROR: ${err.message}`);
              expect(false).assertTrue();
              done();
              return;
            }
            routingManager.isCommunicationDeviceActive(audio.CommunicationDeviceType.SPEAKER, (err, value) => {
              console.info(`${TAG} isCommunicationDeviceActive return: `+ JSON.stringify(value));
              if (err) {
                console.error(`${TAG} isCommunicationDeviceActive ERROR: ${err.message}`);
                expect(false).assertTrue();
                done();
                return;
              }
              if (!value) {
                console.error(`${TAG} isCommunicationDeviceActive reurn false`);
                expect(false).assertTrue();
                done();
                return;
              }
              expect(true).assertTrue();
              done();
              return;
            });
          })
        });
      });
    });
  });

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest001
   * @tc.desc:Get preferred input device - promise
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest001", 0, async function (done) {
    let capturerInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      capturerFlags : 0 }

    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredInputDeviceForCapturerInfo(capturerInfo);
      console.info(`${TAG} getPreferredInputDeviceForCapturerInfo SUCCESS`+JSON.stringify(data));
      expect(true).assertTrue();
      done();
    } catch(e) {
      console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
      expect().assertFail();
      done();
    }
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest002
   * @tc.desc:Get preferred input device no parameter- promise
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest002", 0, async function (done) {
      let routingManager = audio.getAudioManager().getRoutingManager();
      try {
        let data = await routingManager.getPreferredInputDeviceForCapturerInfo();
        console.error(`${TAG} getPreferredInputDeviceForCapturerInfo parameter check ERROR: ${JSON.stringify(data)}`);
        expect().assertFail();
      } catch(e) {
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredInputDeviceForCapturerInfo check no parameter PASS`);
        expect(true).assertTrue();
      }
      done();
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest003
   * @tc.desc:Get preferred input device check number parameter- promise
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest003", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredInputDeviceForCapturerInfo(numberParameter);
      console.error(`${TAG} getPreferredInputDeviceForCapturerInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferredInputDeviceForCapturerInfo check number parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest004
   * @tc.desc:Get preferred input device check string parameter- promise
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest004", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      let data = await routingManager.getPreferredInputDeviceForCapturerInfo(stringParameter);
      console.error(`${TAG} getPreferredInputDeviceForCapturerInfo parameter check ERROR: `+JSON.stringify(data));
      expect().assertFail();
    } catch(e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} getPreferredInputDeviceForCapturerInfo check string parameter PASS`);
      expect(true).assertTrue();
    }
    done();
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest005
   * @tc.desc:Get preferred input device - callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest005", 0, async function (done) {
    let capturerInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      capturerFlags : 0 }

    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredInputDeviceForCapturerInfo(capturerInfo, (e, data)=>{
        if (e) {
          console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
          expect(false).assertTrue();
          done();
        }
        console.info(`${TAG} getPreferredInputDeviceForCapturerInfo SUCCESS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest006
   * @tc.desc:Get preferred input device check number parameter- callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest006", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredInputDeviceForCapturerInfo(numberParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredInputDeviceForCapturerInfo check number parameter PASS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:getPreferredInputDeviceForCapturerInfoTest007
   * @tc.desc:Get preferred input device check string parameter- callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("getPreferredInputDeviceForCapturerInfoTest007", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
      routingManager.getPreferredInputDeviceForCapturerInfo(stringParameter, (e, data)=>{
        if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
          console.error(`${TAG} getPreferredInputDeviceForCapturerInfo ERROR: ${e.message}`);
          expect().assertFail();
          done();
        }
        console.info(`${TAG} getPreferredInputDeviceForCapturerInfo check string parameter PASS`);
        expect(true).assertTrue();
        done();
      });
  })

  /*
   * @tc.name:on_preferredInputDeviceChangeForCapturerInfoTest001
   * @tc.desc:On preferred input device - callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("on_preferredInputDeviceChangeForCapturerInfoTest001", 0, async function (done) {
    let capturerInfo = {
      content : audio.ContentType.CONTENT_TYPE_MUSIC,
      usage : audio.StreamUsage.STREAM_USAGE_MEDIA,
      capturerFlags : 0 }

    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredInputDeviceChangeForCapturerInfo', capturerInfo, (data)=>{});
      expect(true).assertTrue();
      done();
    } catch (e) {
        console.error(`${TAG} on_preferredInputDeviceChangeForCapturerInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
    }
  })

  /*
   * @tc.name:on_preferredInputDeviceChangeForCapturerInfoTest002
   * @tc.desc:On preferred input device check string parameter- callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("on_preferredInputDeviceChangeForCapturerInfoTest002", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredInputDeviceChangeForCapturerInfo', stringParameter, (data)=>{});
      console.error(`${TAG} on_preferredInputDeviceChangeForCapturerInfo with string patameter ERROR: ${e.message}`);
      expect().assertFail();
      done();
    } catch (e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} on_preferredInputDeviceChangeForCapturerInfo check string parameter ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} on_preferredInputDeviceChangeForCapturerInfo PASS: ${e.message}`);
      expect(true).assertTrue();
      done();
    }
  })

  /*
   * @tc.name:on_preferredInputDeviceChangeForCapturerInfoTest003
   * @tc.desc:On preferred input device check number parameter- callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("on_preferredInputDeviceChangeForCapturerInfoTest003", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.on('preferredInputDeviceChangeForCapturerInfo', numberParameter, (data)=>{});
      console.error(`${TAG} on_preferredInputDeviceChangeForCapturerInfo with number patameter ERROR: ${e.message}`);
      expect().assertFail();
      done();
    } catch (e) {
      if (e.code != audio.AudioErrors.ERROR_INVALID_PARAM) {
        console.error(`${TAG} on_preferredInputDeviceChangeForCapturerInfo check number parameter ERROR: ${e.message}`);
        expect().assertFail();
        done();
      }
      console.info(`${TAG} on_preferredInputDeviceChangeForCapturerInfo PASS: ${e.message}`);
      expect(true).assertTrue();
      done();
    }
  })

  /*
   * @tc.name:off_preferredInputDeviceChangeForCapturerInfoTest001
   * @tc.desc:Off preferred input device - callback
   * @tc.type: FUNC
   * @tc.require: I7Q56A
   */
  it("off_preferredInputDeviceChangeForCapturerInfoTest001", 0, async function (done) {
    let routingManager = audio.getAudioManager().getRoutingManager();
    try {
      routingManager.off('preferredInputDeviceChangeForCapturerInfo', (data)=>{});
      console.info(`${TAG} off_preferredInputDeviceChangeForCapturerInfo SUCCESS`);
      expect(true).assertTrue();
      done();
    } catch (e) {
        console.error(`${TAG} off_preferredInputDeviceChangeForCapturerInfo ERROR: ${e.message}`);
        expect().assertFail();
        done();
    }
  })
})
