/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <Hilog/log.h>
#include <SLES/OpenSLES.h>
#include "napi/native_api.h"


// The required functions are implemented here, and the return values are all "native_ Value", for JS to call.
static napi_value Create(napi_env env, napi_callback_info info)
{
    // Get the parameter obtained from the front end and print using API "OH_LOG_Print".
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);
    int value0;
    napi_get_value_int32(env, args[0], &value0);
    int valueForCompare = 5;
    if (value0 > valueForCompare) {
        OH_LOG_Print(LOG_APP, LOG_INFO, 0, "[HiLog]", "Your digit is bigger than 5");
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, 0, "[HiLog]", "Your digit is smaller than 6");
    }

    // Call the API "slCreateEngine" and return data to the front end.
    char successStr[] = "Engine is successfully created";
    char failStr[] = "Failed to create engine";
    SLObjectItf engineObject = NULL; // Declare engine interface object
    SLresult result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    napi_value resultStr;
    if (SL_RESULT_SUCCESS == result) {
        napi_create_string_utf8(env, successStr, sizeof(successStr), &resultStr);
    } else {
        napi_create_string_utf8(env, failStr, sizeof(failStr), &resultStr);
    }
    return resultStr;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    // The first para is the method name of JS call, the third para is the method name of cpp method.
    napi_property_descriptor desc[] = {
        { "create", nullptr, Create, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module
demoModule = {
.nm_version = 1,
.nm_flags = 0,
.nm_filename = nullptr,
.nm_register_func = Init,
.nm_modname = "libnative",
.nm_priv = ((void *)0),
.reserved = {
0 },
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
napi_module_register(& demoModule);
}