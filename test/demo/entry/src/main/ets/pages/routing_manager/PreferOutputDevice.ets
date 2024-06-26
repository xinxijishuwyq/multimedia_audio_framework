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

class Options {
  offset: number = 0
  length: number = 0
}

@Entry
@Component
struct PreferredOutputDevice {
  @State returnMsg: string = 'Hello World';
  @State onReturnMsg: string = 'on 未监听';
  @State offReturnMsg: string = 'off 未监听';
  private audioRoutingManager: audio.AudioRoutingManager | null = null;
  private contentTypeList: EnumValue[] = [];
  @State selectedContentTypeKey: string = "CONTENT_TYPE_SPEECH";
  private streamUsageList: EnumValue[] = [];
  @State selectedStreamUsageKey: string = "STREAM_USAGE_VOICE_COMMUNICATION";
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
  private outputDeviceList: audio.AudioDeviceDescriptor[] = [];
  private selectOutputDeviceList: EnumValue[] = [{ value: "无数据" }];
  @State selectedOutputIndex: number = 0;
  @State outputDeviceListTag: number = 0;
  private audioRenderer: audio.AudioRenderer | null = null;

  aboutToAppear() {

    let audioManager = audio.getAudioManager();
    this.audioRoutingManager = audioManager.getRoutingManager();
    Object.keys(audio.ContentType).forEach((key) => {
      this.contentTypeList.push({ value: key });
    })
    Object.keys(audio.StreamUsage).forEach((key) => {
      this.streamUsageList.push({ value: key });
    })
    this.getSelectOutputDeviceList()
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

  async getSelectOutputDeviceList() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    let deviceFlag = audio.DeviceFlag.OUTPUT_DEVICES_FLAG;
    try {
      this.outputDeviceList = await this.audioRoutingManager.getDevices(deviceFlag);
      if (this.outputDeviceList.length > 0) {
        this.selectOutputDeviceList = this.getDeviceList(this.outputDeviceList);
        this.outputDeviceListTag = 1;
      }

      this.returnMsg = `getOutputDeviceList SUccess:${JSON.stringify(this.selectOutputDeviceList)}`;
    } catch (err) {
      this.returnMsg = `getOutputDeviceList Fail:${JSON.stringify(err)}`
    }
  }

