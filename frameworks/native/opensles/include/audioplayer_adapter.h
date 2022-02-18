#ifndef AUDIOPLAYER_ADAPTER_H
#define AUDIOPLAYER_ADAPTER_H

#include <OpenSLES.h>
#include <OpenSLES_Platform.h>
#include <iostream>
#include <map>
#include <audio_renderer.h>
#include <audio_system_manager.h>
#include <writecallback_adapter.h>

namespace OHOS {
namespace AudioStandard {

class AudioPlayerAdapter {
public:
    static AudioPlayerAdapter* GetInstance();
    AudioRenderer *GetAudioRenderById(SLuint32 id);
    void EraseAudioRenderById(SLuint32 id);
    SLresult CreateAudioPlayerAdapter(SLuint32 id, SLDataSource *dataSource, SLDataSink *dataSink, AudioStreamType streamType);
    SLresult SetPlayStateAdapter(SLuint32 id, SLuint32 state);
    SLresult GetPlayStateAdapter(SLuint32 id, SLuint32 *state);
    SLresult SetVolumeLevelAdapter(SLuint32 id, SLmillibel level);
    SLresult GetVolumeLevelAdapter(SLuint32 id, SLmillibel *level);
    SLresult GetMaxVolumeLevelAdapter(SLuint32 id, SLmillibel *level);
    SLresult EnqueueAdapter(SLuint32 id, const void *pBuffer, SLuint32 size);
    SLresult ClearAdapter(SLuint32 id);
    SLresult GetStateAdapter(SLuint32 id, SLOHBufferQueueState *pState);
    SLresult GetBufferAdapter(SLuint32 id, SLuint8 **pBuffer, SLuint32 &pSize);
    SLresult RegisterCallbackAdapter(SLOHBufferQueueItf itf, SlOHBufferQueueCallback callback, void *pContext);
    
private:
    AudioPlayerAdapter();
    ~AudioPlayerAdapter();
    std::map<SLuint32, AudioRenderer*> renderMap_;
    std::shared_ptr<WriteCallbackAdapter> callbackPtr_;
    std::map<SLuint32, std::shared_ptr<WriteCallbackAdapter>> callbackMap_;

    void ConvertPcmFormat(SLDataFormat_PCM *slFormat, AudioRendererParams *rendererParams);
    AudioSampleFormat SlToOhosSampelFormat(SLDataFormat_PCM *pcmFormat);
    AudioSamplingRate SlToOhosSamplingRate(SLDataFormat_PCM *pcmFormat);
    AudioChannel SlToOhosChannel(SLDataFormat_PCM *pcmFormat);
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_RENDERER_SINK_H