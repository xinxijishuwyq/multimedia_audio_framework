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

class EnumValue {
  value: string = '';
}
class Option {
  offset: number = 0
  length: number = 0
}
@Entry
@Component
struct SingleInstanceCallback {
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
  private audioRenderer: audio.AudioRenderer | null = null;
  private samplingRateList: EnumValue[] = [];
  @State selectedSamplingRateKey: string = "SAMPLE_RATE_44100";
  private channelsList: EnumValue[] = [];
  @State selectedChannelKey: string = "CHANNEL_2";
  private sampleFormatList: EnumValue[] = [];
  @State selectedsampleFormatKey: string = "SAMPLE_FORMAT_S16LE";
  private encodingTypeList: EnumValue[] = [];
  @State selectedEncodingTypeKey: string = "ENCODING_TYPE_RAW";
  private sourceTypeList: EnumValue[] = [];
  @State selectedSourceTypeKey: string = "SOURCE_TYPE_MIC";
  @State returnMsg: string = `hello`;
  @State bufferSize: number = 0;
  @State isBlockingRead: boolean = true;
  @State path: string = "";
  private audioManager: audio.AudioManager | null = null;
  private audioStreamManager: audio.AudioStreamManager | null = null;

  aboutToAppear() {
    Object.keys(audio.AudioSamplingRate).forEach((key) => {
      this.samplingRateList.push({ value: key })
    })

    Object.keys(audio.AudioChannel).forEach((key) => {
      this.channelsList.push({ value: key })
    })

    Object.keys(audio.AudioSampleFormat).forEach((key) => {
      this.sampleFormatList.push({ value: key })
    })

    Object.keys(audio.AudioEncodingType).forEach((key) => {
      this.encodingTypeList.push({ value: key })
    })

    Object.keys(audio.SourceType).forEach((key) => {
      this.sourceTypeList.push({ value: key })
    })
    this.audioManager = audio.getAudioManager();
    this.audioStreamManager = this.audioManager.getStreamManager()
  }

  createAudioCapturer() {
    if (this.audioCapturer !== null) {
      this.returnMsg += `AudioCapturer Created already,don't create anymore`
      return
    }
    let _this = this
    audio.createAudioCapturer(this.audioCapturerOptions)
      .then(data => {
        _this.audioCapturer = data;
        _this.returnMsg = `采集器参数：\n 采样率：${this.selectedSamplingRateKey}\n 声道：${this.selectedChannelKey}\n 采样格式：${this.selectedsampleFormatKey}\n 编码格式：${this.selectedEncodingTypeKey}\n 音源类型：${this.selectedSourceTypeKey}\n`
        _this.returnMsg += `AudioCapturer Created : SUCCESS,state:${_this.audioCapturer.state}\n`
      }).catch((err: BusinessError) => {
      _this.returnMsg += `AudioCapturer Created : Error: ${JSON.stringify(err)}`
    })
  }
  onAudioCapturerChange(){
    let _this = this
    _this.returnMsg = `onAudioCapturerChange 注册成功\n`
    if (this.audioStreamManager) {
      this.audioStreamManager.on('audioCapturerChange', (AudioCapturerChangeInfoArray) =>  {
        _this.returnMsg += `AudioCapturerChangeInfoArray :${JSON.stringify(AudioCapturerChangeInfoArray)}\n`
      });
    }

  }

