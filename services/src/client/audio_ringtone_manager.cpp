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

#include "audio_ringtone_manager.h"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media;

namespace OHOS {
namespace AudioStandard {
unique_ptr<IRingtoneSoundManager> RingtoneFactory::CreateRingtoneManager()
{
    unique_ptr<RingtoneSoundManager> soundMgr = make_unique<RingtoneSoundManager>();
    CHECK_AND_RETURN_RET_LOG(soundMgr != nullptr, nullptr, "Failed to create sound manager object");

    return soundMgr;
}

RingtoneSoundManager::~RingtoneSoundManager()
{
    if (abilityHelper_ != nullptr) {
        abilityHelper_->Release();
        abilityHelper_ = nullptr;
    }
}

int32_t RingtoneSoundManager::SetSystemRingtoneUri(const shared_ptr<Context> &context, const string &uri,
    RingtoneType type)
{
    int32_t result = 0;
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM,
                             ERR_INVALID_PARAM, "invalid type");

    CreateDataAbilityHelper(context);

    ValuesBucket valuesBucket;
    valuesBucket.PutString(MEDIA_DATA_DB_RINGTONE_URI, uri);
    valuesBucket.PutInt(MEDIA_DATA_DB_RINGTONE_TYPE, type);

    string abilityUri = MEDIALIB_DB_URI;
    Uri setRingtoneUri(abilityUri + "/" + MEDIA_KVSTOREOPRN + "/" + MEDIA_KVSTOREOPRN_SET_URI);

    CHECK_AND_RETURN_RET_LOG(abilityHelper_ != nullptr, ERR_INVALID_PARAM, "Helper is null, failed to update uri");
    result = abilityHelper_->Insert(setRingtoneUri, valuesBucket);
    if (result != SUCCESS) {
        MEDIA_ERR_LOG("RingtoneSoundManager::insert ringtone uri failed");
    }

    return result;
}

string RingtoneSoundManager::GetSystemRingtoneUri(const shared_ptr<Context> &context, RingtoneType type)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    string ringtoneUri = "";
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM, "", "invalid type");

    CreateDataAbilityHelper(context);

    string abilityUri = MEDIALIB_DB_URI;
    Uri getRingtoneUri(abilityUri + "/"
                       + MEDIA_KVSTOREOPRN + "/"
                       + MEDIA_KVSTOREOPRN_GET_URI + "/"
                       + to_string(type));

    CHECK_AND_RETURN_RET_LOG(abilityHelper_ != nullptr, "", "Helper is null, failed to retrieve uri");
    ringtoneUri = abilityHelper_->GetType(getRingtoneUri);

    return ringtoneUri;
}

int32_t RingtoneSoundManager::SetSystemNotificationUri(const shared_ptr<Context> &context, const string &uri)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    return ERR_NOT_SUPPORTED;
}

int32_t RingtoneSoundManager::SetSystemAlarmUri(const shared_ptr<Context> &context, const string &uri)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    return ERR_NOT_SUPPORTED;
}

string RingtoneSoundManager::GetSystemNotificationUri(const shared_ptr<Context> &context)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    return "";
}

string RingtoneSoundManager::GetSystemAlarmUri(const shared_ptr<Context> &context)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s", __func__);
    return "";
}

void RingtoneSoundManager::CreateDataAbilityHelper(const shared_ptr<Context> &context)
{
    if (abilityHelper_ == nullptr) {
        auto contextUri = make_unique<Uri>(MEDIALIB_DB_URI);
        CHECK_AND_RETURN_LOG(contextUri != nullptr, "failed to create context uri");

        abilityHelper_ = DataAbilityHelper::Creator(context, move(contextUri));
        CHECK_AND_RETURN_LOG(abilityHelper_ != nullptr, "Unable to create data ability helper");
    }
}

shared_ptr<IRingtonePlayer> RingtoneSoundManager::GetRingtonePlayer(const shared_ptr<Context> &context,
    RingtoneType type)
{
    MEDIA_INFO_LOG("RingtoneSoundManager::%{public}s, type %{public}d", __func__, type);
    CHECK_AND_RETURN_RET_LOG(type >= RINGTONE_TYPE_DEFAULT && type <= RINGTONE_TYPE_MULTISIM, nullptr, "invalid type");

    if (ringtonePlayer_[type] != nullptr && ringtonePlayer_[type]->GetRingtoneState() == STATE_RELEASED) {
        ringtonePlayer_[type] = nullptr;
    }

    if (ringtonePlayer_[type] == nullptr) {
        ringtonePlayer_[type] = make_shared<RingtonePlayer>(context, *this, type);
        CHECK_AND_RETURN_RET_LOG(ringtonePlayer_[type] != nullptr, nullptr, "Failed to create ringtone player object");
    }

    return ringtonePlayer_[type];
}

