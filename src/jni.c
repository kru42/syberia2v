/* jni_patch.c -- Fake Java Native Interface
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <psp2/io/fcntl.h>
#include <psp2/apputil.h>
#include <psp2/system_param.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "so_util.h"
#include "util.h"
#include "log.h"

enum MethodIDs
{
    UNKNOWN = 0,
    HAS_VIBRATOR,
    GET_LOCALE,
    GET_MODEL,
    GET_FILES_DIR,
    GET_PACKAGE_PATH,
    GET_LOCAL_PATH,
    GET_REGION_CODE,
    GET_LANGUAGE_CODE,
    GET_VALUE_DATA_STRING,
} MethodIDs;

typedef struct
{
    char*          name;
    enum MethodIDs id;
} NameToMethodID;

static NameToMethodID name_to_method_ids[] = {
    {"hasVibrator", HAS_VIBRATOR},
    {"getLocale", GET_LOCALE},
    {"getModel", GET_MODEL},
    {"getFilesDir", GET_FILES_DIR},
    {"getPackagePath", GET_PACKAGE_PATH},
    {"getLocalPath", GET_LOCAL_PATH},
    {"getRegionCode", GET_REGION_CODE},
    {"getLanguageCode", GET_LANGUAGE_CODE},
    {"getValueDataString", GET_VALUE_DATA_STRING},
};

char fake_vm[0x1000];
char fake_env[0x1000];

void* CallObjectMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallObjectMethod called");

    switch (methodID)
    {
    case GET_LOCAL_PATH:
        return DATA_PATH;
    default:
        return NULL;
    }
    return NULL;
}

void CallVoidMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallVoidMethod called");

    return;
}

void* CallStaticObjectMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallStaticObjectMethod called");

    switch (methodID)
    {
    case GET_FILES_DIR:
        return DATA_PATH;
    case GET_PACKAGE_PATH:
        return DATA_PATH "/main.3.ru.buka.syberia2.obb";
    case GET_LOCALE:
        // return getLocale();
        return "eu";
    case GET_LANGUAGE_CODE:
        return "en";
    case GET_REGION_CODE:
        return "US";
    case GET_MODEL:
        return "KFTHWI"; // for 60FPS
    case GET_VALUE_DATA_STRING:
        return "";
    default:
        return NULL;
    }
}

int CallStaticBooleanMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallStaticBooleanMethod called");

    switch (methodID)
    {
    case HAS_VIBRATOR:
        return 0;
    default:
        return 0;
    }
}

void CallStaticVoidMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallStaticVoidMethod called");
    return;
}

int NewObject(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("NewObject called", methodID);
    return -1;
}

void CallBooleanMethod(void* env, void* obj, int methodID, uintptr_t* args)
{
    log_info("CallBooleanMethod called");

    return;
}

int FindClass(void* env, const char* name)
{
    log_info("FindClass called. name=%s", name);

    return 0;
}

int GetMethodID(void* env, void* obj, const char* name, const char* sig)
{
    log_info("GetMethodID called. name=%s, sig=%s", name, sig);

    for (int i = 0; i < sizeof(name_to_method_ids) / sizeof(NameToMethodID); i++)
    {
        if (strcmp(name, name_to_method_ids[i].name) == 0)
        {
            return name_to_method_ids[i].id;
        }
    }

    log_info("GetMethodID: unknown method %s", name);
    return UNKNOWN;
}

int GetEnv(void* vm, void** env, int r2)
{
    log_info("GetEnv called");

    memset(fake_env, 'A', sizeof(fake_env));
    *(uintptr_t*)(fake_env + 0x00)  = (uintptr_t)fake_env;
    *(uintptr_t*)(fake_env + 0x18)  = (uintptr_t)FindClass;
    *(uintptr_t*)(fake_env + 0x74)  = (uintptr_t)NewObject;
    *(uintptr_t*)(fake_env + 0x84)  = (uintptr_t)GetMethodID;
    *(uintptr_t*)(fake_env + 0x8C)  = (uintptr_t)CallObjectMethod;
    *(uintptr_t*)(fake_env + 0x98)  = (uintptr_t)CallBooleanMethod;
    *(uintptr_t*)(fake_env + 0xF8)  = (uintptr_t)CallVoidMethod;
    *(uintptr_t*)(fake_env + 0x1CC) = (uintptr_t)CallStaticObjectMethod;
    *(uintptr_t*)(fake_env + 0x1D8) = (uintptr_t)CallStaticBooleanMethod;
    *(uintptr_t*)(fake_env + 0x238) = (uintptr_t)CallStaticVoidMethod;
    *env                            = fake_env;

    return 0;
}

void jni_load(void)
{
    memset(fake_vm, 'A', sizeof(fake_vm));
    *(uintptr_t*)(fake_vm + 0x00) = (uintptr_t)fake_vm; // just point to itself...
    *(uintptr_t*)(fake_vm + 0x10) = (uintptr_t)GetEnv;
    *(uintptr_t*)(fake_vm + 0x14) = (uintptr_t)ret0;
    *(uintptr_t*)(fake_vm + 0x18) = (uintptr_t)GetEnv;

    log_info("calling JNI_OnLoad");

    int (*JNI_OnLoad)(void* vm, void* reserved) = (void*)so_symbol(&syb2_mod, "JNI_OnLoad");
    JNI_OnLoad(fake_vm, NULL);

    log_info("JNI_OnLoad called");
}