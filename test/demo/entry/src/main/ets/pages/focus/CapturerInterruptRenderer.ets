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

import router from '@ohos.router';
import audio from '@ohos.multimedia.audio'
import fs from '@ohos.file.fs';

@Entry
@Component
struct CapturerInterruptRenderer {
  private audioCapturerOption =
    {
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
  private audioCapturer = null
  private capturerPath=''
  @State returnMsg: string = `hello`
  @State onCapturerReturnMsg: string = "未监听"
  @State bufferSize: number = 0
  @State isBlockingRead: boolean = true
  private sourceTypeList = []
  @State selectedSourceTypeKey: string = "请选择音源类型"



  private audioRenderer =null
  @State onRendererReturnMsg: string = `未监听`
  private audioRendererOption={
    streamInfo: {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_44100,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    },
    rendererInfo: {
      content: audio.ContentType.CONTENT_TYPE_SPEECH,
      usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
      rendererFlags: 0
    }
  }
  private volumeTypeList = []
  @State selectedVolumeTypeKey: string = "请选择流类型"
  @State selectedContentTypeKey: string = "CONTENT_TYPE_MUSIC"
  @State selectedStreamUsageKey: string = "STREAM_USAGE_MEDIA"
  @State mode:number=1
  @State writeArr:number=0

  private renderInfo = {
    'RINGTONE': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_RINGTONE,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_RINGTONE',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    'MEDIA': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_MUSIC,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_MUSIC',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    'VOICE_CALL': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_SPEECH,
        usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_SPEECH',
        selectedStreamUsageKey:'STREAM_USAGE_VOICE_COMMUNICATION'
      }
    },
    'VOICE_ASSISTANT': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_SPEECH,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_SPEECH',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    'ALL': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_SPEECH,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_SPEECH',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    'ALARM': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_MUSIC,
        usage: audio.StreamUsage.STREAM_USAGE_ALARM,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_MUSIC',
        selectedStreamUsageKey:'STREAM_USAGE_ALARM'
      }
    },
    'ACCESSIBILITY': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_SPEECH,
        usage: audio.StreamUsage.STREAM_USAGE_ACCESSIBILITY,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_SPEECH',
        selectedStreamUsageKey:'STREAM_USAGE_ACCESSIBILITY'
      }
    },
    'ULTRASONIC': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_ULTRASONIC,
        usage: audio.StreamUsage.STREAM_USAGE_SYSTEM,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_ULTRASONIC',
        selectedStreamUsageKey:'STREAM_USAGE_SYSTEM'
      }
    },
    '__SPEECH': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_SPEECH,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_SPEECH',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    '__MOVIE': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_MOVIE,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_MOVIE',
        selectedStreamUsageKey:'STREAM_USAGE_MEDIA'
      }
    },
    '__UNKNOW': {
      main:{
        content: audio.ContentType.CONTENT_TYPE_UNKNOWN,
        usage: audio.StreamUsage.STREAM_USAGE_UNKNOWN,
        rendererFlags: 0
      },
      info:{
        selectedContentTypeKey:'CONTENT_TYPE_UNKNOWN',
        selectedStreamUsageKey:'STREAM_USAGE_UNKNOWN'
      }
    }
  }

  aboutToAppear() {
    for (let key in audio.SourceType) {
      if (key === "SOURCE_TYPE_INVALID") {
        continue
      }
      this.sourceTypeList.push({ value: key })
    }

    for (let key in audio.AudioVolumeType) {
      this.volumeTypeList.push({ value: key })
    }
    //实际volumetype没有，这三个流类型与music的流类型策略一致
    this.volumeTypeList.push({ value: '__SPEECH' })
    this.volumeTypeList.push({ value: '__MOVIE' })
    this.volumeTypeList.push({ value: '__UNKNOW' })
  }

  async onBackPress() {
    if (this.audioCapturer !== null) {
      await this.audioCapturer.release()
      this.audioCapturer = null
    }

    if (this.audioRenderer== null) {
      await this.audioRenderer.release()
      this.audioRenderer = null
    }

  }

  createAudioCapturer() {
    if (this.audioCapturer !== null) {
      this.returnMsg = `AudioCapturer  Created already,don't create anymore`
      return
    }
    let _this = this
    audio.createAudioCapturer(this.audioCapturerOption, async (err, data) => {
      if (err) {
        _this.returnMsg = `AudioCapturer  Created : Error: ${JSON.stringify(err)}`
      } else {
        _this.audioCapturer = data;
        _this.returnMsg = `AudioCapturer  Created : SUCCESS,state:${_this.audioCapturer.state}\n`
        _this.returnMsg += `${JSON.stringify(this.audioCapturerOption)}`
      }
    })
  }

  capturerStart() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer  instance had not created,dont‘t allow to start\n`
      return
    }
    let _this = this
    this.audioCapturer.start((err) => {
      if (err) {
        _this.returnMsg = `AudioCapturer  start : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `AudioCapturer  start : SUCCESS,state:${_this.audioCapturer.state}\n`
      }
    });
  }

  async readCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer  instance had not created,dont‘t allow to read\n`
      return
    }
    if (this.bufferSize == 0) {
      this.bufferSize = await this.audioCapturer.getBufferSize()
    }

    let _this = this
    //READ 5S
    let start = new Date().getTime()
    let end = new Date().getTime()
    let buffer = null

    _this.returnMsg += `AudioCapturer  read start.....\n `

    let path = globalThis.pathDir + `/test_capturer_${new Date().getTime()}_${this.audioCapturerOption.streamInfo.samplingRate}_${this.audioCapturerOption.streamInfo.channels}.pcm`;
    _this.capturerPath = path
    try {
      await fs.open(path, 0o100);
      this.returnMsg += "文件创建成功，"
    } catch (err) {
      this.returnMsg += `文件创建失败 err：${JSON.stringify(err)}`
      return
    }

    let file
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
          _this.audioCapturer.read(_this.bufferSize, _this.isBlockingRead, async (err, buffer) => {
            if (err) {
              reject(err)
            } else {
              resolve(buffer)
            }
          })
        })
        let options = {
          offset: index * _this.bufferSize,
          length: _this.bufferSize
        }
        let writeLen = await fs.write(file.fd, buffer, options)
        index++

      } catch (err) {
        _this.returnMsg = `AudioCapturer  read : Error: ${JSON.stringify(err)}\n`
      }
      end = new Date().getTime()
    }
    _this.returnMsg = `AudioCapturer  read end, state:${_this.audioCapturer.state}\n`
  }

  stopCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer instance had not created,dont‘t allow to stop\n`
      return
    }
    let _this = this
    _this.audioCapturer.stop((err) => {
      if (err) {
        _this.returnMsg = `AudioCapturer  stop : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `AudioCapturer  stop : SUCCESS,state:${_this.audioCapturer.state}\n`
      }
    })
  }

  releaseCapturer() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer  instance had not created,dont‘t allow to release\n`
      return
    }
    let _this = this
    _this.audioCapturer.release((err) => {
      if (err) {
        _this.returnMsg = `AudioCapturer  release : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `AudioCapturer  release SUCCESS,state:${_this.audioCapturer.state}\n`
        _this.audioCapturer = null
      }
    })
  }

  onCapturerAudioInterrupt() {
    if (this.audioCapturer == null) {
      this.returnMsg = `AudioCapturer  instance had not created,dont‘t allow to release\n`
      return
    }
    let _this = this
    _this.onCapturerReturnMsg = `监听中...`
    _this.audioCapturer.on('audioInterrupt', (InterruptEvent) => {
      _this.onCapturerReturnMsg = JSON.stringify(InterruptEvent)
    })
  }

  async renderPlay() {
    if (this.capturerPath == "") {
      this.returnMsg += `AudioCapturer  尚未录音\n`
      return
    }
    let audioRendererOptions = {
      streamInfo: this.audioCapturerOption.streamInfo,
      rendererInfo: {
        content: audio.ContentType.CONTENT_TYPE_RINGTONE,
        usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
        rendererFlags: 0
      }
    }
    this.returnMsg = `audioRendererOptions ${JSON.stringify(audioRendererOptions)} \n`
    try {
      this.audioRenderer = await audio.createAudioRenderer(audioRendererOptions)
      this.returnMsg += `audioRenderer  create success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer  create : Error: ${JSON.stringify(err)}\n`
      return
    }
    let bufferSize
    try {
      bufferSize = await this.audioRenderer.getBufferSize()
      this.returnMsg += `audioRenderer getBufferSize success,bufferSize:${bufferSize} \n`
      await this.audioRenderer.start()
      this.returnMsg += `audioRenderer  start success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer  start : Error: ${JSON.stringify(err)}\n`
      return
    }


    let path = this.capturerPath
    try {
      this.returnMsg += `path:${path}\n`
      let stat = await fs.stat(path);
      this.returnMsg += `stat:${JSON.stringify(stat)}\n`;
      this.returnMsg += `size:${stat.size}\n`;
      let len = stat.size % bufferSize == 0 ? Math.floor(stat.size / bufferSize) : Math.floor(stat.size / bufferSize + 1);

      let file = await fs.open(path, 0o0);
      this.returnMsg += `fd:${file.fd}\n`;

      let buf = new ArrayBuffer(bufferSize);
      this.returnMsg += `audioRenderer  write start.......... \n`;
      for (let i = 0;i < len; i++) {
        if (this.audioRenderer.state > audio.AudioState.STATE_RUNNING) {
          return
        }
        let options = {
          offset: i * bufferSize,
          length: bufferSize
        }
        let readsize = await fs.read(file.fd, buf, options);
        let writeSize = await this.audioRenderer.write(buf);
      }
      this.returnMsg += `audioRenderer  write end. \n`;
    } catch (err) {
      this.returnMsg += `audioRenderer  write : Error: ${JSON.stringify(err)}\n`
    }

    await this.audioRenderer.release()
  }


  createAudioRenderer() {
    if(this.selectedVolumeTypeKey=="请选择流类型"){
      this.returnMsg += `audioRenderer   请选择流类型\n`
      return
    }
    if (this.audioRenderer !== null) {
      this.returnMsg = `audioRenderer  Created already,don't create anymore`
      return
    }
    let _this = this
    audio.createAudioRenderer(this.audioRendererOption, async (err, data) => {
      console.error("this.audioRendererOptions----------:"+JSON.stringify(this.audioRendererOption))
      if (err) {
        _this.returnMsg = `audioRenderer  Created : Error: ${JSON.stringify(err)}`
      } else {
        _this.audioRenderer = data;
        _this.returnMsg += `audioRenderer  Created : SUCCESS,state:${_this.audioRenderer.state}\n`
      }
    })
  }

  rendererStart() {
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to start\n`
      return
    }
    let _this = this
    this.audioRenderer.start((err) => {
      if (err) {
        _this.returnMsg = `audioRenderer  start : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `audioRenderer  start : SUCCESS,state:${_this.audioRenderer.state}\n`
      }
    });
  }

  getBufferSize() {
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to getBufferSize`
      return
    }
    let _this = this
    _this.audioRenderer.getBufferSize((err, bufferSize) => {
      if (err) {
        _this.returnMsg = `audioRenderer  getBufferSize : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `audioRenderer  getBufferSize : SUCCESS,bufferSize:${bufferSize},state:${_this.audioRenderer.state}\n`
        _this.bufferSize = bufferSize
      }
    });
  }

  async writeRenderer() {
    if(this.writeArr==1){
      this.returnMsg += `audioRenderer  write already clicked`
      return
    }
    if (this.audioRenderer == null) {
      this.returnMsg += `audioRenderer  instance had not created,dont‘t allow to read\n`
      return
    }
    if (this.bufferSize == 0) {
      this.bufferSize = await this.audioRenderer.getBufferSize()
    }


    this.writeArr=1

    let _this = this
    let path = globalThis.pathDir + '/test_44100_2.wav';
    try {
      let stat = await fs.stat(path);
      let len = stat.size % this.bufferSize == 0 ? Math.floor(stat.size / this.bufferSize) : Math.floor(stat.size / this.bufferSize + 1);
      let file = await fs.open(path, 0o0);
      let buf = new ArrayBuffer(this.bufferSize);
      this.returnMsg = `audioRenderer  write start.......... \n`;
      while(true){
        for (let i = 0;i < len; i++) {
          let options = {
            offset: i * this.bufferSize,
            length: this.bufferSize
          }
          let readsize = await fs.read(file.fd, buf, options);
          await this.audioRenderer.write(buf)

        }
      }
      _this.returnMsg += `audioRenderer  write end, state:${_this.audioRenderer.state}\n`
    } catch (err) {
      this.returnMsg += `audioRenderer  write : Error: ${JSON.stringify(err)}\n`
    }
  }

  stopRenderer() {
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to stop\n`
      return
    }
    let _this = this
    _this.audioRenderer.stop((err) => {
      if (err) {
        _this.returnMsg = `audioRenderer  stop : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `audioRenderer  stop : SUCCESS,state:${_this.audioRenderer.state}\n`
      }
    })
  }

  pauseRenderer() {
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to pause\n`
      return
    }
    let _this = this
    _this.audioRenderer.pause((err) => {
      if (err) {
        _this.returnMsg = `audioRenderer  pause : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `audioRenderer  pause : SUCCESS,state:${_this.audioRenderer.state}\n`
      }
    })
  }

  releaseRenderer() {
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to release\n`
      return
    }
    let _this = this
    _this.audioRenderer.release((err) => {
      if (err) {
        _this.returnMsg = `audioRenderer  release : Error: ${JSON.stringify(err)}\n`
      } else {
        _this.returnMsg = `audioRenderer  release SUCCESS,state:${_this.audioRenderer.state}\n`
        _this.audioRenderer = null
        _this.writeArr=0
      }
    })
  }

  setInterruptModeCallback(){
    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer  instance had not created,dont‘t allow to setInterruptModeCallback\n`
      return
    }
    let mode = this.mode;
    let _this = this
    this.audioRenderer.setInterruptMode(mode, (err, data)=>{
      if(err){
        _this.returnMsg = `audioRenderer  setInterruptMode callback : Error: ${JSON.stringify(err)}\n`
      }
      _this.returnMsg = `audioRenderer  setInterruptMode callback : Success\n`
    });
  }

  onRendererInterrupt(){

    if (this.audioRenderer == null) {
      this.returnMsg = `audioRenderer instance had not created,dont‘t allow to onInterrupt \n`
      return
    }
    let _this = this
    _this.onRendererReturnMsg=`已监听`
    this.audioRenderer.on('audioInterrupt', async(interruptEvent) => {
      //console.log('interruptEvent:'+JSON.stringify(interruptEvent))
      _this.onRendererReturnMsg=JSON.stringify(interruptEvent)
      if(interruptEvent.hintType==2||interruptEvent.hintType==3){
        await _this.audioRenderer.pause()
      }else if(interruptEvent.hintType==1){
        await _this.audioRenderer.start()
      }
    })
  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("【音频录制-录制】返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(120).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Row() {
            Select(this.sourceTypeList).value(this.selectedSourceTypeKey)
              .onSelect((index, value) => {
                this.selectedSourceTypeKey = value
                this.audioCapturerOption.capturerInfo.source = audio.SourceType[value]
              })
          }.margin({ top: 10 })

          Row() {
            Button() {
              Text("capturer::on('audioInterrupt')").fontSize(22).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => {
              this.onCapturerAudioInterrupt()
            })
          }.margin({ top: 10 }).width('100%')

          Text(this.onCapturerReturnMsg).fontSize(22).lineHeight(30)

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

          Row() {
            Button() {
              Text("播放录音数据").fontSize(22).fontColor(Color.Blue)
            }.width('100%').height(60).backgroundColor(Color.Pink)
            .onClick(() => this.renderPlay())

          }.margin({ top: 10, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Divider().strokeWidth(10).color(Color.Blue)
          Row() {
          Select(this.volumeTypeList).value(this.selectedVolumeTypeKey)
            .onSelect((index, value) => {
              this.selectedVolumeTypeKey = value
              this.audioRendererOption.rendererInfo = this.renderInfo[this.selectedVolumeTypeKey].main
              this.selectedStreamUsageKey = this.renderInfo[this.selectedVolumeTypeKey].info.selectedStreamUsageKey
              this.selectedContentTypeKey = this.renderInfo[this.selectedVolumeTypeKey].info.selectedContentTypeKey
            })
        }.margin({ top: 20 })

        if (this.selectedVolumeTypeKey !== "请选择流类型") {
          Text(this.selectedContentTypeKey).fontSize(16).lineHeight(40)
          Text(this.selectedStreamUsageKey).fontSize(16).lineHeight(40)
        }
        Divider().strokeWidth(2).color(Color.Blue)

        Row() {
          Text("焦点模式").fontSize(24)
          Radio({ value: `mute1`, group: `modeGroup` }).onChange((isChecked) => {
            if (isChecked) {
              this.mode = 0;
            } else {
              this.mode = 1;
            }
          }).checked(this.mode == 0)
          Text("共享焦点").fontSize(18)

          Radio({ value: `mute2`, group: `modeGroup` }).onChange((isChecked) => {
            if (isChecked) {
              this.mode = 1;
            } else {
              this.mode = 0;
            }
          }).checked(this.mode == 1)
          Text("独立焦点").fontSize(18)
        }

        Text(`焦点数据：${this.mode}`).fontSize(24)
        Row() {
          Button() {
            Text("setInterruptMode callback").fontSize(22).fontColor(Color.White)
          }.width('100%').height(60).onClick(() => {
            this.setInterruptModeCallback()
          })

        }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        Row() {
          Button() {
            Text("render::on('audioInterrupt')").fontSize(22).fontColor(Color.White)
          }.width('100%').height(60).onClick(() => {
            this.onRendererInterrupt()
          })
        }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        Text(this.onRendererReturnMsg).fontSize(22)

        Row() {
          Button() {
            Text("createAudioRenderer").fontSize(22).fontColor(Color.Blue)
          }.width('80%').height(60).backgroundColor(Color.Pink).onClick(() => {
            this.createAudioRenderer()
          })

          Button() {
            Text("start").fontSize(22).fontColor(Color.Blue)
          }.width('20%').height(60).backgroundColor(Color.Pink).onClick(() => {
            this.rendererStart()
          })
        }.margin({ top: 10 }).width('100%')

        Row() {
          Button() {
            Text("write").fontSize(22).fontColor(Color.Blue)
          }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => {
            this.writeRenderer()
          })

          Button() {
            Text("pause").fontSize(22).fontColor(Color.Blue)
          }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => {
            this.pauseRenderer()
          })
        }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        Row() {
          Button() {
            Text("stop").fontSize(22).fontColor(Color.Blue)
          }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => this.stopRenderer())

          Button() {
            Text("release").fontSize(22).fontColor(Color.Blue)
          }.width('49%').height(60).backgroundColor(Color.Pink).onClick(() => this.releaseRenderer())
        }.margin({ top: 10, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        Divider().strokeWidth(6).color(Color.Blue).margin({bottom:10})
        }
      }.margin({ top: 130 }).width('100%')

    }
  }
}