  offAudioCapturerChange(){
    let _this = this
    if (this.audioStreamManager) {
      this.audioStreamManager.off('audioCapturerChange');
    }
    _this.returnMsg = `取消AudioCapturerChange监听 成功\n`
  }
  capturerStart() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to start\n`
      return
    }
    let _this = this
    this.audioCapturer.start()
      .then(() => {
        if (_this.audioCapturer) {
          _this.returnMsg += `AudioCapturer start : SUCCESS,state:${_this.audioCapturer.state}\n`
        }

      }).catch((err: BusinessError) => {
      _this.returnMsg += `AudioCapturer start : Error: ${JSON.stringify(err)}\n`
    })

  }

  getBufferSize() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to getBufferSize`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.getBufferSize()
        .then(bufferSize => {
          if (_this.audioCapturer) {
            _this.returnMsg += `AudioCapturer getBufferSize : SUCCESS,bufferSize:${bufferSize},state:${_this.audioCapturer.state}\n`
          }

          _this.bufferSize = bufferSize
        }).catch((err: BusinessError) => {
        _this.returnMsg += `AudioCapturer getBufferSize : Error: ${JSON.stringify(err)}\n`
      })
    }

  }

  async readCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to read\n`
      return
    }
    if (this.bufferSize == 0) {
      this.bufferSize = await this.audioCapturer.getBufferSize()
    }

    let _this = this
    //READ 5S
    let start = new Date().getTime()
    let end = new Date().getTime()
    let buffer: ArrayBuffer | null = null

    _this.returnMsg += `AudioCapturer read start.....\n `

    let path = globalThis.pathDir + `/test_capturer_${new Date().getTime()}_${this.audioCapturerOptions.streamInfo.samplingRate}_${this.audioCapturerOptions.streamInfo.channels}.pcm`;
    _this.path = path
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
          if (_this.audioCapturer) {
            _this.audioCapturer.read(_this.bufferSize, _this.isBlockingRead)
              .then((buffer) => {
                resolve(buffer)
              }).catch((err: BusinessError) => {
              reject(err)
            })
          }
        })
        let options: Option = {
          offset: index * _this.bufferSize,
          length: _this.bufferSize
        }
        console.log('options::::' + JSON.stringify(options))
        let writeLen = await fs.write(file.fd, buffer, options)
        console.log('fs.write writeLen:' + writeLen)
        index++

      } catch (err) {
        _this.returnMsg += `AudioCapturer read : Error: ${JSON.stringify(err)}\n`
      }
      end = new Date().getTime()
    }
    if (_this.audioCapturer) {
      _this.returnMsg += `AudioCapturer read end, state:${_this.audioCapturer.state}\n`
    }

  }

  stopCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to stop\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.stop()
        .then(() => {
          if (_this.audioCapturer) {
            _this.returnMsg += `AudioCapturer stop : SUCCESS,state:${_this.audioCapturer.state}\n`
          }

        }).catch((err: BusinessError) => {
        _this.returnMsg += `AudioCapturer stop : Error: ${JSON.stringify(err)}\n`
      })
    }

  }

  releaseCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to release\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.release()
        .then(() => {
          if (_this.audioCapturer) {
            _this.returnMsg += `AudioCapturer release SUCCESS,state:${_this.audioCapturer.state}\n`
          }
          _this.audioCapturer = null
        }).catch((err: BusinessError) => {
        _this.returnMsg += `AudioCapturer release : Error: ${JSON.stringify(err)}\n`
      })
    }

  }

  async renderPlay() {
    if(this.path==""){
      this.returnMsg += `AudioCapturer 尚未录音\n`
      return
    }
    let audioRendererOptions: audio.AudioRendererOptions = {
      streamInfo: this.audioCapturerOptions.streamInfo,
      rendererInfo: {
        usage: audio.StreamUsage.STREAM_USAGE_MUSIC,
        rendererFlags: 0
      }
    }
    this.returnMsg = `audioRendererOptions ${JSON.stringify(audioRendererOptions)} \n`
    try {
      this.audioRenderer = await audio.createAudioRenderer(audioRendererOptions)
      this.returnMsg += `audioRenderer create success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer create : Error: ${JSON.stringify(err)}\n`
      return
    }
    let bufferSize: number = 0;
    try {
      bufferSize = await this.audioRenderer.getBufferSize()
      this.returnMsg += `audioRenderer getBufferSize success,bufferSize:${bufferSize} \n`
      await this.audioRenderer.start()
      this.returnMsg += `audioRenderer start success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer start : Error: ${JSON.stringify(err)}\n`
      return
    }


    let path = this.path
    try {
      this.returnMsg += `path:${path}\n`
      let stat = await fs.stat(path);
      this.returnMsg += `stat:${JSON.stringify(stat)}\n`;
      this.returnMsg += `size:${stat.size}\n`;
      let len = stat.size % bufferSize == 0 ? Math.floor(stat.size / bufferSize) : Math.floor(stat.size / bufferSize + 1);

      let file = await fs.open(path, 0o0);
      this.returnMsg += `fd:${file.fd}\n`;

      let buf = new ArrayBuffer(bufferSize);
      this.returnMsg += `audioRenderer write start.......... \n`;
      for (let i = 0;i < len; i++) {
        let options: Option = {
          offset: i * bufferSize,
          length: bufferSize
        }
        let readsize = await fs.read(file.fd, buf, options);
        let writeSize = await this.audioRenderer.write(buf);
      }
      this.returnMsg += `audioRenderer write end. \n`;
    } catch (err) {
      this.returnMsg += `audioRenderer write : Error: ${JSON.stringify(err)}\n`
    }

    await this.audioRenderer.release()
  }

  getCurrentAudioCapturerInfoArray(){
    let _this = this
    if (this.audioStreamManager) {
      this.audioStreamManager.getCurrentAudioCapturerInfoArray().then((AudioCapturerChangeInfoArray) => {
        console.info('getCurrentAudioCapturerInfoArray **** Get Promise Called ****');
        if (AudioCapturerChangeInfoArray != null) {
          _this.returnMsg =`getCurrentAudioCapturerInfoArray :Success: ${JSON.stringify(AudioCapturerChangeInfoArray)}`;
        }
      }).catch((err: BusinessError) => {
        _this.returnMsg =`getCurrentAudioCapturerInfoArray :ERROR: ${err}`;
      });
    }

  }




  getCapturerInfo() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to getCapturerInfo\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.getCapturerInfo()
        .then((capturerInfo) => {
          _this.returnMsg = `AudioCapturer getCapturerInfo : Success: ${JSON.stringify(capturerInfo)}`
        }).catch((err: BusinessError) => {
        _this.returnMsg = `AudioCapturer getCapturerInfo : Error: ${JSON.stringify(err)}`
      })
    }

  }

  getStreamInfo() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to getStreamInfo\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.getStreamInfo()
        .then(streamInfo => {
          _this.returnMsg = `AudioCapturer getStreamInfo : Success: ${JSON.stringify(streamInfo)}`
        }).catch((err: BusinessError) => {
        _this.returnMsg = `AudioCapturer getStreamInfo : Error: ${JSON.stringify(err)}`
      })
    }

  }

  getAudioTime() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to getAudioTime\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.getAudioTime()
        .then(timestamp => {
          _this.returnMsg = `AudioCapturer getAudioTime Current timestamp: ${JSON.stringify(timestamp)}`
        }).catch((err: BusinessError) => {
        _this.returnMsg = `AudioCapturer getAudioTime : Error: ${JSON.stringify(err)}`
      })
    }

  }

  getAudioStreamId() {
    if (this.audioCapturer == null) {
      this.returnMsg += `AudioCapturer instance had not created,dont‘t allow to getAudioStreamId\n`
      return
    }
    let _this = this
    if (_this.audioCapturer) {
      _this.audioCapturer.getAudioStreamId()
        .then(streamid => {
          _this.returnMsg = `audioCapturer GetStreamId: ${streamid}`
        }).catch((err: BusinessError) => {
        _this.returnMsg = `AudioCapturer GetStreamId : Error: ${JSON.stringify(err)}`
      })
    }

  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("【音频录制-单实例-promise】返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(300).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Row() {
            Select(this.samplingRateList).value(this.selectedSamplingRateKey)
              .onSelect((index, value) => {
                this.selectedSamplingRateKey = value
                this.audioCapturerOptions.streamInfo.samplingRate = audio.AudioSamplingRate[value]

              }).font({ size: 22 })
          }.margin({ top: 0 })

          Row() {
            Select(this.channelsList).value(this.selectedChannelKey)
              .onSelect((index, value) => {
                this.selectedChannelKey = value
                this.audioCapturerOptions.streamInfo.channels = audio.AudioChannel[value]
              })
          }.margin({ top: 10 })

          Row() {
            Select(this.sampleFormatList).value(this.selectedsampleFormatKey)
              .onSelect((index, value) => {
                this.selectedsampleFormatKey = value
                this.audioCapturerOptions.streamInfo.sampleFormat = audio.AudioSampleFormat[value]
              })
          }.margin({ top: 10 })

          Row() {
            Select(this.encodingTypeList).value(this.selectedEncodingTypeKey)
              .onSelect((index, value) => {
                this.selectedEncodingTypeKey = value
                this.audioCapturerOptions.streamInfo.encodingType = audio.AudioEncodingType[value]
              })
          }.margin({ top: 10 })
          Divider().strokeWidth(6).color(Color.Blue)

          Row() {
            Select(this.sourceTypeList).value(this.selectedSourceTypeKey)
              .onSelect((index, value) => {
                this.selectedSourceTypeKey = value
                this.audioCapturerOptions.capturerInfo.source = audio.SourceType[value]
              })
          }.margin({ top: 10 })


          Row() {
            Button() {
              Text("createAudioCapturer promise").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.createAudioCapturer()
            })
          }.margin({ top: 10 }).width('100%')

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

          Row() {
            Button() {
              Text("getCurrentAudioCapturerInfoArray Promise").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.getCurrentAudioCapturerInfoArray()
            })
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("start promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => {
              this.capturerStart()
            })

            Button() {
              Text("getBufferSize promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => {
              this.getBufferSize()
            })

          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

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
              Text("read promise").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.readCapturer()
            })
          }.margin({ top: 10 }).width('100%')

          Row() {
            Button() {
              Text("stop promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.stopCapturer())

            Button() {
              Text("release promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.releaseCapturer())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("播放录音数据").fontSize(22).fontColor(Color.Blue)
            }.width('100%').height(60).onClick(() => this.renderPlay()).backgroundColor(Color.Pink)

          }.margin({ top: 10, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("getCapturerInfo promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.getCapturerInfo())

            Button() {
              Text("getStreamInfo promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.getStreamInfo())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("getAudioStreamId promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.getAudioStreamId())

            Button() {
              Text("getAudioTime promise").fontSize(22).fontColor(Color.White)
            }.width('49%').height(60).onClick(() => this.getAudioTime())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        }.width('100%')
      }.margin({ top: 310 }).width('100%')

    }
  }
}