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
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @since 12
 * @version 1.0
 */

#ifndef NATIVE_AUDIODEVICE_BASE_H
#define NATIVE_AUDIODEVICE_BASE_H

#include "native_audiostream_base.h"
#include "native_audiocommon_base.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines the audio device change type.
 *
 * @since 12
 */
typedef enum {
    /**
     * Device connection.
     *
     * @since 12
     */
    AUDIO_DEVICE_CHANGE_TYPE_CONNECT = 0,

    /**
     * Device disconnection.
     *
     * @since 12
     */
    AUDIO_DEVICE_CHANGE_TYPE_DISCONNECT = 1,
} OH_AudioDevice_ChangeType;

/**
 * Defines the audio device device role.
 *
 * @since 12
 */
typedef enum {
    /**
     * Input role.
     *
     * @since 12
     */
    AUDIO_DEVICE_CHANGE_ROLE_INPUT_DEVICE = 1,

    /**
     * Output role.
     *
     * @since 12
     */
    AUDIO_DEVICE_CHANGE_ROLE_OUTPUT_DEVICE = 2,
} OH_AudioDevice_DeviceRole;

/**
 * Defines the audio device device type.
 *
 * @since 12
 */
typedef enum {
    /**
     * Invalid device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_INVALID = 0,

    /**
     * Built-in earpiece.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_EARPIECE = 1,

    /**
     * Built-in speaker.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_SPEAKER = 2,

    /**
     * Wired headset, which is a combination of a pair of earpieces and a microphone.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_WIRED_HEADSET = 3,

    /**
     * A pair of wired headphones.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_WIRED_HEADPHONES = 4,

    /**
     * Bluetooth device using the synchronous connection oriented link (SCO).
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_BLUETOOTH_SCO = 7,

    /**
     * Bluetooth device using advanced audio distibution profile (A2DP).
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_BLUETOOTH_A2DP = 8,

    /**
     * Built-in microphone.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_MIC = 15,

    /**
     * USB audio headset.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_USB_HEADSET = 22,

    /**
     * Display port device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_DISPLAY_PORT = 24,

    /**
     * Default device type.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_TYPE_DEFAULT = 1000,
} OH_AudioDevice_DeviceType;

/**
 * Defines the audio device flag.
 *
 * @since 12
 */
typedef enum {
    /**
     * None device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_FLAG_NONE_DEVICES_FLAG = 0,

    /**
     * Output device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_FLAG_OUTPUT_DEVICES_FLAG = 1,

    /**
     * Input device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_FLAG_INPUT_DEVICES_FLAG = 2,

    /**
     * All device.
     *
     * @since 12
     */
    AUDIO_DEVICE_DEVICE_FLAG_ALL_DEVICES_FLAG = 3,
} OH_AudioDevice_DeviceFlag;

/**
 * Declaring the audio device descriptor.
 * The instance is used to get more audio device detail attributes.
 *
 * @since 12
 */
typedef struct OH_AudioDeviceDescriptor OH_AudioDeviceDescriptor;

/**
 * Declaring the audio device descriptor array.
 *
 * @since 12
 */
typedef struct OH_AudioDeviceDescriptorArray {
    /**
     * Audio device descriptor array size.
     *
     * @since 12
     */
    uint32_t size;

    /**
     * Audio device descriptor array.
     *
     * @since 12
     */
    OH_AudioDeviceDescriptor **descriptors;
} OH_AudioDeviceDescriptorArray;

/**
 * Query the device role of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param device role pointer variable that will be set the device role value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceRole(OH_AudioDeviceDescriptor*audioDeviceDescriptor,
    OH_AudioDevice_DeviceRole *deviceRole);

/**
 * Query the device type of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param device type pointer variable that will be set the device type value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceType(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    OH_AudioDevice_DeviceType *deviceType);

/**
 * Query the device id of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param device id pointer variable that will be set the device id value.
 * @return {@link #AUDIODEVICE_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceId(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t *id);

/**
 * Query the device name of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param device name pointer variable that will be set the device name value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceName(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **name);

/**
 * Query the device address of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param device address pointer variable that will be set the device address value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceAddress(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **address);

/**
 * Query the sample rate array of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param sample rate array pointer variable that will be set the sample rate array value.
 * @param sample rate size pointer variable that will be set the sample rate size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceSampleRates(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t **sampleRates, uint32_t *size);

/**
 * Query the device channel count array of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 *     OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param channel count array pointer variable that will be set the channel count array value.
 * @param channel count size pointer variable that will be set the channel count size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceChannelCounts(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t **channelCounts, uint32_t *size);

/**
 * Query the channel mask array of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param channel mask array pointer variable that will be set the channel mask array value.
 * @param channel mask size pointer variable that will be set the channel mask size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceChannelMasks(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    uint32_t **channelMasks, uint32_t *size);

/**
 * Query the display name of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param display name pointer variable that will be set the display name value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceDisplayName(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    char **displayName);

/**
 * Query the encoding type array of the target audio device descriptor.
 *
 * @since 12
 *
 * @param descriptor reference returned by OH_AudioRoutingManager_GetDevices or
 * OH_AudioRouterManager_OnDeviceChangedCallback.
 * @param encoding type array pointer variable that will be set the encoding type array value.
 * @param encoding type size pointer variable that will be set the encoding type size value.
 * @return {@link #AUDIOCOMMON_SUCCESS} or an undesired error.
 */
OH_AudioCommon_Result OH_AudioDeviceDescriptor_GetDeviceEncodingTypes(OH_AudioDeviceDescriptor *audioDeviceDescriptor,
    OH_AudioStream_EncodingType **encodingTypes, uint32_t *size);
#ifdef __cplusplus
}
#endif
#endif // NATIVE_AUDIODEVICE_BASE_H