// Player class symbols
RingtonePlayer::RingtonePlayer(const shared_ptr<Context> &context, RingtoneSoundManager &audioMgr, RingtoneType type)
    : volume_(HIGH_VOL), loop_(false), context_(context), audioRingtoneMgr_(audioMgr), type_(type)
{
    InitialisePlayer();
    (void)Configure(volume_, loop_);
}

RingtonePlayer::~RingtonePlayer()
{
    if (player_ != nullptr) {
        player_->Release();
        player_ = nullptr;
        callback_ = nullptr;
    }
}

void RingtonePlayer::InitialisePlayer()
{
    player_ = Media::PlayerFactory::CreatePlayer();
    CHECK_AND_RETURN_LOG(player_ != nullptr, "Failed to create ringtone player instance");

    callback_ = std::make_shared<RingtonePlayerCallback>(*this);
    CHECK_AND_RETURN_LOG(callback_ != nullptr, "Failed to create callback object");

    player_->SetPlayerCallback(callback_);
    ringtoneState_ = STATE_NEW;
    configuredUri_ = "";
}

int32_t RingtonePlayer::PrepareRingtonePlayer(bool isReInitNeeded)
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);

    // fetch uri from kvstore
    auto kvstoreUri = audioRingtoneMgr_.GetSystemRingtoneUri(context_, type_);
    CHECK_AND_RETURN_RET_LOG(!kvstoreUri.empty(), ERR_INVALID_PARAM, "Failed to obtain ringtone uri for playing");

    // If uri is different from from configure uri, reset the player
    if (kvstoreUri != configuredUri_ || isReInitNeeded) {
        (void)player_->Reset();

        auto ret = player_->SetSource(kvstoreUri);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Set source failed %{public}d", ret);

        ret = player_->PrepareAsync();
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Prepare failed %{public}d", ret);

        configuredUri_ = kvstoreUri;
        ringtoneState_ = STATE_NEW;
    }

    return SUCCESS;
}

int32_t RingtonePlayer::Configure(const float &volume, const bool &loop)
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);

    CHECK_AND_RETURN_RET_LOG(volume >= LOW_VOL && volume <= HIGH_VOL, ERR_INVALID_PARAM, "Volume level invalid");
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, ERR_INVALID_PARAM, "no player_");

    loop_ = loop;
    volume_ = volume;

    if (ringtoneState_ != STATE_NEW) {
        (void)player_->SetVolume(volume_, volume_);
        (void)player_->SetLooping(loop_);
    }

    (void)PrepareRingtonePlayer(false);

    return SUCCESS;
}

int32_t RingtonePlayer::Start()
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);

    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, ERR_INVALID_PARAM, "no player_");

    if (player_->IsPlaying() || isStartQueued_) {
        MEDIA_ERR_LOG("Play in progress, cannot start now");
        return ERROR;
    }

    // Player doesnt support play in stopped state. Hence reiniialise player for making start<-->stop to work
    if (ringtoneState_ == STATE_STOPPED) {
        (void)PrepareRingtonePlayer(true);
    } else {
        (void)PrepareRingtonePlayer(false);
    }

    if (ringtoneState_ == STATE_NEW) {
        MEDIA_INFO_LOG("Start received before player preparing is finished");
        isStartQueued_ = true;
        return SUCCESS;
    }

    auto ret = player_->Play();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR, "Start failed %{public}d", ret);

    ringtoneState_ = STATE_RUNNING;

    return SUCCESS;
}

int32_t RingtonePlayer::Stop()
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(player_ != nullptr && ringtoneState_ != STATE_INVALID, ERR_INVALID_PARAM, "no player_");

    if (ringtoneState_ != STATE_STOPPED && player_->IsPlaying()) {
        (void)player_->Stop();
    }

    ringtoneState_ = STATE_STOPPED;
    isStartQueued_ = false;

    return SUCCESS;
}

int32_t RingtonePlayer::Release()
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s player", __func__);

    if (player_ != nullptr) {
        (void)player_->Release();
    }

    ringtoneState_ = STATE_RELEASED;
    player_ = nullptr;
    callback_ = nullptr;

    return SUCCESS;
}

