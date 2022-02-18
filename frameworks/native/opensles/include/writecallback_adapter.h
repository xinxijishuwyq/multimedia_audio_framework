#ifndef WRITECALLBACK_ADAPTER_H
#define WRITECALLBACK_ADAPTER_H

#include <OpenSLES.h>
#include <OpenSLES_Platform.h>
#include <iostream>
#include <audio_renderer.h>


namespace OHOS {
namespace AudioStandard {
class WriteCallbackAdapter : public AudioRendererWriteCallback {
public:
    WriteCallbackAdapter (SlOHBufferQueueCallback callback, SLOHBufferQueueItf itf, void *pContext);
    ~WriteCallbackAdapter();
    void OnWriteData(size_t length) override;

private:
    SlOHBufferQueueCallback callback_;
    SLOHBufferQueueItf itf_;
    void *context_;
    
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_RENDERER_SINK_H