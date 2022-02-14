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

#include "device_status_listener.h"

#include <hdf_io_service_if.h>

#include "audio_errors.h"
#include "media_log.h"

namespace OHOS {
namespace AudioStandard {
static DeviceType GetDeviceTypeByName(std::string deviceName)
{
    DeviceType deviceType = DEVICE_TYPE_INVALID;

    if (!deviceName.compare(std::string("speaker")))
        deviceType = DEVICE_TYPE_SPEAKER;
    else if (!deviceName.compare(std::string("headset")))
        deviceType = DEVICE_TYPE_WIRED_HEADSET;
    else if (!deviceName.compare(std::string("bluetooth_sco")))
        deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    else if (!deviceName.compare(std::string("bluetooth_a2dp")))
        deviceType = DEVICE_TYPE_BLUETOOTH_A2DP;

    return deviceType;
}

static void OnServiceStatusReceived(struct ServiceStatusListener *listener,
                                    struct ServiceStatus *serviceStatus)
{
    MEDIA_DEBUG_LOG("[DeviceStatusListener] OnServiceStatusReceived in");
    MEDIA_DEBUG_LOG("[DeviceStatusListener]: service name: %{public}s", serviceStatus->serviceName);

    DeviceType deviceType = GetDeviceTypeByName(serviceStatus->info);
    if (deviceType != DEVICE_TYPE_INVALID) {
        if ((serviceStatus->status == SERVIE_STATUS_START) || (serviceStatus->status == SERVIE_STATUS_STOP)) {
            bool connected = (serviceStatus->status == SERVIE_STATUS_START) ? true : false;
            MEDIA_DEBUG_LOG("[DeviceStatusListener]: Device: %{public}s: connected: %{public}s",
                            serviceStatus->info, connected ? "true" : "false");
            DeviceStatusListener *deviceStatusListener = reinterpret_cast<DeviceStatusListener *>(listener->priv);
            deviceStatusListener->deviceObserver_.OnDeviceStatusUpdated(deviceType, connected,
                                                                        deviceStatusListener->privData_);
        }
    }
}

DeviceStatusListener::DeviceStatusListener(IDeviceStatusObserver &observer)
    : deviceObserver_(observer), hdiServiceManager_(nullptr), listener_(nullptr) {}

DeviceStatusListener::~DeviceStatusListener() = default;

int32_t DeviceStatusListener::RegisterDeviceStatusListener(void *privData)
{
    hdiServiceManager_ = HDIServiceManagerGet();
    if (hdiServiceManager_ == nullptr) {
        MEDIA_ERR_LOG("[DeviceStatusListener]: Get HDI service manager failed");
        return ERR_OPERATION_FAILED;
    }

    privData_ = privData;
    listener_ = HdiServiceStatusListenerNewInstance();
    listener_->callback = OnServiceStatusReceived;
    listener_->priv = (void *)this;
    int32_t status = hdiServiceManager_->RegisterServiceStatusListener(hdiServiceManager_, listener_,
                                                                       DEVICE_CLASS_DEFAULT);
    if (status != HDF_SUCCESS) {
        MEDIA_ERR_LOG("[DeviceStatusListener]: Register service status listener failed");
        return ERR_OPERATION_FAILED;
    }

    return SUCCESS;
}

int32_t DeviceStatusListener::UnRegisterDeviceStatusListener()
{
    if ((hdiServiceManager_ == nullptr) || (listener_ == nullptr)) {
        return ERR_ILLEGAL_STATE;
    }

    int32_t status = hdiServiceManager_->UnregisterServiceStatusListener(hdiServiceManager_, listener_);
    if (status != HDF_SUCCESS) {
        MEDIA_ERR_LOG("[DeviceStatusListener]: UnRegister service status listener failed");
        return ERR_OPERATION_FAILED;
    }

    hdiServiceManager_ = nullptr;
    listener_ = nullptr;
    privData_ = nullptr;

    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
