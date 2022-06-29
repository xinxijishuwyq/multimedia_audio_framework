/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
import fileio from '@ohos.fileio';
import featureAbility from '@ohos.ability.featureAbility'
import prompt from '@system.prompt';
import abilityAccessCtrl from '@ohos.abilityAccessCtrl';
import bundle from '@ohos.bundle';


export default {

    data : {
        audioManager:null
    },
    sleep (ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    },

	async getPermission() {
		let appInfo = await bundle.getApplicationInfo('ohos.acts.multimedia.audio.audiomanager', 0, 100);
        let tokenID = appInfo.accessTokenId;
        let atManager = abilityAccessCtrl.createAtManager();
        let result1 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.MEDIA_LOCATION",1);
        let result2 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.READ_MEDIA",1);
        let result3 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.WRITE_MEDIA",1);
        let result4 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.GRANT_SENSITIVE_PERMISSIONS",1);
        let result5 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.REVOKE_SENSITIVE_PERMISSIONS",1);
        let result6 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.MICROPHONE",1);
        let result7 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.ACCESS_NOTIFICATION_POLICY",1);
        let result8 = await atManager.grantUserGrantedPermission(tokenID, "ohos.permission.MODIFY_AUDIO_SETTINGS",1);
        let isGranted1 = await atManager.verifyAccessToken(tokenID, "ohos.permission.MEDIA_LOCATION");
        let isGranted2 = await atManager.verifyAccessToken(tokenID, "ohos.permission.READ_MEDIA");
        let isGranted3 = await atManager.verifyAccessToken(tokenID, "ohos.permission.WRITE_MEDIA");
        let isGranted4 = await atManager.verifyAccessToken(tokenID, "ohos.permission.GRANT_SENSITIVE_PERMISSIONS");
        let isGranted5 = await atManager.verifyAccessToken(tokenID, "ohos.permission.REVOKE_SENSITIVE_PERMISSIONS");
        let isGranted6 = await atManager.verifyAccessToken(tokenID, "ohos.permission.MICROPHONE");
        let isGranted7 = await atManager.verifyAccessToken(tokenID, "ohos.permission.ACCESS_NOTIFICATION_POLICY");
        let isGranted8 = await atManager.verifyAccessToken(tokenID, "ohos.permission.MODIFY_AUDIO_SETTINGS");
		console.info("AudioManagerLog: Perm1:"+result1);
        console.info("AudioManagerLog: Perm2:"+result2);
        console.info("AudioManagerLog: Perm3:"+result3);
        console.info("AudioManagerLog: Perm1G:"+isGranted1);
        console.info("AudioManagerLog: Perm2G:"+isGranted2);
        console.info("AudioManagerLog: Perm3G:"+isGranted3);
		console.info("AudioManagerLog: Perm4:"+result4);
        console.info("AudioManagerLog: Perm5:"+result5);
        console.info("AudioManagerLog: Perm6:"+result6);
        console.info("AudioManagerLog: Perm4G:"+isGranted4);
        console.info("AudioManagerLog: Perm5G:"+isGranted5);
        console.info("AudioManagerLog: Perm6G:"+isGranted6);
		console.info("AudioManagerLog: Perm7:"+result7);
        console.info("AudioManagerLog: Perm8:"+result8);
        console.info("AudioManagerLog: Perm7G:"+isGranted7);
        console.info("AudioManagerLog: Perm8G:"+isGranted8);
	},
	
    async onRendererChangeWithPlay() {
        var AudioStreamInfo = {
            samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_44100,
            channels: audio.AudioChannel.CHANNEL_1,
            sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
            encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
        }

        var AudioRendererInfo = {
            content: audio.ContentType.CONTENT_TYPE_SPEECH,
            usage: audio.StreamUsage.STREAM_USAGE_VOICE_COMMUNICATION,
            rendererFlags: 1
        }

        var AudioRendererOptions = {
            streamInfo: AudioStreamInfo,
            rendererInfo: AudioRendererInfo
        }
        var audioStreamManager;
        await this.audioManager.getStreamManager().then(async function (data) {
            audioStreamManager = data;
            console.info('AudioFrameworkChange: Get AudioStream Manager : Success ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: Get AudioStream Manager : ERROR : '+err.message);
        });

        audioStreamManager.on('audioRendererChange',(AudioFrameworkChangeInfoArray) =>{
            console.info('AudioFrameworkChange: [RENDERER-CHANGE-ON-PLAY] ######### Entered On Listener Loop ##########');
            var varStr;
            for (let i=0;i<AudioFrameworkChangeInfoArray.length;i++) {
                prompt.showToast({
					message:["Ren On StreamId : " + AudioFrameworkChangeInfoArray[i].streamId + " ; ClientUid : " + AudioFrameworkChangeInfoArray[i].clientUid + " ; Content : " + AudioFrameworkChangeInfoArray[i].rendererInfo.content + " ; Usage : " + AudioFrameworkChangeInfoArray[i].rendererInfo.usage + " ; Renderer Flag : " + AudioFrameworkChangeInfoArray[i].rendererInfo.rendererFlags + " ; StateChange : " +AudioFrameworkChangeInfoArray[i].rendererState],
                    duration: 2000
                });
            }
        });

        await this.sleep (500);
        var resultFlag;
        var audioRen;
        await audio.createAudioRenderer(AudioRendererOptions).then(async function (data) {
            audioRen = data;
            console.info('AudioFrameworkChange: AudioRender Created : Success : Stream Type: SUCCESS');
        }).catch((err) => {
            console.info('AudioFrameworkChange: AudioRender Created : ERROR : '+err.message);
            return resultFlag;
        });

        await this.sleep(500);
        console.info('AudioFrameworkChange: AudioRenderer : STATE : '+audioRen.state);

        await audioRen.start().then(async function () {
            console.info('AudioFrameworkChange: renderInstant started :SUCCESS ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: renderInstant start :ERROR : '+err.message);
            resultFlag=false;
        });
        
        if (resultFlag == false){
            console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
            return resultFlag;
        }

        await this.sleep(500);

        console.info('AudioFrameworkChange: AudioRenderer : STATE : '+audioRen.state);

        var bufferSize;
        await audioRen.getBufferSize().then(async function (data) {
            console.info('AudioFrameworkChange: getBufferSize :SUCCESS '+data);
            bufferSize=data;
        }).catch((err) => {
            console.info('AudioFrameworkChange: getBufferSize :ERROR : '+err.message);
            resultFlag=false;
        });
        if (resultFlag == false){
            console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
            return resultFlag;
        }

        console.info("AudioFrameworkChange: getCacheDir before Start $$$$$");
        var context = featureAbility.getContext();
        var path1 = await context.getCacheDir();
        var fpath2 = path1 + "/PinkPanther60-44100-1c.wav";
        console.info("AudioFrameworkChange: getCacheDir after Start $$$$$");
        console.info('AudioFrameworkChange: Path $$$$$: ' + fpath2);

        let ss = fileio.createStreamSync(fpath2, 'r');
        console.info('AudioFrameworkChange:case2: File Path: ' + ss);
        let discardHeader = new ArrayBuffer(44);
        ss.readSync(discardHeader);
        let totalSize = fileio.statSync(fpath2).size;
        console.info('AudioFrameworkChange: File totalSize size: ' +totalSize);
        totalSize = totalSize-44;
        console.info('AudioFrameworkChange: File size : Removing header: ' +totalSize);
        let rlen = 0;
        while (rlen < totalSize/2) {
            let buf = new ArrayBuffer(bufferSize);
            rlen += ss.readSync(buf);
            await audioRen.write(buf);
        }
        console.info('AudioFrameworkChange: Renderer after read');

        await this.sleep(500);
        await audioRen.drain().then(async function () {
            console.info('AudioFrameworkChange: Renderer drained : SUCCESS');
        }).catch((err) => {
            console.error('AudioFrameworkChange: Renderer drain: ERROR : '+err.message);
            resultFlag=false;
        });
        if (resultFlag == false){
            console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
            return resultFlag;
        }

        await this.sleep(100);
        console.info('AudioFrameworkChange: AudioRenderer : STATE : '+audioRen.state);

        await audioRen.stop().then(async function () {
            console.info('AudioFrameworkChange: Renderer stopped : SUCCESS');
            resultFlag=true;
            console.info('AudioFrameworkChange: resultFlagRen : '+resultFlag);
        }).catch((err) => {
            console.info('AudioFrameworkChange: Renderer stop:ERROR : '+err.message);
            resultFlag=false;
        });

        await this.sleep(500);
        console.info('AudioFrameworkChange: AudioRenderer : STATE : '+audioRen.state);

        await audioRen.release().then(async function () {
            console.info('AudioFrameworkChange: Renderer release : SUCCESS');
        }).catch((err) => {
            console.info('AudioFrameworkChange: Renderer release :ERROR : '+err.message);
            resultFlag=false;
        });

        await this.sleep(500);

        console.info('AudioFrameworkChange: AudioRenderer : STATE : '+audioRen.state);
        console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
    },
    async offRendererChange() {
        var audioStreamManager;
        await this.audioManager.getStreamManager().then(async function (data) {
            audioStreamManager = data;
            console.info('AudioFrameworkChange: Get AudioStream Manager : Success ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: Get AudioStream Manager : ERROR : '+err.message);
        });
        audioStreamManager.off('audioRendererChange');
        console.info('AudioFrameworkChange: [RENDERER-CHANGE-OFF] ######### RendererChange Off is called #########');
        prompt.showToast({
            message:"Renderer State Listener Turned Off",
            duration: 1000
        });
    },
    async getRendererChangePromise() {
        var audioStreamManager;
        this.audioManager.getStreamManager((err, data) => {
            if (err) {
                console.error('AudioFrameworkChange: Get AudioStream Manager : ERROR : '+err.message);
            }
            else {
                audioStreamManager = data;
                console.info('AudioFrameworkChange: Get AudioStream Manager : Success ');
            }
        });
        await this.sleep(1000);
        await audioStreamManager.getCurrentAudioRendererInfoArray().then(async function (AudioFrameworkChangeInfoArray) {
            console.info('AudioFrameworkChange: [GET-RENDERER-CHANGE-INFO-PROMISE] ######### Get Current Renderer Info Promise is called #########');
            for (let i=0;i<AudioFrameworkChangeInfoArray.length;i++) {
                prompt.showToast({
					message:["Ren Get StreamId : " + AudioFrameworkChangeInfoArray[i].streamId + " ; ClientUid : " + AudioFrameworkChangeInfoArray[i].clientUid + " ; Content : " + AudioFrameworkChangeInfoArray[i].rendererInfo.content + " ; Usage : " + AudioFrameworkChangeInfoArray[i].rendererInfo.usage + " ; Renderer Flag : " + AudioFrameworkChangeInfoArray[i].rendererInfo.rendererFlags + " ; StateChange : " +AudioFrameworkChangeInfoArray[i].rendererState],
                    duration: 2000
                });
            }
        }).catch((err) => {
            console.log('AudioFrameworkChange: getCurrentAudioRendererInfoArray :ERROR: '+err.message);
        });
    },

	async onCapturerChangeRecord() {
		var audioStreamManager;
        var AudioStreamInfo = {
            samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_8000,
            channels: audio.AudioChannel.CHANNEL_1,
            sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_U8,
            encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW,
        };
        var AudioCapturerInfo = {
            source: audio.SourceType.SOURCE_TYPE_MIC,
            capturerFlags : 1,
        }
        var AudioCapturerOptions = {
            streamInfo: AudioStreamInfo,
            capturerInfo: AudioCapturerInfo,
        }

        var fpath = '/data/storage/el2/base/haps/entry/cache/capture_js-8000-1C-8B.pcm';
		
        var resultFlag;
		var audioCap;
        console.info('AudioFrameworkChange: Promise : Audio Recording Function');
		await this.audioManager.getStreamManager().then(async function (data) {
            audioStreamManager = data;
            console.info('AudioFrameworkChange: getStreamManager : Success ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: getStreamManager : ERROR : '+err.message);
        });

        audioStreamManager.on('audioCapturerChange',async function (AudioCapturerChangeInfoArray) {
			console.info('AudioFrameworkChange: [CAPTURER-CHANGE-ON-RECORD] ######### Entered On Listener Loop ########## ');
            for (let i=0;i<AudioCapturerChangeInfoArray.length;i++) {
				prompt.showToast({
                    message:["Cap On StreamId : " + AudioCapturerChangeInfoArray[i].streamId+ " ; ClientUid : " + AudioCapturerChangeInfoArray[i].clientUid + " ; Source : " + AudioCapturerChangeInfoArray[i].capturerInfo.source + " ; Capturer Flag : " + AudioCapturerChangeInfoArray[i].capturerInfo.capturerFlags + " ; StateChange : " + AudioCapturerChangeInfoArray[i].capturerState],
                    duration: 2000
                });
            }
        });
        await this.sleep (500);

        await audio.createAudioCapturer(AudioCapturerOptions).then(async function (data) {
            audioCap = data;
            console.info('AudioFrameworkChange: AudioCapturer Created : Success : Stream Type: SUCCESS');
        }).catch((err) => {
            console.info('AudioFrameworkChange: AudioCapturer Created : ERROR : '+err.message);
            return resultFlag;
        });

		await this.sleep (500);
        console.info('AudioFrameworkChange: AudioCapturer : Path : '+fpath);

        console.info('AudioFrameworkChange: AudioCapturer : STATE : '+audioCap.state);

        await audioCap.start().then(async function () {
            console.info('AudioFrameworkChange: Capturer started :SUCCESS ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: Capturer start :ERROR : '+err.message);
            resultFlag=false;
        });
        if (resultFlag == false){
            console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
            return resultFlag;
        }

		await this.sleep (500);
        console.info('AudioFrameworkChange: AudioCapturer : STATE : '+audioCap.state);

        var bufferSize = await audioCap.getBufferSize();
        console.info('AudioFrameworkChange: buffer size: ' + bufferSize);

        var fd = fileio.openSync(fpath, 0o102, 0o777);
        if (fd !== null) {
            console.info('AudioFrameworkChange: file fd created');
        }
        else{
            console.info('AudioFrameworkChange: Capturer start :ERROR : ');
            resultFlag=false;
            return resultFlag;
        }

        fd = fileio.openSync(fpath, 0o2002, 0o666);
        if (fd !== null) {
            console.info('AudioFrameworkChange: file fd opened : Append Mode :PASS');
        }
        else{
            console.info('AudioFrameworkChange: file fd Open: Append Mode : FAILED');
            resultFlag=false;
            return resultFlag;
        }
        await this.sleep(100);
        var numBuffersToCapture = 50;
        while (numBuffersToCapture) {
            var buffer = await audioCap.read(bufferSize, true);
            await this.sleep(50);
            var number = fileio.writeSync(fd, buffer);
            await this.sleep(50);
            numBuffersToCapture--;
        }
        await this.sleep(1000);
        console.info('AudioFrameworkChange: AudioCapturer : STATE : '+audioCap.state);

        await audioCap.stop().then(async function () {
            console.info('AudioFrameworkChange: Capturer stopped : SUCCESS');
            resultFlag=true;
            console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
        }).catch((err) => {
            console.info('AudioFrameworkChange: Capturer stop:ERROR : '+err.message);
            resultFlag=false;
        });

		await this.sleep (500);
        console.info('AudioFrameworkChange: AudioCapturer : STATE : '+audioCap.state);

        await audioCap.release().then(async function () {
            console.info('AudioFrameworkChange: Capturer release : SUCCESS');
        }).catch((err) => {
            console.info('AudioFrameworkChange: Capturer release :ERROR : '+err.message);
            resultFlag=false;
        });

		await this.sleep (500);
        console.info('AudioFrameworkChange: AudioCapturer : STATE : '+audioCap.state);
        console.info('AudioFrameworkChange: resultFlag : '+resultFlag);
    },
    async offCapturerChange() {
		var audioStreamManager;
		await this.audioManager.getStreamManager().then(async function (data) {
            audioStreamManager = data;
            console.info('AudioFrameworkChange: getStreamManager : Success ');
        }).catch((err) => {
            console.info('AudioFrameworkChange: getStreamManager : ERROR : '+err.message);
        });
        audioStreamManager.off('audioCapturerChange');
        console.info('AudioFrameworkChange: [CAPTURER-CHANGE-OFF] ######### CapturerChange off is called for ##########');
		prompt.showToast({
            message:"Capturer State Listener Turned Off",
            duration: 1000
        });
    },
	async getCapturerChangePromise() {
		var audioStreamManager;
		this.audioManager.getStreamManager((err, data) => {
            if (err) {
                console.error('AudioCapturerChange: Get AudioStream Manager : ERROR : '+err.message);
            }
            else {
                audioStreamManager = data;
                console.info('AudioCapturerChange: Get AudioStream Manager : Success ');
            }
        });
        await this.sleep(1000);
        audioStreamManager.getCurrentAudioCapturerInfoArray().then(async function (AudioCapturerChangeInfoArray) {
			console.info('AudioFrameworkChange: [GET-CAPTURER-CHANGE-PROMISE] ######### Get CapturerChange Promise is called for ##########');
            for (let i=0;i<AudioCapturerChangeInfoArray.length;i++) {
				prompt.showToast({
                    message:["Cap Get StreamId : " + AudioCapturerChangeInfoArray[i].streamId+ " ; ClientUid : " + AudioCapturerChangeInfoArray[i].clientUid + " ; Source : " + AudioCapturerChangeInfoArray[i].capturerInfo.source + " ; Capturer Flag : " + AudioCapturerChangeInfoArray[i].capturerInfo.capturerFlags + " ; StateChange : " + AudioCapturerChangeInfoArray[i].capturerState],
                    duration: 2000
                });
			}
        }).catch((err) => {
            console.log('AudioFrameworkChange: getCurrentAudioCapturerInfoArray :ERROR: '+err.message);
        });   
    },
	
    onInit() {
        this.title = this.$t('strings.world');
        this.audioManager = audio.getAudioManager();
    }
}