RingtoneState RingtonePlayer::GetRingtoneState()
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);
    return ringtoneState_;
}

void RingtonePlayer::SetPlayerState(RingtoneState ringtoneState)
{
    if (ringtoneState_ != RingtoneState::STATE_RELEASED) {
        ringtoneState_ = ringtoneState;
    }

    if (ringtoneState_ == RingtoneState::STATE_PREPARED) {
        MEDIA_INFO_LOG("Player prepared callback received. loop:%{public}d volume:%{public}f", loop_, volume_);

        Media::Format format;
        format.PutIntValue(Media::PlayerKeys::CONTENT_TYPE, CONTENT_TYPE_RINGTONE);
        format.PutIntValue(Media::PlayerKeys::STREAM_USAGE, STREAM_USAGE_NOTIFICATION_RINGTONE);

        (void)player_->SetParameter(format);
        (void)player_->SetVolume(volume_, volume_);
        (void)player_->SetLooping(loop_);

        if (isStartQueued_) {
            auto ret = player_->Play();
            isStartQueued_ = false;
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "Play failed %{public}d", ret);
            ringtoneState_ = RingtoneState::STATE_RUNNING;
        }
    }
}

int32_t RingtonePlayer::GetAudioRendererInfo(AudioStandard::AudioRendererInfo &rendererInfo) const
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);
    rendererInfo.contentType = ContentType::CONTENT_TYPE_RINGTONE;
    rendererInfo.streamUsage = StreamUsage::STREAM_USAGE_NOTIFICATION_RINGTONE;
    rendererInfo.rendererFlags = 0;
    return SUCCESS;
}

std::string RingtonePlayer::GetTitle()
{
    MEDIA_INFO_LOG("RingtonePlayer::%{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, "", "context cannot be null");

    auto contextUri = make_unique<Uri>(MEDIALIB_DB_URI);
    CHECK_AND_RETURN_RET_LOG(contextUri != nullptr, "", "failed to create context uri");

    shared_ptr<DataAbilityHelper> helper = DataAbilityHelper::Creator(context_, move(contextUri));
    CHECK_AND_RETURN_RET_LOG(helper != nullptr, "", "Unable to create data ability helper");

    Uri mediaLibUri(MEDIALIB_DB_URI);

    vector<string> columns = {};
    columns.push_back(MEDIA_DATA_DB_TITLE);

    auto uri = audioRingtoneMgr_.GetSystemRingtoneUri(context_, type_);
    DataAbilityPredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_FILE_PATH, uri);

    shared_ptr<AbsSharedResultSet> resultSet = nullptr;
    resultSet = helper->Query(mediaLibUri, columns, predicates);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, "", "Unable to fetch details for path %{public}s", uri.c_str());

    int ret = resultSet->GoToFirstRow();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, "", "Failed to obtain the record");

    int32_t titleIndex(0);
    string title("");
    resultSet->GetColumnIndex(MEDIA_DATA_DB_TITLE, titleIndex);
    resultSet->GetString(titleIndex, title);

    helper->Release();

    return title;
}

// Callback class symbols
RingtonePlayerCallback::RingtonePlayerCallback(RingtonePlayer &ringtonePlayer) : ringtonePlayer_(ringtonePlayer)
{}

void RingtonePlayerCallback::OnError(PlayerErrorType errorType, int32_t errorCode)
{
    MEDIA_ERR_LOG("Error reported from media server %{public}d", errorCode);
}

void RingtonePlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    if (type == INFO_TYPE_STATE_CHANGE) {
        state_ = static_cast<PlayerStates>(extra);

        switch (state_) {
            case PLAYER_STATE_ERROR:
                ringtoneState_ = STATE_INVALID;
                break;
            case PLAYER_IDLE:
            case PLAYER_INITIALIZED:
            case PLAYER_PREPARING:
                ringtoneState_ = STATE_NEW;
                break;
            case PLAYER_PREPARED:
                ringtoneState_ = STATE_PREPARED;
                break;
            case PLAYER_STARTED:
                ringtoneState_ = STATE_RUNNING;
                break;
            case PLAYER_PAUSED:
                ringtoneState_ = STATE_PAUSED;
                break;
            case PLAYER_STOPPED:
            case PLAYER_PLAYBACK_COMPLETE:
                ringtoneState_ = STATE_STOPPED;
                break;
            default:
                break;
        }

        ringtonePlayer_.SetPlayerState(ringtoneState_);
    }
}
} // AudioStandard
} // OHOS
