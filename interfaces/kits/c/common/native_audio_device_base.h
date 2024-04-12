/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/**
 * @addtogroup OHAudio
 * @{
 *
 * @brief Provide the definition of the C interface for the audio module.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 *
 * @since 12
 * @version 1.0
 */

/**
 * @file native_audiodevice_base.h
 *
 * @brief Declare audio device related interfaces for audio device descriptor.
 *
 * @library libohaudio.so
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @since 12
 * @version 1.0
 */

#ifndef NATIVE_AUDIODEVICE_BASE_H
#define NATIVE_AUDIODEVICE_BASE_H

#include "native_audiostream_base.h"
#include "native_common_base.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Defines the audio device change type.
 *
 * @since 12
 */
typedef enum {
    /**
     * @brief Device connection.
     */
    AUDIO_DEVICE_CHANGE_TYPE_CONNECT = 0,

    /**
     * @brief Device disconnection.
     */
    AUDIO_DEVICE_CHANGE_TYPE_DISCONNECT = 1,
} OH_AudioDevice_ChangeType;

/**
 * @brief Defines the audio device device role.
 *
 * @since 12
 */
typedef enum {
    /**
     * @brief Input role.
     */
    AUDIO_DEVICE_CHANGE_ROLE_INPUT_DEVICE = 1,

    /**
     * @brief Output role.
     */
    AUDIO_DEVICE_CHANGE_ROLE_OUTPUT_DEVICE = 2,
} OH_AudioDevice_DeviceRole;

/**
 * @brief Defines the audio device device type.
 *
 * @since 12
 */
typedef enum {
    /**
     * @brief Invalid device.
     */
    AUDIO_DEVICE_DEVICE_TYPE_INVALID = 0,

    /**
     * @brief Built-in earpiece.
     */
    AUDIO_DEVICE_DEVICE_TYPE_EARPIECE = 1,

    /**
     * @brief Built-in speaker.
     */
    AUDIO_DEVICE_DEVICE_TYPE_SPEAKER = 2,

    /**
     * @brief Wired headset, which is a combination of a pair of earpieces and a microphone.
     */
    AUDIO_DEVICE_DEVICE_TYPE_WIRED_HEADSET = 3,

    /**
     * @brief A pair of wired headphones.
     */
    AUDIO_DEVICE_DEVICE_TYPE_WIRED_HEADPHONES = 4,

    /**
     * @brief Bluetooth device using the synchronous connection oriented link (SCO).
     */
    AUDIO_DEVICE_DEVICE_TYPE_BLUETOOTH_SCO = 7,

    /**
     * @brief Bluetooth device using advanced audio distibution profile (A2DP).
     */
    AUDIO_DEVICE_DEVICE_TYPE_BLUETOOTH_A2DP = 8,

    /**
     * @brief Built-in microphone.
     */
    AUDIO_DEVICE_DEVICE_TYPE_MIC = 15,

    /**
     * @brief USB audio headset.
     */
    AUDIO_DEVICE_DEVICE_TYPE_USB_HEADSET = 22,

    /**
     * @brief Display port device.
     */
    AUDIO_DEVICE_DEVICE_TYPE_DISPLAY_PORT = 24,

    /**
     * @brief Default device type.
     */
    AUDIO_DEVICE_DEVICE_TYPE_DEFAULT = 1000,
} OH_AudioDevice_DeviceType;

/**
 * @brief Defines the audio device flag.
 *
 * @since 12
 */
typedef enum {
    /**
     * @brief None device.
     */
    AUDIO_DEVICE_DEVICE_FLAG_NONE_DEVICES_FLAG = 0,

    /**
     * @brief Output device.
     */
    AUDIO_DEVICE_DEVICE_FLAG_OUTPUT_DEVICES_FLAG = 1,

    /**
     * @brief Input device.
     */
    AUDIO_DEVICE_DEVICE_FLAG_INPUT_DEVICES_FLAG = 2,

    /**
     * @brief All device.
     */
    AUDIO_DEVICE_DEVICE_FLAG_ALL_DEVICES_FLAG = 3,
} OH_AudioDevice_DeviceFlag;

/**
 * @brief Declaring the audio device descriptor.
 * The instance is used to get more audio device detail attributes.
 *
 * @since 12
 */
typedef struct OH_AudioDeviceDescriptor OH_AudioDeviceDescriptor;

/**
 * @brief Declaring the audio device descriptor array.
 *
 * @since 12
 */
typedef struct OH_AudioDeviceDescriptorArray {
    /**
     * @brief Audio device descriptor array size.
     */
    uint32_t size;

    /**
     * @brief Audio device descriptor array.
     */
    OH_AudioDeviceDescriptor **descriptors;
} OH_AudioDeviceDescriptorArray;

/**
 * @brief Query the device role of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param deviceRole pointer variable that will be set the device role value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceRole(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    OH_AudioDevice_DeviceRole *deviceRole);

/**
 * @brief Query the device type of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param deviceType pointer variable that will be set the device type value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceType(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    OH_AudioDevice_DeviceType *deviceType);

/**
 * @brief Query the device id of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param id pointer variable that will be set the device id value.
 * @return {@link #AUDIODEVICE_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceId(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t *id);

/**
 * @brief Query the device name of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param name pointer variable that will be set the device name value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceName(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **name);

/**
 * @brief Query the device address of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param address pointer variable that will be set the device address value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceAddress(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **address);

/**
 * @brief Query the sample rate array of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param sampleRates array pointer variable that will be set the sample rate array value.
 * @param size pointer variable that will be set the sample rate size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceSampleRates(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t **sampleRates, uint32_t *size);

/**
 * @brief Query the device channel count array of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param channelCounts array pointer variable that will be set the channel count array value.
 * @param size pointer variable that will be set the channel count size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceChannelCounts(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t **channelCounts, uint32_t *size);

/**
 * @brief Query the display name of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param displayName pointer variable that will be set the display name value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceDisplayName(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **displayName);

/**
 * @brief Query the encoding type array of the target audio device descriptor.
 *
 * @param audioDeviceDescriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param encodingTypes array pointer variable that will be set the encoding type array value.
 * @param size pointer variable that will be set the encoding type size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceEncodingTypes(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    OH_AudioStream_EncodingType **encodingTypes, uint32_t *size);
#ifdef __cplusplus
}
#endif
#endif // NATIVE_AUDIODEVICE_BASE_H