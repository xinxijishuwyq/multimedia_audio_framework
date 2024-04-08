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

class Option {
  offset: number = 0
  length: number = 0
}

@Entry
@Component
struct OnOff {
  private audioCapturerOptions: audio.AudioCapturerOptions = {
    streamInfo: {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_44100,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    },
    capturerInfo: {
      source: audio.SourceType.SOURCE_TYPE_MIC,
      capturerFlags: 0
    }
  }
  private audioCapturer: audio.AudioCapturer | null = null;
  @State returnMsg: string = `hello`
  @State onReturnMsg: string = `hello`
  @State onMarkReachReturnMsg: string = `hello`
  @State onPeriodReachReturnMsg: string = `hello`
  @State bufferSize: number = 0
  @State isBlockingRead: boolean = true
  @State path: string = ""
  @State markReachFrame: number = 1000
  @State periodReachFrame: number = 1000
  private audioManager: audio.AudioManager | null = null;
  private audioStreamManager: audio.AudioStreamManager | null = null;

  aboutToAppear() {
    this.audioManager = audio.getAudioManager();
    this.audioStreamManager = this.audioManager.getStreamManager()
  }

  onMarkReach() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to onMarkReach `
      return
    }
    // let _this = this
    this.onMarkReachReturnMsg = "markReach 已监听"
    this.audioCapturer.on('markReach', this.markReachFrame, (position) => {
      if (position == this.markReachFrame) {
        this.onMarkReachReturnMsg = `ON Triggered successfully markReachFrame:${this.markReachFrame} `
      }
    })
  }

  offMarkReach() {
    if (this.audioCapturer == null) {
      this.onMarkReachReturnMsg = `AudioCapturer instance had not created,dont‘t allow to offMarkReach `
      return
    }
    this.audioCapturer.off('markReach');
    this.onMarkReachReturnMsg = "markReach 已取消监听"
  }

  onPeriodReach() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to onPeriodReach `
      return
    }

    // let _this = this
    this.onPeriodReachReturnMsg = "periodReach 已监听"
    this.audioCapturer.on('periodReach', this.periodReachFrame, (position) => {
      if (position == this.periodReachFrame) {
        this.onPeriodReachReturnMsg = `ON Triggered successfully periodReachFrame:${this.periodReachFrame} `
      }
    });
  }

  offPeriodReach() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to PeriodReach `
      return
    }
    this.audioCapturer.off('periodReach');
    this.onPeriodReachReturnMsg = "periodReach 已取消监听"
  }

  onStateChange() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to PeriodReach `
      return
    }
    let _this = this
    this.onReturnMsg ="stateChange 已监听"
    this.audioCapturer.on('stateChange', (state) => {
      _this.onReturnMsg = `audio capturer state is: ${state}`
    });
  }

  onAudioCapturerChange(){
    // let _this = this
    if (this.audioStreamManager) {
      this.returnMsg = `onAudioCapturerChange 注册成功\n`
      this.audioStreamManager.on('audioCapturerChange', (AudioCapturerChangeInfoArray) =>  {
        this.returnMsg += `AudioCapturerChangeInfoArray :${JSON.stringify(AudioCapturerChangeInfoArray)}\n`
      });
    }

  }

  offAudioCapturerChange(){
    // let _this = this
    if (this.audioStreamManager) {
      this.audioStreamManager.off('audioCapturerChange');
      this.returnMsg = `取消AudioCapturerChange监听 成功\n`
    }

  }

  createAudioCapturer() {
    if (this.audioCapturer !== null) {
      this.returnMsg = `AudioCapturer Created already,don't create anymore`
      return
    }
    let _this = this
    audio.createAudioCapturer(this.audioCapturerOptions, async (err, data) => {
      if (err) {
        _this.returnMsg = `AudioCapturer Created : Error: ${JSON.stringify(err)}`
      } else {
        _this.audioCapturer = data;
        _this.returnMsg = `AudioCapturer Created : SUCCESS,state:${_this.audioCapturer.state}\n`
        _this.onReturnMsg="无监听"
        _this.onPeriodReachReturnMsg="无监听"
        _this.onMarkReachReturnMsg="无监听"
      }
    })
  }

  capturerStart() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to start\n`
      return
    }
    // let _this = this
    this.audioCapturer.start((err) => {
      if (err) {
        this.returnMsg = `AudioCapturer start : Error: ${JSON.stringify(err)}\n`
      } else {
        if (this.audioCapturer) {
          this.returnMsg = `AudioCapturer start : SUCCESS,state:${this.audioCapturer.state}\n`
        }
      }
    });
  }

  async readCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to read\n`
      return
    }
    if (this.bufferSize == 0) {
      this.bufferSize = await this.audioCapturer.getBufferSize()
    }

    // let _this = this
    //READ 5S
    let start = new Date().getTime()
    let end = new Date().getTime()
    let buffer: ArrayBuffer;

    this.returnMsg += `AudioCapturer read start.....\n `

    let path = globalThis.pathDir + `/test_capturer_${new Date().getTime()}_${this.audioCapturerOptions.streamInfo.samplingRate}_${this.audioCapturerOptions.streamInfo.channels}.pcm`;
    this.path = path
    try {
      await fs.open(path, 0o100);
      this.returnMsg += "文件创建成功，"
    } catch (err) {
      this.returnMsg += `文件创建失败 err：${JSON.stringify(err)}`
      return
    }

    let file: fs.File;
    try {
      file = await fs.open(path, 0o2);
      this.returnMsg += "文件以读写模式打开成功"
    } catch (err) {
      this.returnMsg += `文件以读写模式打开失败 err：${JSON.stringify(err)}`
      return
    }

    this.returnMsg += `fd:${file.fd}\n`;
    let index = 0;
    while (end - start <= 5000) {
      try {
        buffer = await new Promise((resolve, reject) => {
          if (this.audioCapturer) {
            this.audioCapturer.read(this.bufferSize, this.isBlockingRead, async (err, buffer) => {
              if (err) {
                reject(err)
              } else {
                resolve(buffer)
              }
            })
          }

        })
        let options: Option = {
          offset: index * this.bufferSize,
          length: this.bufferSize
        }
        let writeLen = await fs.write(file.fd, buffer, options)
        index++

      } catch (err) {
        this.returnMsg = `AudioCapturer read : Error: ${JSON.stringify(err)}\n`
      }
      end = new Date().getTime()
    }
    this.returnMsg = `AudioCapturer read end, state:${this.audioCapturer.state}\n`
  }

  stopCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to stop\n`
      return
    }
    // let _this = this
    this.audioCapturer.stop((err) => {
      if (err) {
        this.returnMsg = `AudioCapturer stop : Error: ${JSON.stringify(err)}\n`
      } else {
        if (this.audioCapturer) {
          this.returnMsg = `AudioCapturer stop : SUCCESS,state:${this.audioCapturer.state}\n`
        }
      }
    })
  }

  releaseCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to release\n`
      return
    }
    // let _this = this
    this.audioCapturer.release((err) => {
      if (err) {
        this.returnMsg = `AudioCapturer release : Error: ${JSON.stringify(err)}\n`
      } else {
        if (this.audioCapturer) {
          this.returnMsg = `AudioCapturer release SUCCESS,state:${this.audioCapturer.state}\n`
          this.audioCapturer = null
        }

      }
    })
  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("【音频录制-监听】返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(120).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Text("markReach").fontSize(24)
          Row() {
            Slider({
              value: this.markReachFrame,
              min: 0,
              max: 10000,
              step: 1000,
              style: SliderStyle.InSet
            })
              .blockColor('#191970')
              .trackColor('#ADD8E6')
              .selectedColor('#4169E1')
              .showTips(true)
              .onChange((value: number, mode: SliderChangeMode) => {
                this.markReachFrame = value;
                console.info('value:' + value + 'mode:' + mode.toString());
              })
              .width('80%')
            Text(this.markReachFrame.toFixed(0)).fontSize(24)
          }

          Row() {
            Button() {
              Text("on('markReach')").fontSize(22).fontColor(Color.White)
            }.width('71%').height(60).onClick(() => this.onMarkReach())

            Button() {
              Text("off").fontSize(22).fontColor(Color.White)
            }.width('29%').height(60).onClick(() => this.offMarkReach())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Text(this.onMarkReachReturnMsg).fontSize(24)

          Text("periodReach").fontSize(24).margin({ top: 30 })
          Row() {
            Slider({
              value: this.periodReachFrame,
              min: 0,
              max: 10000,
              step: 1000,
              style: SliderStyle.InSet
            })
              .blockColor('#191970')
              .trackColor('#ADD8E6')
              .selectedColor('#4169E1')
              .showTips(true)
              .onChange((value: number, mode: SliderChangeMode) => {
                this.periodReachFrame = value;
                console.info('value:' + value + 'mode:' + mode.toString());
              })
              .width('80%')
            Text(this.periodReachFrame.toFixed(0)).fontSize(24)
          }

          Row() {
            Button() {
              Text("on('periodReach')").fontSize(22).fontColor(Color.White)
            }.width('71%').height(60).onClick(() => this.onPeriodReach())

            Button() {
              Text("off").fontSize(22).fontColor(Color.White)
            }.width('29%').height(60).onClick(() => this.offPeriodReach())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Text(this.onPeriodReachReturnMsg).fontSize(24)

          Row() {
            Button() {
              Text("on('stateChange')").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.onStateChange()
            })
          }.margin({ top: 30 }).width('100%')

          Row() {
            Button() {
              Text("onAudioCapturerChange").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => {
              this.onAudioCapturerChange()
            })

            Button() {
              Text("offAudioCapturerChange").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => {
              this.offAudioCapturerChange()
            })

          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Text(this.onReturnMsg).fontSize(24)


          Row() {
            Button() {
              Text("createAudioCapturer").fontSize(22).fontColor(Color.Blue)
            }.width('80%').height(60).backgroundColor(Color.Pink).onClick(() => {
              this.createAudioCapturer()
            })

            Button() {
              Text("start").fontSize(22).fontColor(Color.Blue)
            }.width('20%').height(60).backgroundColor(Color.Pink).onClick(() => {
              this.capturerStart()
            })
          }.margin({ top: 10 }).width('100%')


          Row() {
            Radio({ value: 'isBlockingRead1', group: "isBlockingReadGroup" }).onChange((isChecked) => {
              if (isChecked) {
                this.isBlockingRead = true;
              } else {
                this.isBlockingRead = false;
              }
            }).checked(this.isBlockingRead == true)
            Text("阻塞").fontSize(18)

            Radio({ value: 'isBlockingRead2', group: "isBlockingReadGroup" }).onChange((isChecked) => {
              if (isChecked) {
                this.isBlockingRead = false;
              } else {
                this.isBlockingRead = true;
              }
            }).checked(this.isBlockingRead == false)
            Text("不阻塞").fontSize(18)
          }

          Row() {
            Text("是否阻塞读操作：" + this.isBlockingRead).fontSize(24)
          }

          Row() {
            Button() {
              Text("read").fontSize(22).fontColor(Color.Blue)
            }.width('100%').height(60).backgroundColor(Color.Pink).onClick(() => {
              this.readCapturer()
            })
          }.margin({ top: 10 }).width('100%')

          Row() {
            Button() {
              Text("stop").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => this.stopCapturer())

            Button() {
              Text("release").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => this.releaseCapturer())
          }.margin({ top: 10, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        }.width('100%')
      }.margin({ top: 130 }).width('100%')

    }
  }
}