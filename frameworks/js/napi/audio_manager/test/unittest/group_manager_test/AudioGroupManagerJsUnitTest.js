/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'


describe("AudioGroupManagerJsUnitTest", function () {
    const ACCESSIBILITY = 5;
    const ALARM = 4;
    let audioManager = audio.getAudioManager();
    let audioVolumeManager = audioManager.getVolumeManager();
    const GROUP_ID = audio.DEFAULT_VOLUME_GROUP_ID;
    let audioVolumeGroupManager;

    beforeAll(async function () {
        audioVolumeGroupManager = await audioVolumeManager.getVolumeGroupManager(GROUP_ID).catch((err) => {
            console.error("Create audioVolumeManager error " + JSON.stringify(err));
        });
        console.info("Create audioVolumeManager finished");

        // input testsuit setup step，setup invoked before all testcases
        console.info('beforeAll called')
    })

    afterAll(function () {

        // input testsuit teardown step，teardown invoked after all testcases
        console.info('afterAll called')
    })

    beforeEach(function () {

        // input testcase setup step，setup invoked before each testcases
        console.info('beforeEach called')
    })

    afterEach(function () {

        // input testcase teardown step，teardown invoked after each testcases
        console.info('afterEach called')
    })

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_001
     * @tc.desc:verify alarm Volume set successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_001", 0, async function (done) {
        let volume = 4;
        audioVolumeGroupManager.setVolume(ALARM, volume, (err) => {
            if (err) {
                console.error(`Failed to set ALARM volume. ${err}`);
                expect(false).assertTrue();
                done();
                return;
            }
            console.info('invoked to indicate a successful volume ALARM setting.');
            expect(true).assertTrue();

            audioVolumeGroupManager.getVolume(ALARM, (err, value) => {
                if (err) {
                    console.error(`Failed to obtain ALARM volume. ${err}`);
                    expect(false).assertTrue();
                    done();
                    return;
                }
                console.info(`get alarm volume is obtained ${value}.`);
                expect(value).assertEqual(volume);
                done();
            })
        })
    })

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_002
     * @tc.desc:Verify whether the abnormal volume setting is successful
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_002", 0, async function (done) {
        let volume = -1;
        audioVolumeGroupManager.setVolume(ALARM, volume, (err) => {
            if (err) {
                console.error(`Failed to set ALARM volume. ${err}`);
                expect(true).assertTrue();
                done();
                return;
            }
            console.info('Callback invoked to indicate a successful volume setting.');
            expect(false).assertTrue();
            done();
        })
    })

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_003
     * @tc.desc:Verify whether the abnormal volume setting is successful
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_003", 0, async function (done) {
        let volume = 17;
        audioVolumeGroupManager.setVolume(ALARM, volume, (err) => {
            if (err) {
                console.error(`Failed to set ALARM volume. ${err}`);
                expect(true).assertTrue();
                done();
                return;
            }
            console.info('invoked to indicate a successful ALARM volume setting.');
            expect(false).assertTrue();
            done();
        })
    });

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_004
     * @tc.desc:verify alarm Volume set successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_004", 0, async function (done) {
        let volume = 6;
        audioVolumeGroupManager.setVolume(ACCESSIBILITY, volume, (err) => {
            if (err) {
                console.error(`Failed to set ACCESSIBILITY volume. ${err}`);
                expect(false).assertTrue();
                done();
                return;
            }
            console.info('invoked to indicate a successful ACCESSIBILITY volume setting.');
            expect(true).assertTrue();

            audioVolumeGroupManager.getVolume(ACCESSIBILITY, (err, value) => {
                if (err) {
                    console.error(`Failed to obtain ACCESSIBILITY volume. ${err}`);
                    expect(false).assertTrue();
                    done();
                    return;
                }
                console.info(`get ACCESSIBILITY volume is obtained ${value}.`);
                expect(value).assertEqual(volume);
                done();
            })
        })
    })

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_005
     * @tc.desc:Verify whether the abnormal volume setting is successful
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_005", 0, async function (done) {
        let volume = -3;
        audioVolumeGroupManager.setVolume(ACCESSIBILITY, volume, (err) => {
            if (err) {
                console.error(`Failed to set ALARM volume. ${err}`);
                expect(true).assertTrue();
                done();
                return;
            }
            console.info('invoked to indicate a successful ACCESSIBILITY volume setting.');
            expect(false).assertTrue();
            done();
        })
    })

    /*
     * @tc.name:SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_006
     * @tc.desc:Verify whether the abnormal volume setting is successful
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it("SUB_AUDIO_GROUP_MANAGER_SET_VOLUME_006", 0, async function (done) {
        let volume = 16;
        audioVolumeGroupManager.setVolume(ACCESSIBILITY, volume, (err) => {
            if (err) {
                console.error(`Failed to set ALARM volume. ${err}`);
                expect(true).assertTrue();
                done();
                return;
            }
            console.info('invoked to indicate a successful ACCESSIBILITY volume setting.');
            expect(false).assertTrue();
            done();
        })
    })
})