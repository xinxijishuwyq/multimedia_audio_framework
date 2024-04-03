/*
* Copyright (C) 2023 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

import { audio } from '@kit.AudioKit';
import fs from '@ohos.file.fs';
import { BusinessError } from '@kit.BasicServicesKit';

class Option {
  offset: number = 0
  length: number = 0
}

class EnumValue {
  value: string = '';
}

class KeyInfo {
  selectedContentTypeKey: string = '';
  selectedStreamUsageKey: string = ''
}

interface AudioRenderInfo {
  main: audio.AudioRendererInfo,
  info: KeyInfo
}

@Entry
@Component
struct Volume {
  private audioRendererOptions: audio.AudioRendererOptions = {
    streamInfo: {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_44100,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    },
    rendererInfo: {
      usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
      rendererFlags: 0
    }
  }
  private audioRenderer: audio.AudioRenderer | null = null;
  @State returnMsg: string = '';
  private audioVolumeGroupManager: audio.AudioVolumeGroupManager | null = null;
  @State selectedVolumeTypeKey: string = "请选择";
  @State selectedVolumeType: number = -1; //音量类型
  private volumeTypeSelectList: EnumValue[] = [];
  @State selectedVolume: number = 0; //音量大小
  @State minVolume: number = 0;
  @State maxVolume: number = 15;
  private volumeSizeList: EnumValue[] = [{ value: '-1' }];
  @State mute: boolean = false;
  private audioVolumeManager: audio.AudioVolumeManager | null = null;
  @State isOnClickedState: string = "unclicked";
  @State onReturnMsg: string = "";
  @State path: string = globalThis.pathDir + '/test_44100_2.wav';
  @State selectedContentTypeKey: string = "CONTENT_TYPE_MUSIC";
  @State selectedStreamUsageKey: string = "STREAM_USAGE_MEDIA";

  private ringToneRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_RINGTONE,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_RINGTONE',
      selectedStreamUsageKey: 'STREAM_USAGE_MEDIA'
    }
  }

  private mediaRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_MUSIC,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_MUSIC',
      selectedStreamUsageKey: 'STREAM_USAGE_MEDIA'
    }
  }

  private voiceCallRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_SPEECH',
      selectedStreamUsageKey: 'STREAM_USAGE_VOICE_COMMUNICATION'
    }
  }

  private voiceAssistantRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_MUSIC,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_SPEECH',
      selectedStreamUsageKey: 'STREAM_USAGE_MEDIA'
    }
  }

  private allRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_MUSIC,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_SPEECH',
      selectedStreamUsageKey: 'STREAM_USAGE_MEDIA'
    }
  }

  private accessibilityRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_ACCESSIBILITY,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_SPEECH',
      selectedStreamUsageKey: 'STREAM_USAGE_ACCESSIBILITY'
    }
  }

  private alarmRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_ALARM,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_MUSIC',
      selectedStreamUsageKey: 'STREAM_USAGE_ALARM'
    }
  }

  private ultrasonicRenderInfo: AudioRenderInfo = {
    main: {
      usage: audio.StreamUsage.STREAM_USAGE_SYSTEM,
      rendererFlags: 0
    },
    info: {
      selectedContentTypeKey: 'CONTENT_TYPE_ULTRASONIC',
      selectedStreamUsageKey: 'STREAM_USAGE_SYSTEM'
    }
  }

  async aboutToAppear() {
    Object.keys(audio.AudioVolumeType).forEach((key) => {
      this.volumeTypeSelectList.push({ value: key });
    })

    let audioManager = audio.getAudioManager();
    this.audioVolumeManager = audioManager.getVolumeManager();
    let groupid = audio.DEFAULT_VOLUME_GROUP_ID;
    let _this = this;
    this.audioVolumeManager.getVolumeGroupManager(groupid, (err, value) => {
      if (err) {
        _this.returnMsg = "getVolumeGroupManager failed. err:" + JSON.stringify(err);
        _this.returnMsg += "groupid:" + groupid;
        return;
      }
      _this.returnMsg += "getVolumeGroupManager success";
      _this.audioVolumeGroupManager = value;
    });
  }

  onBackPress() {
    if (this.audioRenderer !== null) {
      this.audioRenderer.release().then(() => {
        console.info('Renderer released successfully');
        this.audioRenderer = null;
      }).catch((err: BusinessError) => {
        console.error(`ERROR: ${err}`);
      });
    }
  }

  async renderPlay() {
    if (this.audioRenderer !== null) {
      await this.audioRenderer.release();
      this.audioRenderer = null;
    }
    this.returnMsg = `audioRendererOptions ${JSON.stringify(this.audioRendererOptions)} \n`;
    console.log(`audioRendererOptions ${JSON.stringify(this.audioRendererOptions)} \n`);

    try {
      this.audioRenderer = await audio.createAudioRenderer(this.audioRendererOptions);
      this.returnMsg = `audioRenderer create success \n`;
    } catch (err) {
      this.returnMsg = `audioRenderer create : Error: ${JSON.stringify(err)}\n`;
      return;
    }
    let bufferSize: number;
    try {
      bufferSize = await this.audioRenderer.getBufferSize();
      this.returnMsg = `audioRenderer getBufferSize success,bufferSize:${bufferSize} \n`;
      await this.audioRenderer.start();
      this.returnMsg = `audioRenderer start success \n`;
    } catch (err) {
      this.returnMsg = `audioRenderer start : Error: ${JSON.stringify(err)}\n`;
      return;
    }


    let path = this.path;
    try {
      this.returnMsg = `path:${path}\n`;
      let stat = await fs.stat(path);
      this.returnMsg = `stat:${JSON.stringify(stat)}\n`;
      this.returnMsg = `size:${stat.size}\n`;
      let len = stat.size % bufferSize == 0 ? Math.floor(stat.size / bufferSize) : Math.floor(stat.size / bufferSize + 1);

      let file = await fs.open(path, 0o0);
      this.returnMsg = `fd:${file.fd}\n`;

      let buf = new ArrayBuffer(bufferSize);
      this.returnMsg = `audioRenderer write start.......... \n`;
      for (let i = 0;i < len; i++) {
        if (this.audioRenderer.state > 2) {
          return;
        }
        let options: Option = {
          offset: i * bufferSize,
          length: bufferSize
        }
        let readsize = await fs.read(file.fd, buf, options);
        let writeSize = await this.audioRenderer.write(buf);
      }
      this.returnMsg = `audioRenderer write end. \n`;
    } catch (err) {
      this.returnMsg = `audioRenderer write : Error: ${JSON.stringify(err)}\n`;
    }

    await this.audioRenderer.release();
    this.audioRenderer = null;

  }

  async getVolumeRange() {
    if (!this.audioVolumeGroupManager) {
      this.returnMsg = "AudioVolumeGroupManager instance had not created";
      return;
    }
    this.minVolume = await this.audioVolumeGroupManager.getMinVolume(this.selectedVolumeType);
    this.maxVolume = await this.audioVolumeGroupManager.getMaxVolume(this.selectedVolumeType);
    this.returnMsg = "minVolume:" + this.minVolume + "; maxVolume:" + this.maxVolume;
  }

  build() {

    Row() {
      Row() {
        Column() {
          Text("【音量】返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }
        .width('98%')
        .height(70)
        .backgroundColor(Color.Orange)
        .position({ x: '1%' })
        .zIndex(999)
      }.position({ x: 0, y: 0 }).width('100%');
      Scroll() {

        Column() {
          Column() {
            Row() {
              Select(this.volumeTypeSelectList)
                .value(this.selectedVolumeTypeKey)
                .onSelect(async(index, value) => {
                  this.selectedVolumeType = audio.AudioVolumeType[value];
                  this.selectedVolumeTypeKey = value;
                  switch (this.selectedVolumeTypeKey) {
                    case 'RINGTONE':
                      this.audioRendererOptions.rendererInfo = this.ringToneRenderInfo.main;
                      this.selectedStreamUsageKey = this.ringToneRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.ringToneRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'MEDIA':
                      this.audioRendererOptions.rendererInfo = this.mediaRenderInfo.main;
                      this.selectedStreamUsageKey = this.mediaRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.mediaRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'VOICE_CALL':
                      this.audioRendererOptions.rendererInfo = this.voiceCallRenderInfo.main;
                      this.selectedStreamUsageKey = this.voiceCallRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.voiceCallRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'VOICE_ASSISTANT':
                      this.audioRendererOptions.rendererInfo = this.voiceAssistantRenderInfo.main;
                      this.selectedStreamUsageKey = this.voiceAssistantRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.voiceAssistantRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'ALL':
                      this.audioRendererOptions.rendererInfo = this.allRenderInfo.main;
                      this.selectedStreamUsageKey = this.allRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.allRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'ALARM':
                      this.audioRendererOptions.rendererInfo = this.alarmRenderInfo.main;
                      this.selectedStreamUsageKey = this.alarmRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.alarmRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'ACCESSIBILITY':
                      this.audioRendererOptions.rendererInfo = this.accessibilityRenderInfo.main;
                      this.selectedStreamUsageKey = this.accessibilityRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.accessibilityRenderInfo.info.selectedContentTypeKey;
                      break;
                    case 'ULTRASONIC':
                      this.audioRendererOptions.rendererInfo = this.ultrasonicRenderInfo.main;
                      this.selectedStreamUsageKey = this.ultrasonicRenderInfo.info.selectedStreamUsageKey;
                      this.selectedContentTypeKey = this.ultrasonicRenderInfo.info.selectedContentTypeKey;
                  }


                  this.getVolumeRange();
                  if(this.audioRenderer!==null){
                    await this.audioRenderer.release()
                    this.audioRenderer = null
                  }
                  this.renderPlay()
                })
            }

            if (this.selectedVolumeTypeKey !== "请选择") {

              Divider().strokeWidth(1).color(Color.Blue).margin({ bottom: 20 })
              Row() {
                Text("音量").fontSize(20)
                Slider({
                  value: this.selectedVolume,
                  min: this.minVolume,
                  max: this.maxVolume,
                  step: 1,
                  style: SliderStyle.InSet
                })
                  .blockColor('#191970')
                  .trackColor('#ADD8E6')
                  .selectedColor('#4169E1')
                  .showTips(true)
                  .onChange((value: number, mode: SliderChangeMode) => {
                    this.selectedVolume = value;
                    console.info('value:' + value + 'mode:' + mode.toString());
                  })
                  .width('80%')
                Text(this.selectedVolume.toFixed(0)).fontSize(24)
              }

              Text(`${this.selectedContentTypeKey}`).fontSize(16).lineHeight(40)
              Text(`${this.selectedStreamUsageKey}`).fontSize(16).lineHeight(40)
            }

          }.width('100%')

          Divider().strokeWidth(10).color(Color.Blue).margin({ top: 20 })

          Column() {
            Row() {
              Button("on('volumeChange')").width('80%').fontSize(20).height(60)
                .onClick(() => {
                  this.isOnClickedState = "clicked";
                  let _this = this;
                  if (!this.audioVolumeManager) {
                    this.returnMsg = "AudioVolumeManager instance had not created";
                    return;
                  }
                  this.audioVolumeManager.on('volumeChange', (volumeEvent) => {
                    _this.isOnClickedState = "clicked_callback";
                    _this.onReturnMsg = JSON.stringify(volumeEvent);
                  });
                })
            }.margin({ top: 0 }).width('100%').justifyContent(FlexAlign.Center);

            if (this.isOnClickedState == "clicked") {
              Text("监听已触发").fontSize(18)
            } else if (this.isOnClickedState == "unclicked") {
              Text("监听尚未触发").fontSize(18)
            } else if (this.isOnClickedState == "clicked_callback") {
              Text("收到的回调数据:" + this.onReturnMsg).fontSize(18)
            }

            Row() {
              Button() {
                Text("setVolume callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick(() => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                _this.returnMsg = this.selectedVolumeType.toString() + ',' + this.selectedVolume.toString()
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.setVolume(this.selectedVolumeType, this.selectedVolume, (err) => {
                  if (err) {
                    _this.returnMsg = 'setVolumeCallback:' + JSON.stringify(err);
                    return;
                  }
                  _this.returnMsg = "setVolumeCallback:SUCCESS"
                })
              })

              Button() {
                Text("setVolume promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick(async () => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.setVolume(this.selectedVolumeType, this.selectedVolume).then(() => {
                  _this.returnMsg = "setVolumePromise:SUCCESS"
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'setVolumePromise err:' + JSON.stringify(err);
                })
              })
            }


            Row() {
              Button() {
                Text("getVolume callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getVolume(this.selectedVolumeType, (err, value) => {
                  if (err) {
                    _this.returnMsg = 'getVolumeCallback:' + JSON.stringify(err);
                  }
                  _this.returnMsg = "getVolumeCallback:SUCCESS,返回值:" + value
                })
              })

              Button() {
                Text("getVolume promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getVolume(this.selectedVolumeType).then((value) => {
                  _this.returnMsg = "getVolumePromise:SUCCESS,返回值:" + value
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'getVolumePromise:' + JSON.stringify(err);
                })
              })
            }.margin({ top: 10 })

            Row() {
              Button() {
                Text("getMinVolume callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getMinVolume(this.selectedVolumeType, (err, value) => {
                  if (err) {
                    _this.returnMsg = 'getMinVolumeCallback:' + JSON.stringify(err);
                  }
                  _this.returnMsg = "getMinVolumeCallback:SUCCESS,返回值:" + value
                })
              })

              Button() {
                Text("getMinVolume promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getMinVolume(this.selectedVolumeType).then((value) => {
                  _this.returnMsg = "getMinVolumePromise:SUCCESS,返回值:" + value
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'getMinVolumePromise:' + JSON.stringify(err);
                })
              })
            }.margin({ top: 10 })

            Row() {
              Button() {
                Text("getMaxVolume callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getMaxVolume(this.selectedVolumeType, (err, value) => {
                  if (err) {
                    _this.returnMsg = 'getMaxVolumeCallback:' + JSON.stringify(err);
                  }
                  _this.returnMsg = "getMaxVolumeCallback:SUCCESS,返回值:" + value
                })
              })

              Button() {
                Text("getMaxVolume promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.getMaxVolume(this.selectedVolumeType).then((value) => {
                  _this.returnMsg = "getMaxVolumePromise:SUCCESS,返回值:" + value
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'getMaxVolumePromise:' + JSON.stringify(err);
                })
              })
            }.margin({ top: 10 })

            Row() {
              Text("是否静音").fontSize(24)
              Radio({ value: 'mute1', group: "muteGroup" }).onChange((isChecked) => {
                if (isChecked) {
                  this.mute = true;
                } else {
                  this.mute = false;
                }
              }).checked(this.mute == true)
              Text("静音").fontSize(18)

              Radio({ value: 'mute2', group: "muteGroup" }).onChange((isChecked) => {
                if (isChecked) {
                  this.mute = false;
                } else {
                  this.mute = true;
                }
              }).checked(this.mute == false)
              Text("取消静音").fontSize(18)
            }

            Row() {
              Text("静音数据：" + this.mute).fontSize(24)
            }

            Row() {
              Button() {
                Text("mute callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.mute(this.selectedVolumeType, this.mute, (err) => {
                  if (err) {
                    _this.returnMsg = 'muteCallback err:' + JSON.stringify(err);
                    return;
                  }
                  _this.returnMsg = "muteCallback:SUCCESS"
                })
              })

              Button() {
                Text("mute promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.mute(this.selectedVolumeType, this.mute).then(() => {
                  _this.returnMsg = "mutePromise:SUCCESS"
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'mutePromise err:' + JSON.stringify(err);
                })
              })
            }.margin({ top: 10 })

            Row() {
              Button() {
                Text("isMute callback").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60).margin({ right: 10 })
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.isMute(this.selectedVolumeType, (err, value) => {
                  if (err) {
                    _this.returnMsg = 'isMuteCallback:' + JSON.stringify(err);
                  }
                  _this.returnMsg = "isMuteCallback:SUCCESS,返回值：" + value
                })
              })

              Button() {
                Text("isMute promise").fontSize(20).fontColor(Color.White)
              }.width('40%').height(60)
              .onClick((event) => {
                if (this.selectedVolumeType == -1) {
                  this.returnMsg = "请选择音频流类型"
                  return
                }
                let _this = this;
                if (!this.audioVolumeGroupManager) {
                  this.returnMsg = "AudioVolumeGroupManager instance had not created";
                  return;
                }
                this.audioVolumeGroupManager.isMute(this.selectedVolumeType).then((value) => {
                  _this.returnMsg = "isMutePromise:SUCCESS,返回值：" + value
                }).catch((err: BusinessError) => {
                  _this.returnMsg = 'isMutePromise:' + JSON.stringify(err);
                })
              })
            }.margin({ top: 10 })

          }.width('100%').margin({ top: 20 })
        }
      }.width('100%').margin({ top: 80 })
    }
    .height('100%').width('100%');

  }
}