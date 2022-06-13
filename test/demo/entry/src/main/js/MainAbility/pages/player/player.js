import router from '@system.router';
import prompt from '@system.prompt';
import fileio from '@ohos.fileio';
import audio from '@ohos.multimedia.audio'


class Playlist {
    constructor() {
    }

    audioFiles = [];
}

class Song {
    constructor(name, fileUri) {
        this.name = name;
        this.fileUri = fileUri;
    }
}

export default {
    data: {
        title: '', // Current music name
        audioUrl: '', // Image url of pause or play
        volumeImageUrl: '', // Image url of zero or non-zero volume
        rateImageUrl: '', // Image url of rendering rate
        muteImageUrl: '', // Image url of mute or non mute
        index: 0, // Position of the music being played in the playlist
        isPlaying: false, // Is music playing
        isPaused: false, // Is music paused
        musicProgress: 0, // Play progress, from 0 to 100
        readTimes: 0, // The number of times the renderer reads buffer
        canAppContinuePlay: true, // Related to pause, is it allowed to continue writing data to the renderer
        isContinuePlay: false, // Continue play or play from the beginning
        playlist: null,
        audioManager: null,
        audioRenderer: null,
        // The following variables are created to display apis' information
        isMute: false,
        state: 0,
        volume: 0,
        currentProgress: 0,
        rendererContent: 0,
        rendererFlags: 0,
        rendererUsage: 0,
        steamChannels: 0,
        steamSampleFormat: 0,
        steamEncodingType: 0,
        steamSamplingRate: 0,
        audioTime: 0,
        bufferSize: 0,
        renderRate: 0,
        minVolume: 0,
        maxVolume: 0,
        isActive: true,
        ringMode: 0,
        audioParameter: '',
        devices: null,
        deviceActive: true,
        isMicrophoneMuted: false,
        currentRate: audio.AudioRendererRate.RENDER_RATE_NORMAL,
    },

    onInit() {
        this.audioUrl = 'common/images/play.png';
        this.getPlaylist(() => {
            console.log('[JSAR] Get play list finished');
        });
        this.title = this.playlist.audioFiles[this.index].name;
        console.log('[JSAR] The first music in the list is ' + this.title);
        this.audioManager = audio.getAudioManager();
    },

    async onShow() {
        await this.audioManager.getVolume(audio.AudioVolumeType.MEDIA).then((value) => {
            console.log("[JSAR] In onShow part, volume is " + value);
            this.volume = value;
            this.volumeImageUrl = 'common/images/volumezero.png';
            if (value == 0) {
                this.volumeImageUrl = 'common/images/volumezero.png';
            } else {
                this.volumeImageUrl = 'common/images/volume.png';
            }
        });
        await this.audioManager.isMute(audio.AudioVolumeType.MEDIA).then((value) => {
            console.log("[JSAR] In onShow part, isMute? " + value);
            if (value) {
                this.volumeImageUrl = 'common/images/mute.png';
            } else {
                this.volumeImageUrl = 'common/images/unmute.png';
            }
        });
        await this.audioManager.setAudioParameter('PBits per sample', '8 bit', (err) => {
            if (err) {
                console.error('[JSAR] Failed to set the audio parameter. ${err.message}');
            }
            console.log('[JSAR] Callback invoked to indicate a successful setting of the audio parameter.');
        });
        await this.audioManager.setMicrophoneMute(false, (err) => {
            if (err) {
                console.error('[JSAR] Failed to mute the microphone. ${err.message}');
            }
            console.log('[JSAR] Callback invoked to indicate that the microphone is muted.');
        });
        this.rateImageUrl = 'common/images/0.png';
        this.muteImageUrl = 'common/images/unmute.png'
        await console.log("[JSAR] Load completed!");
        await this.showInfo();
    },

    async createRender() {
        var audioStreamInfo = {
            samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_48000,
            channels: audio.AudioChannel.CHANNEL_1,
            sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
            encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
        }
        var audioRendererInfo = {
            content: audio.ContentType.CONTENT_TYPE_MUSIC,
            usage: audio.StreamUsage.STREAM_USAGE_MEDIA,
            rendererFlags: 0
        }
        var audioRendererOptions = {
            streamInfo: audioStreamInfo,
            rendererInfo: audioRendererInfo
        }
        await audio.createAudioRenderer(audioRendererOptions).then((data) => {
            this.audioRenderer = data;
            console.info('[JSAR] AudioRenderer Created : Success : Stream Type: SUCCESS');
        }).catch((err) => {
            console.info('[JSAR] AudioRenderer Created : ERROR : ' + err.message);
        });
        if (this.currentRate != audio.AudioRendererRate.RENDER_RATE_NORMAL) {
            this.audioRenderer.setRenderRate(this.currentRate).then(() => {
                console.log('[JSAR] Set renderer rate SUCCESS');
            }).catch((err) => {
                console.log('[JSAR] ERROR: ' + err.message);
            });
        }
        // Test the API .on('stateChange')
        this.audioRenderer.on('stateChange', (state) => {
            var renderStateStr = '';
            switch (state) {
                case 1:
                    renderStateStr = 'Renderer STATE_PREPARED';
                    break;
                case 2:
                    renderStateStr = 'Renderer STATE_RUNNING';
                    break;
                case 3:
                    renderStateStr = 'Renderer STATE_STOPPED';
                    break;
                case 4:
                    renderStateStr = 'Renderer STATE_RELEASED';
                    break;
                case 5:
                    renderStateStr = 'Renderer STATE_PAUSED';
                    break;
                default:
                    renderStateStr = 'Renderer STATE_NEW or INVALID';
            }
            prompt.showToast({
                message: renderStateStr,
                duration: 1000
            });
            console.log("[JSAR] Audio renderer state is: " + renderStateStr);
        });
        // Test the API .on('markReach'), set mark at 1000
        this.audioRenderer.on('markReach', 1000, (position) => {
            if (position == "1000") {
                prompt.showToast({
                    message: 'Reached mark at frame 1000',
                    duration: 1000
                });
                console.log('[JSAR] "markReach" triggered successfully');
            }
        });
    },

    getPlaylist(callback) {
        console.log('[JSAR] Get audio assets started');
        this.playlist = new Playlist();
        this.playlist.audioFiles = [];
        // This path may not available
        this.playlist.audioFiles[0] = new Song('test.pcm', '/resources/base/media/test.pcm');
        this.playlist.audioFiles[1] = new Song('test2.pcm', '/resources/base/media/test2.pcm');
        console.log('[JSAR] Get audio assets ended');
        this.title = this.playlist.audioFiles[this.index].name;
        console.log('[JSAR] Generate play list finished');
    },

    async onPlayClick() {
        console.info('[JSAR] onPlayClick, isPlaying=' + this.isPlaying);
        if (this.isPlaying) {
            await this.pausePlay();
        } else {
            console.info('[JSAR] onPlayClick, isPaused is=' + this.isPaused);
            if (this.isPaused) {
                await this.continuePlay();
            } else {
                await this.startPlay();
            }
        }
    },

    async pausePlay() {
        this.canAppContinuePlay = false;
        console.log("[JSAR] After click pausePlay, before paused, the renderer state is: " + this.audioRenderer.state);
        await this.audioRenderer.pause().then(() => {
            console.log('[JSAR] Renderer paused');
            this.isPaused = true;
        }).catch((err) => {
            console.log('[JSAR] ERROR: ' + err.message);
        });
        this.isPlaying = false;
        this.audioUrl = 'common/images/play.png';
        console.log("[JSAR] After on pause click, isPlaying=" + this.isPlaying);
        console.log("[JSAR] After click pausePlay and paused, the renderer state is=" + this.audioRenderer.state);
    },

    async startPlay() {
        await this.audioManager.mute(audio.AudioVolumeType.MEDIA, this.isMute, (err) => {
            if (err) {
                console.error(' [JSAR] Failed to mute the stream. ${err.message}');
            } else {
                console.log('[JSAR] Callback invoked to indicate that the stream is muted. to ' + this.isMute);
            }
        });
        console.log("[JSAR] this.readTimes= " + this.readTimes);
        if (!this.isContinuePlay) {
            if (!this.audioRenderer === undefined) {
                this.audioRenderer.release();
            }
            await this.createRender();
        }
        this.isContinuePlay = false;
        let bufferSize = 0;
        try {
            bufferSize = await this.audioRenderer.getBufferSize();
            await this.audioRenderer.start();
        } catch (err) {
            console.log('[JSAR] ERROR: ' + err.message);
        }
        let pathPre = '/data/storage/el2/base/haps/entry/files/';
        console.log("[JSAR] The path pre is: " + pathPre);
        let path = pathPre + this.playlist.audioFiles[this.index].name;
        console.log("[JSAR] Now the music url: " + path);
        let stat = fileio.statSync(path);
        console.log('[JSAR] Size= ' + stat.size + " File's atime= " + stat.atime + ", file mode= " + stat.mode + ", readTimes = " + this.readTimes);
        let len = (stat.size % bufferSize) === 0 ? stat.size / bufferSize : parseInt(stat.size / bufferSize + 1);
        this.musicProgress = this.readTimes / len;
        console.log('[JSAR] The music progress is ' + this.musicProgress);
        let ss = fileio.createStreamSync(path, 'r');
        let buf = new ArrayBuffer(bufferSize);
        console.log('[JSAR] The path stat size' + path + stat.size);
        this.isPlaying = true;
        this.audioUrl = 'common/images/pause.png';
        for (; this.readTimes < len; this.readTimes++) {
            this.musicProgress = this.readTimes / len * 100;
            console.log('[JSAR] The music progress is ' + this.musicProgress);
            if (!this.canAppContinuePlay) {
                this.isPlaying = false;
                this.audioUrl = 'common/images/play.png';
                break;
            }
            try {
                // Hint: readSync has some paras, related to read offset.
                let readOption = {
                    position: this.readTimes * bufferSize,
                    length: bufferSize,
                }
                console.log('[JSAR] Loading... read from ' + readOption.position + 'read size ' + readOption.length);
                await ss.readSync(buf, readOption);
                await this.audioRenderer.write(buf).then((writtenBytes) => {
                    if (writtenBytes < 0) {
                        console.error('[JSAR]Write failed.');
                    } else {
                        console.log('[JSAR]Actual written bytes: ' + writtenBytes);
                    }
                }).catch((err) => {
                    console.log('[JSAR]ERROR at renderer write: ' + err.message);
                    this.audioRenderer.release();
                });
            } catch (err) {
                console.log(err + '[JSAR] err=*' + JSON.stringify(err));
                await this.audioRenderer.release();
            }
        }
        this.isPlaying = false;
        this.audioUrl = 'common/images/play.png';
        // If the play is finished, play from the beginning
        if (this.readTimes == len) {
            this.readTimes = 0;
            await this.audioRenderer.stop();
            await this.startPlay();
        }
    },

    async continuePlay() {
        this.canAppContinuePlay = true;
        this.isPlaying = true;
        this.isPaused = false;
        this.isContinuePlay = true;
        this.audioUrl = 'common/images/pause.png';
        console.log("[JSAR] After startPlay, the isPlaying is: " + this.isPlaying);
        console.log("[JSAR] After startPlay, the renderer state is: " + this.audioRenderer.state);
        await this.startPlay();
    },

    async onPreviousClick() {
        this.readTimes = 0;
        this.isPaused = false;
        this.index--;
        if (this.index < 0 && this.playlist.audioFiles.length >= 1) {
            this.index = this.playlist.audioFiles.length - 1;
        }
        console.info('[JSAR] onPreviousClick, now index is: ' + this.index);
        this.currentProgress = 0;
        this.title = this.playlist.audioFiles[this.index].name;
        console.info('[JSAR] onPreviousClick, now the title is: ' + this.playlist.audioFiles[this.index].name);
        this.isPaused = true;
        await this.startPlay();
    },

    async onNextClick() {
        this.readTimes = 0;
        this.isPaused = false;
        this.index++;
        if (this.index >= this.playlist.audioFiles.length) {
            this.index = 0;
        }
        console.info('[JSAR] onNextClick, now index is: ' + this.index);
        this.currentProgress = 0;
        this.title = this.playlist.audioFiles[this.index].name;
        console.info('[JSAR] onNextClick, now the title is: ' + this.playlist.audioFiles[this.index].name);
        this.isPaused = true;
        await this.startPlay();
    },

    async onReplay() {
        this.readTimes = 0;
        this.canAppContinuePlay = true;
        this.isPaused = false;
        console.info('[JSAR] onReplay, now the title is: ' + this.playlist.audioFiles[this.index].name);
        await this.audioRenderer.stop();
        await this.startPlay();
    },

    async onChangeRate() {
        var tempRate = 0;
        var tempUrl = '';
        switch (this.currentRate) {
            case audio.AudioRendererRate.RENDER_RATE_NORMAL:
                tempRate = audio.AudioRendererRate.RENDER_RATE_DOUBLE;
                tempUrl = 'common/images/1.png'
                break;
            case audio.AudioRendererRate.RENDER_RATE_DOUBLE:
                tempRate = audio.AudioRendererRate.RENDER_RATE_HALF;
                tempUrl = 'common/images/2.png';
                break;
            case audio.AudioRendererRate.RENDER_RATE_HALF:
                tempRate = audio.AudioRendererRate.RENDER_RATE_NORMAL;
                tempUrl = 'common/images/0.png';
                break;
            default:
        }
        await this.audioRenderer.setRenderRate(tempRate).then(() => {
            console.log('[JSAR] setRenderRate SUCCESS ' + tempRate);
            this.currentRate = tempRate;
            this.rateImageUrl = tempUrl;
        }).catch((err) => {
            console.log('[JSAR] ERROR: ' + err.message);
        });
    },

    async onMuteAndUnmute() {
        let isMuteAfterClickButton;
        await this.audioManager.isMute(audio.AudioVolumeType.MEDIA).then((value) => {
            isMuteAfterClickButton = value;
        });
        await this.audioManager.mute(audio.AudioVolumeType.MEDIA,!isMuteAfterClickButton, (err) => {
            if (err) {
                console.error(' [JSAR] Failed to mute the stream. ${err.message}');
            } else {
                console.log('[JSAR] Callback invoked to indicate that the stream is muted. to ' + !isMuteAfterClickButton);
            }
        });
        await this.audioManager.isMute(audio.AudioVolumeType.MEDIA).then((value) => {
            if (value) {
                this.isMute = true;
                this.muteImageUrl = 'common/images/mute.png';
            } else {
                this.isMute = false;
                this.muteImageUrl = 'common/images/unmute.png';
            }
        });
    },

    media(e) {
        this.volume = e.value;
        this.audioManager.setVolume(audio.AudioVolumeType.MEDIA, this.volume).then(() => {
            console.log('[JSAR] Promise returned to indicate a successful volume setting. ' + this.volume);
            if (this.volume == 0) {
                this.volumeImageUrl = 'common/images/volumezero.png';
            } else {
                this.volumeImageUrl = 'common/images/volume.png';
            }
        })
    },

    showPanel() {
        this.$element('showPanel').show();
    },

    closePanel() {
        this.$element('showPanel').close();
    },

    backToIndex() {
        router.replace({
            uri: 'pages/index/index',
        })
    },

    async showInfo() {
        await this.getAndSetRendererInfo();
        await this.getAndSetManagerInfo();
        console.log("[JSAR] AudioRenderer " + this.state + ", " + this.rendererContent + ", " + this.renderRate + ", " + this.rendererUsage + ", " + this.rendererFlags + ", " + this.steamEncodingType + ", " + this.steamSampleFormat + ", " + this.steamSamplingRate + ", " + this.steamChannels + ", " + this.audioTime + ", " + this.bufferSize + ", " + this.renderRate);
        console.log("[JSAR] AudioManager " + this.volume + ", " + this.minVolume + ", " + this.maxVolume + ", " + this.isMute + ", " + this.isActive + ", " + this.ringMode + ", " + this.audioParameter + ", " + this.devices + ", " + this.deviceActive + ", " + this.isMicrophoneMuted);
        this.$element('showInfo').show();
    },

    async getAndSetRendererInfo() {
        this.state = this.audioRenderer.state
        await this.audioRenderer.getRendererInfo().then((rendererInfo) => {
            this.rendererContent = rendererInfo.content;
            this.rendererFlags = rendererInfo.rendererFlags;
            this.rendererUsage = rendererInfo.usage;
        }).catch((err) => {
            console.log('AudioFrameworkRenderLog: RendererInfo :ERROR: ' + err.message);
        });
        await this.audioRenderer.getStreamInfo().then((streamInfo) => {
            this.steamChannels = streamInfo.channels;
            this.steamSamplingRate = streamInfo.samplingRate;
            this.steamSampleFormat = streamInfo.sampleFormat;
            this.steamEncodingType = streamInfo.encodingType;
        }).catch((err) => {
            console.log('ERROR: ' + err.message);
        });
        await this.audioRenderer.getAudioTime().then((timestamp) => {
            this.audioTime = timestamp;
        }).catch((err) => {
            console.log('ERROR: ' + err.message);
        });
        await this.audioRenderer.getBufferSize().then((bufferSize) => {
            this.bufferSize = bufferSize;
        }).catch((err) => {
            console.log('ERROR: ' + err.message);
        });
        await this.audioRenderer.getRenderRate().then((renderRate) => {
            this.renderRate = renderRate;
        }).catch((err) => {
            console.log('ERROR: ' + err.message);
        });
    },

    async getAndSetManagerInfo() {
        await this.audioManager.getVolume(audio.AudioVolumeType.MEDIA).then((value) => {
            this.volume = value;
        });
        await this.audioManager.getMinVolume(audio.AudioVolumeType.MEDIA).then((value) => {
            this.minVolume = value;
        });
        await this.audioManager.getMaxVolume(audio.AudioVolumeType.MEDIA).then((data) => {
            this.maxVolume = data;
        });
        await this.audioManager.isMute(audio.AudioVolumeType.MEDIA).then((value) => {
            this.isMute = value;
        });
        await this.audioManager.isActive(audio.AudioVolumeType.MEDIA).then((value) => {
            this.isActive = value;
        });
        await this.audioManager.getRingerMode().then((value) => {
            this.ringMode = value;
        });
        await this.audioManager.getAudioParameter('PBits per sample').then((value) => {
            this.audioParameter = value;
        });
        await this.audioManager.getDevices(audio.DeviceFlag.OUTPUT_DEVICES_FLAG).then((data) => {
            this.devices = JSON.stringify(data);
        });
        await this.audioManager.isDeviceActive(audio.DeviceType.SPEAKER).then((value) => {
            this.deviceActive = value;
        });
        await this.audioManager.isMicrophoneMute().then((value) => {
            this.isMicrophoneMuted = value;
        });
    },

    closeInfo() {
        this.$element('showInfo').close();
    },
}



