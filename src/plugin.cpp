#include "plugin.h"
#include <cstring>
#include "aac_encoder.h"

static const uint8_t pMyUUID[] = { 
    0x3e, 0x4f, 0x5a, 0x6b, 0x7c, 0x8d, 0x9e, 0xaf, 
    0xb0, 0xc1, 0xd2, 0xe3, 0xf4, 0xa5, 0xb6, 0xc7 
};

using namespace IOPlugin;

StatusCode g_HandleGetInfo(HostPropertyCollectionRef* p_pProps)
{
    StatusCode err = p_pProps->SetProperty(pIOPropUUID, propTypeUInt8, pMyUUID, 16);
    if (err == errNone)
    {
        err = p_pProps->SetProperty(pIOPropName, propTypeString, "AAC Encoder (FDK)", strlen("AAC Encoder (FDK)"));
    }
    return err;
}

StatusCode g_HandleCreateObj(unsigned char* p_pUUID, ObjectRef* p_ppObj)
{
    if (memcmp(p_pUUID, AACEncoder::s_UUID, 16) == 0)
    {
        *p_ppObj = new AACEncoder();
        return errNone;
    }
    return errUnsupported;
}

StatusCode g_HandlePluginStart()
{
    g_Log(logLevelInfo, "AAC Encoder Plugin (FDK) :: Started");
    return errNone;
}

StatusCode g_HandlePluginTerminate()
{
    g_Log(logLevelInfo, "AAC Encoder Plugin (FDK) :: Terminated");
    return errNone;
}

StatusCode g_ListCodecs(HostListRef* p_pList)
{
    return AACEncoder::s_RegisterCodecs(p_pList);
}

StatusCode g_ListContainers(HostListRef* p_pList)
{
    return errNone;
}

StatusCode g_GetEncoderSettings(unsigned char* p_pUUID, HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList)
{
    if (memcmp(p_pUUID, AACEncoder::s_UUID, 16) == 0)
    {
        return AACEncoder::s_GetEncoderSettings(p_pValues, p_pSettingsList);
    }
    return errNoCodec;
}
