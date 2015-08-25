#include "protocal.h"
#include "protocalApi.h"
#include "bicProcess.h"
#include "records.h"
#include "trace.h"

static const uint8_t version[4] = {PROTOCAL_VERSION_MAJOR,
        PROTOCAL_VERSION_SUB, PROTOCAL_VERSION_MODIFY, PROTOCAL_VERSION_NUM};

static const char copyright[] = "Ma Yunfei @ 2015";

static const char date[] = __DATE__;

uint8_t          g_isConnected;

extern void bleSChedule(void);
extern int32_t uartSendData(void *pData, uint32_t len);
extern int32_t bleSendData(uint8_t *pData, uint32_t len);
static void waitEmpty(void)
{}

/*
 * initialize protocal stack
 *
 * @param none
 *
 * @return @ERROR_TYPE
 */
int bicProcessInit(void)
{
    g_isConnected = 0;
    return protocalApiInit(bleSendData, bicProcessCmd,
        bicProcessAck, waitEmpty, bleSChedule);
}


static int32_t onSet(uint8_t cmd, uint8_t *pData,
    uint8_t len)
{
    int32_t ret = 0;
    uint32_t buff[RECORD_SIZE], size = RECORD_SIZE;

    switch(cmd)
    {
        case CMD_ID_RESET_TARGET:
            break;

        case CMD_ID_DEVICE_CONNECTION:
            g_isConnected = pData[0];
            break;

        case BIC_CMD_ID_RECORD:
            size = size > len ? len : size;
            memcpy(buff, &pData[1], size);
            ret = recordSave(pData[0], buff, len);
            break;

        default:
            break;
    }
    return ret;
}

static int32_t onQuery(uint8_t cmd,
                    uint8_t *pData, uint8_t len)
{
    int32_t ret = 0;
    uint8_t size, idx;
    uint8_t buff[24];

    switch(cmd)
    {
        case CMD_ID_PROTOCAL_VERSION:
            protocalApiSetU32(cmd, *(uint32_t*)version);
            ret = 1;
            break;

        case CMD_ID_PROTOCAL_COPYRIGHT:
            protocalApiSetStr(cmd, copyright);
            ret = 1;
            break;

        case CMD_ID_PROTOCAL_DATETIME:
            protocalApiSetStr(cmd, date);
            ret = 1;
            break;


        case CMD_ID_DEVICE_CONNECTION:
            g_isConnected = 1;
            protocalApiSetU8(cmd, g_isConnected);
            printf("get connection:%d", g_isConnected);
            break;

        case BIC_CMD_ID_RECORD:
            idx = pData[0];
            size = pData[1];
            ret = recordLoad(idx, buff, size);
            protocalApiSetData(cmd,buff, size);
            break;

        default:
            break;
    }
    return ret;
}

/*
 * @brief Do the protocal level command handler
 * Note: this function does hiden process in embeded target, mobile phone can
 * get those information via commands to identify it.
 *
 * @param[i] cmdType    command type
 * @param[i] cmd        command id
 * @param[i] pData      data to be handled
 * @param[i] len        length of pData context
 *
 * @return
 *  0 : continue process
 *  1 : this is an internal command, no needer further process
 */
int32_t bicProcessCmd(uint8_t cmdType,
            uint8_t cmd, uint8_t *pData, uint8_t len)
{
    uint32_t ret = 0;

    Trace("type:%d, cmd:%d, len:%d", cmdType, cmd, len);
    switch(cmdType)
    {
        case CMD_TYPE_ACK:
            break;

        case CMD_TYPE_QUERY:
            ret = onQuery(cmd, pData, len);
            break;

        case CMD_TYPE_SET:
            ret = onSet(cmd, pData, len);
            break;

        default:
            break;
    }

    return ret;
}

/*!
 * process response the acknowledge
 *
 * @param[i] cmd     command id
 * @param[i] result  error code
 *
 * @return none
 */
void bicProcessAck(uint8_t cmd, uint8_t result)
{
    if (result == ERROR_CHECKSUM)
    {
        protocalApiResendLastPackage();
    }
}

/*!
 * Dispatch events for high level processing
 *
 * @param none
 *
 * @return ERROR_TYPE
 */
int32_t bicProcessDispatchEvents(void)
{
    return protocalApiDispatchEvents();
}

/*!
 * This function response characters when data is received, data will be
 * transferred into low level protocal for further analysis.
 * Notice: this function must be invoked in an individual thread.
 * Make sure it will not be blocked by other tasks
 *
 * @param[in] ch
 *
 * @return none
 */
int32_t bicReceiveChar(uint8_t ch)
{
    return protocalApiReceiveChar(ch);
}

/*
 * This function router data to remote device
 *
 *
 */
void bicRouter(uint8_t ch)
{
    protocalApiRouter(ch);
}

