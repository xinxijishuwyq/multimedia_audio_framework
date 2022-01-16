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

#ifndef AUDIO_RINGTONE_MANAGER_H
#define AUDIO_RINGTONE_MANAGER_H

#include <array>

#include "iringtone_sound_manager.h"
#include "media_log.h"
#include "audio_errors.h"
#include "player.h"
#include "context.h"
#include "abs_shared_result_set.h"
#include "data_ability_helper.h"
#include "data_ability_predicates.h"
#include "rdb_errno.h"
#include "result_set.h"
#include "uri.h"
#include "values_bucket.h"
#include "want.h"

namespace OHOS {
namespace AudioStandard {
namespace {
const int32_t SIM_COUNT = 2;
const float HIGH_VOL = 1.0f;
const float LOW_VOL = 0.0f;
const std::string MEDIALIB_DB_URI = "dataability:///com.ohos.medialibrary.MediaLibraryDataAbility";
const std::string MEDIA_DATA_DB_TITLE = "title";
const std::string MEDIA_DATA_DB_FILE_PATH = "path";
const std::string MEDIA_DATA_DB_RINGTONE_URI = "ringtone_uri";
const std::string MEDIA_DATA_DB_RINGTONE_TYPE = "ringtone_type";
const std::string MEDIA_KVSTOREOPRN = "kvstore_operation";
const std::string MEDIA_KVSTOREOPRN_SET_URI = "set_ringtone_uri";
const std::string MEDIA_KVSTOREOPRN_GET_URI = "get_ringtone_uri";
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RingtoneSoundManager"};
}

class RingtoneSoundManager : public IRingtoneSoundManager {
public:
    RingtoneSoundManager() = default;
    ~RingtoneSoundManager();

    // IRingtoneSoundManager override
    std::shared_ptr<IRingtonePlayer> GetRingtonePlayer(const std::shared_ptr<AppExecFwk::Context> &ctx,
        RingtoneType type) override;
    int32_t SetSystemNotificationUri(const std::shared_ptr<AppExecFwk::Context> &ctx, const std::string &uri) override;
    int32_t SetSystemAlarmUri(const std::shared_ptr<AppExecFwk::Context> &ctx, const std::string &uri) override;
    int32_t SetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &ctx, const std::string &uri,
        RingtoneType type) override;
    std::string GetSystemRingtoneUri(const std::shared_ptr<AppExecFwk::Context> &ctx, RingtoneType type) override;
    std::string GetSystemNotificationUri(const std::shared_ptr<AppExecFwk::Context> &ctx) override;
    std::string GetSystemAlarmUri(const std::shared_ptr<AppExecFwk::Context> &ctx) override;

private:
    void CreateDataAbilityHelper(const std::shared_ptr<AppExecFwk::Context> &ctx);
    std::array<std::string, SIM_COUNT> ringtoneUri_ = {};
    std::array<std::shared_ptr<IRingtonePlayer>, SIM_COUNT> ringtonePlayer_ = {nullptr};
    std::shared_ptr<AppExecFwk::DataAbilityHelper> abilityHelper_ = nullptr;
};

class RingtonePlayerCallback;
class RingtonePlayer : public IRingtonePlayer {
public:
    RingtonePlayer(const std::shared_ptr<AppExecFwk::Context> &ctx, RingtoneSoundManager &audioMgr, RingtoneType type);
    ~RingtonePlayer();
    void SetPlayerState(RingtoneState ringtoneState);

    // IRingtone override
    RingtoneState GetRingtoneState() override;
    int32_t Configure(const float &volume, const bool &loop) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;
    int32_t GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const override;
    std::string GetTitle() override;

private:
    void InitialisePlayer();
    int32_t PrepareRingtonePlayer(bool isReInitNeeded);

    float volume_ = HIGH_VOL;
    bool loop_ = false;
    bool isStartQueued_ = false;
    std::string configuredUri_ = "";
    std::shared_ptr<Media::Player> player_ = nullptr;
    std::shared_ptr<AppExecFwk::Context> context_;
    std::shared_ptr<RingtonePlayerCallback> callback_ = nullptr;
    RingtoneSoundManager &audioRingtoneMgr_;
    RingtoneType type_ = RINGTONE_TYPE_DEFAULT;
    RingtoneState ringtoneState_ = STATE_NEW;
};

class RingtonePlayerCallback : public Media::PlayerCallback {
public:
    RingtonePlayerCallback(RingtonePlayer &ringtonePlayer);
    virtual ~RingtonePlayerCallback() = default;
    void OnError(Media::PlayerErrorType errorType, int32_t errorCode) override;
    void OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody) override;

private:
    Media::PlayerStates state_ = Media::PLAYER_IDLE;
    RingtoneState ringtoneState_ = STATE_NEW;
    RingtonePlayer &ringtonePlayer_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_RINGTONE_MANAGER_H