  getPreferredOutputDeviceForRendererInfoCallback() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    this.audioRoutingManager.getPreferOutputDeviceForRendererInfo(this.audioRendererOptions.rendererInfo, (err, data) => {
      if (err) {
        this.returnMsg = `getPreferOutputDeviceForRendererInfo callback Fail:${JSON.stringify(err)}`;
      } else {
        this.returnMsg = `getPreferOutputDeviceForRendererInfo callback Success:${JSON.stringify(data)} \n`;
        this.returnMsg += `优先设备为：${JSON.stringify(this.getDeviceList(data))}`;
      }
    })
  }

  getPreferredOutputDeviceForRendererInfoPromise() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    this.audioRoutingManager.getPreferOutputDeviceForRendererInfo(this.audioRendererOptions.rendererInfo)
      .then(data => {
        this.returnMsg = `getPreferOutputDeviceForRendererInfo promise Success:${JSON.stringify(data)} \n`;
        this.returnMsg += `优先设备为：${JSON.stringify(this.getDeviceList(data))}`;
      }).catch((err: BusinessError) => {
      this.returnMsg = `getPreferOutputDeviceForRendererInfo promise Fail:${JSON.stringify(err)}`;
    })
  }

  onPreferredOutputDeviceChangeForRendererInfo() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    this.onReturnMsg = "on 监听中...";
    this.audioRoutingManager.on('preferOutputDeviceChangeForRendererInfo', this.audioRendererOptions.rendererInfo, (audioDeviceDescriptors) => {
      this.onReturnMsg = `on 收到回调：\n`;
      this.onReturnMsg = `${JSON.stringify(audioDeviceDescriptors)}`;
    })
  }

  offPreferredOutputDeviceChangeForRendererInfo() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    this.offReturnMsg = "off 监听中...";
    this.audioRoutingManager.off('preferOutputDeviceChangeForRendererInfo', (audioDeviceDescriptors) => {
      this.offReturnMsg = `off 收到回调：\n`;
      this.offReturnMsg = `${JSON.stringify(audioDeviceDescriptors)}`;
    })
  }

  selectOutputDevice() {
    if (!this.audioRoutingManager) {
      this.returnMsg = "AudioRoutingManager instance had not created";
      return;
    }
    let _this = this;
    if (this.outputDeviceList.length == 0) {
      _this.returnMsg = `没有输出设备类`;
      return;
    }
    let outputAudioDeviceDescriptor = this.outputDeviceList.filter((value, index) => {
      return index == _this.selectedOutputIndex;
    })
    if (outputAudioDeviceDescriptor.length == 0) {
      _this.returnMsg = `尚未选择输出设备`;
      return;
    }
    this.audioRoutingManager.selectOutputDevice(outputAudioDeviceDescriptor)
      .then(() => {
        _this.returnMsg = `selectOutputDevice promise Success,所选的输出设备为: \n`;
        _this.returnMsg += `${JSON.stringify(this.getDeviceList(outputAudioDeviceDescriptor))} \n`;
      }).catch((err: BusinessError) => {
      _this.returnMsg = `selectOutputDevice promise Failed err:${JSON.stringify(err)}`;
    })
  }

  getDeviceList(deviceDescriptors: audio.AudioDeviceDescriptor[]) {
    let deviceList: EnumValue[] = [];
    for (let i = 0;i < deviceDescriptors.length; i++) {
      let deviceTypeName = this.getDeviceTypeNameByValue(deviceDescriptors[i].deviceType)
      deviceList.push({ value: deviceTypeName })
    }
    return deviceList;
  }

  getDeviceTypeNameByValue(value: audio.DeviceType) {
    let keyValue: string = '';
    Object.keys(audio.AudioVolumeType).forEach((key) => {
      if (audio.DeviceType[key] == value) {
        keyValue = key;
      }
    })
    return keyValue;
  }

  async renderPlay() {
    if (this.audioRenderer !== null) {
      await this.audioRenderer.release()
      this.audioRenderer = null
    }
    try {
      this.audioRenderer = await audio.createAudioRenderer(this.audioRendererOptions)
      this.returnMsg += `audioRenderer create success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer create : Error: ${JSON.stringify(err)}\n`
      return
    }
    let bufferSize: number;
    try {
      bufferSize = await this.audioRenderer.getBufferSize()
      this.returnMsg += `audioRenderer getBufferSize success,bufferSize:${bufferSize} \n`
      await this.audioRenderer.start()
      this.returnMsg += `audioRenderer start success \n`
    } catch (err) {
      this.returnMsg += `audioRenderer start : Error: ${JSON.stringify(err)}\n`
      return
    }

    let path = globalThis.pathDir + '/test_44100_2.wav';
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
        if (this.audioRenderer.state > audio.AudioState.STATE_RUNNING) {
          return
        }
        let options: Options = {
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

  build() {
    Column() {
      Row() {
        Column() {
          Text("【路由管理-首选输出设备】返回数据：").fontWeight(FontWeight.Bolder).position({ x: 10, y: 5 }).fontSize(18)
          Text(this.returnMsg).position({ x: 10, y: 30 }).fontSize(14)
        }.width('98%').height(120).backgroundColor(Color.Orange).position({ x: '1%' })
      }.position({ x: 0, y: 0 }).width('100%').zIndex(999)

      Scroll() {
        Column() {

          Divider().strokeWidth(6).color(Color.Blue)

          Row() {
            Select(this.contentTypeList).value(this.selectedContentTypeKey)
              .onSelect((index, value) => {
                this.selectedContentTypeKey = value
                this.audioRendererOptions.rendererInfo.content = audio.ContentType[value]
              })
          }.margin({ top: 10 })

          Row() {
            Select(this.streamUsageList).value(this.selectedStreamUsageKey)
              .onSelect((index, value) => {
                this.selectedStreamUsageKey = value
                this.audioRendererOptions.rendererInfo.usage = audio.StreamUsage[value]
              })
          }.margin({ top: 10 })

          Divider().strokeWidth(6).color(Color.Blue)

          Row() {
            Button() {
              Text("getPreferredOutputDeviceForRendererInfo callback")
                .fontSize(18)
                .fontColor(Color.White)
                .align(Alignment.Center)
                .lineHeight(30)
            }.width('100%').height(60)
            .onClick(() => this.getPreferredOutputDeviceForRendererInfoCallback())

          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("getPreferredOutputDeviceForRendererInfo promise")
                .fontSize(18)
                .fontColor(Color.White)
                .align(Alignment.Center)
                .lineHeight(30)
            }.width('100%').height(60)
            .onClick(() => this.getPreferredOutputDeviceForRendererInfoPromise())

          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Row() {
            Button() {
              Text("on('preferredOutputDeviceChangeForRendererInfo')")
                .fontSize(16)
                .fontColor(Color.White)
                .align(Alignment.Center)
                .lineHeight(30)
            }.width('100%').height(60).onClick(() => this.onPreferredOutputDeviceChangeForRendererInfo())
          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Text(this.onReturnMsg).fontSize(20).lineHeight(30)

          Row() {
            Button() {
              Text("off").fontSize(16).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => this.offPreferredOutputDeviceChangeForRendererInfo())

          }.margin({ top: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

          Text(this.offReturnMsg).fontSize(20).lineHeight(30)

          if (this.outputDeviceListTag == 1) {
            Row() {
              Text("输出设备").fontSize(20).margin({ left: 10 })
              Select(this.selectOutputDeviceList).value(this.selectOutputDeviceList[this.selectedOutputIndex].value)
                .onSelect((index, value) => {
                  this.selectedOutputIndex = index;
                })
            }.margin({ top: 10 }).width('100%')
          }
          Row() {
            Button() {
              Text("selectOutputDevice").fontSize(20).fontColor(Color.White)
            }.width('100%').height(60).onClick(() => this.selectOutputDevice())
          }.margin({ top: 10, bottom: 20 }).width('100%')

          Row() {
            Button() {
              Text("播放音频数据").fontSize(22).fontColor(Color.Blue)
            }.width('100%').height(60).onClick(() => this.renderPlay()).backgroundColor(Color.Pink)

          }.margin({ top: 10, bottom: 10 }).width('100%').justifyContent(FlexAlign.SpaceBetween)

        }.width('100%')
      }.margin({ top: 130 }).width('100%')

    }
  }
}