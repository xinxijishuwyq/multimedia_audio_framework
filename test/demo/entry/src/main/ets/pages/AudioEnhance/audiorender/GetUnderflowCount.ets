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
struct GetUnderflowCount {
  private audioRendererOptions = {
    streamInfo: {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_44100,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    },
    rendererInfo: {
      content: audio.ContentType.CONTENT_TYPE_MUSIC,
      usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
      rendererFlags: 0
    }
  }
  private audioRenderer = null
  @State returnMsg: string = ``
  @State path: string = globalThis.pathDir + '/test_44100_2.wav';

  async onBackPress() {
    if (this.audioRenderer == null) {
      this.returnMsg += `audioRenderer instance had not created\n`
      return
    }
    await this.audioRenderer.release()
    this.audioRenderer = null
  }

  async sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
 }

  async renderPlay() {
    try {
      this.audioRenderer = await audio.createAudioRenderer(this.audioRendererOptions)
      this.returnMsg += `audioRenderer create success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer create : Error: ${JSON.stringify(err)}\n`
      return
    }
    let bufferSize
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
     // while(true){
        for (let i = 0;i < len; i++) {
          let options = {
            offset: i * bufferSize,
            length: bufferSize
          }
          let readsize = await fs.read(file.fd, buf, options);
          let writeSize = await this.audioRenderer.write(buf);
          if (i == len - 5) {
            this.sleep(3000);
          }
          if (i == len - 1) {
            this.sleep(1000);
          }
      }
     // }
      this.returnMsg += `audioRenderer write end. \n`;
    } catch (err) {
      this.returnMsg += `audioRenderer write : Error: ${JSON.stringify(err)}\n`
    }
  }

  getUnderflowCountPromise() {
    if (this.audioRenderer == null) {
      this.returnMsg += `audioRenderer instance had not created,dont‘t allow to getUnderflowCountPromise\n`
      return
    }

    let _this = this
    this.audioRenderer.getUnderflowCount()
      .then(underflowCount => {
        _this.returnMsg = `audioRenderer getUnderflowCountPromise : Success, 返回值: ${underflowCount}`
      })
      .catch((err) => {
        _this.returnMsg = `audioRenderer getUnderflowCountPromise : Error: ${JSON.stringify(err)}`
      })
  }

  getUnderflowCountCallback() {
    if (this.audioRenderer == null) {
      this.returnMsg += `audioRenderer instance had not created,dont‘t allow to getUnderflowCountCallback\n`
      return
    }

    let _this = this
    this.audioRenderer.getUnderflowCount((err, underflowCount) => {
      if (err) {
        _this.returnMsg = `audioRenderer getUnderflowCountCallback  : Error: ${JSON.stringify(err)}`
      } else {
        _this.returnMsg = `audioRenderer getUnderflowCountCallback  : Success, 返回值: ${underflowCount}`
      }
    })
  }

  async createRender() {
    try {
      this.audioRenderer = await audio.createAudioRenderer(this.audioRendererOptions)
      this.returnMsg += `audioRenderer create success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer create : Error: ${JSON.stringify(err)}\n`
      return
    }
  }

  build() {
    Column() {
      Row() {
        Column() {
          Text("返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(220).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {
          Row() {
            Button() {
              Text("创建AudioRender").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).onClick(() => this.createRender()).backgroundColor(Color.Pink)

            Button() {
              Text("播放音乐").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).onClick(() => this.renderPlay()).backgroundColor(Color.Pink)

          }.margin({ top: 20, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("getUnderflowCount callback").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).onClick(() => this.getUnderflowCountCallback())

            Button() {
              Text("getUnderflowCount promise").fontSize(22).fontColor(Color.Blue)
            }.width('49%').height(60).onClick(() => this.getUnderflowCountPromise())
          }.margin({ top: 20, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        }.width('100%')
      }.margin({ top: 230 }).width('100%')
    }
  }
}