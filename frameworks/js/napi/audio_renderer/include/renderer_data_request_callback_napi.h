/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDERER_DATA_REQUEST_CALLBACK_NAPI_H_
#define RENDERER_DATA_REQUEST_CALLBACK_NAPI_H_

#include "audio_common_napi.h"
#include "audio_renderer.h"
#include "audio_renderer_napi.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace AudioStandard {
class RendererDataRequestCallbackNapi : public AudioRendererWriteCallback {
public:
    RendererDataRequestCallbackNapi(napi_env env, AudioRendererNapi *rendererNapi);
    virtual ~RendererDataRequestCallbackNapi();
    void SaveCallbackReference(const std::string &callbackName, napi_value callback);
    void OnWriteData(size_t length) override;

private:
    struct RendererDataRequestJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        BufferDesc bufDesc_ {};
        AudioRendererNapi *rendererNapiObj;
        AudioRendererDataInfo audioRendererDataInfo;
    };

    void OnJsRendererDataRequestCallback(std::unique_ptr<RendererDataRequestJsCallback> &jsCb);
    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> rendererDataRequestCallback_ = nullptr;
    AudioRendererNapi *rendererNapi_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // RENDERER_DATA_REQUEST_CALLBACK_NAPI_H_
