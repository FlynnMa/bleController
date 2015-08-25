#include <stdlib.h>
#include <string.h>
#include "protocalTypes.h"
#include "protocal.h"
#include "protocalApi.h"
#include "trace.h"

static const uint8_t headerMagic[4] =
{
    64,27,103,119
};

static const uint8_t magicDevice[4] =
{
    203,102,124,114
};


static int32_t protocalAllocEmptyEventMemory(
        ProtocalStatusType *pProtocal, EventStackType **ppEvent);

/*!
 * Initialize protocal

*
 * @param none
 *
 * @return @ERROR_TYPE
 */
int32_t protocalInit(ProtocalStatusType *pProtocal)
{
    memset(pProtocal, 0, sizeof(ProtocalStatusType));
    pProtocal->bufferIndex = 0;
    pProtocal->connectionStatus = 0;
    pProtocal->checkSum = 0;
    pProtocal->cmd = CMD_ID_INVALID;
    return 0;
}

/*!
 * @brief make data package
 *
 * @param[in] cmd               command id
 * @param[in] pData             arguments of cmd
 * @param[in] lengthOfData      length of @pData
 *
 * @return none
 */
void protocalMakePackage(ProtocalStatusType *pProtocal, uint8_t cmdType,
        uint8_t cmd, const void *pData, uint8_t lengthOfData)
{
    uint8_t checkSum = 0;
    uint8_t i;

    /* add header magic to indicate a new frame start */
    pProtocal->package[0] = headerMagic[0];
    pProtocal->package[1] = headerMagic[1];
    pProtocal->package[2] = headerMagic[2];
    pProtocal->package[3] = headerMagic[3];

    pProtocal->package[4] = cmdType;
    pProtocal->package[5] = lengthOfData + 2;
    pProtocal->package[6] = cmd;
    if (NULL != pData)
    {
        memcpy(&pProtocal->package[7], pData, lengthOfData);
    }

    /* calculate checksum without header */
    for(i = 4; i < lengthOfData + 7; i++)
    {
        checkSum += pProtocal->package[i];
    }
    pProtocal->package[lengthOfData + 7] = checkSum;
    pProtocal->packageSize = lengthOfData + 8;
}

void protocalRouterPackage(ProtocalStatusType *pProtocal,
    void *pkg, int32_t pkgLen)
{
    EventStackType *pEvent;
    int32_t         ret;

    ret = protocalAllocEmptyEventMemory(pProtocal, &pEvent);
    if (ret)
    {
        return;
    }

    memcpy(pEvent->buff, pkg, pkgLen);
    pEvent->cmdType = CMD_TYPE_ROUTER;
    pEvent->buffSize = pkgLen;
    listAdd(&pProtocal->eventStack, (uint32_t)pEvent);
}

/*!
 * Ackknoledge to the host or slave
 *
 * @param[in] result
 *
 * @return none
 */
void protocalAck(ProtocalStatusType *pProtocal, uint8_t result)
{
    protocalMakePackage(pProtocal, CMD_TYPE_ACK, pProtocal->cmd, &result, 1);
    if (NULL != pProtocal->pfnSendData)
    {
        pProtocal->pfnSendData(pProtocal->package,
            pProtocal->packageSize);
    }
}

/*!
 * @brief send command to host or slave
 *
 * @param[in] cmd               command id
 * @param[in] pData             arguments of cmd
 * @param[in] lengthOfData      length of @pData
 *
 * @return @ERROR_TYPE
 */
int protocalSendCmd(ProtocalStatusType *pProtocal, uint8_t cmdType,
    uint8_t cmd, const void *pData, uint8_t lengthOfData)
{
    if (pProtocal->waitingStat)
    {
        return ERROR_BUSY_STAT;
    }

    protocalMakePackage(pProtocal, cmdType, cmd, pData, lengthOfData);
    if (pProtocal->pfnSendData)
    {
        pProtocal->pfnSendData(pProtocal->package, pProtocal->packageSize);
    }

    return EV_SUCCESS;
}

/*!
 * Send again last package
 *
 * @param[i] pProtocal an instance of ProtocalStatusType
 *
 * @return none
 */
void protocalResendLastPackage(ProtocalStatusType *pProtocal)
{
    if (pProtocal->pfnSendData)
    {
        pProtocal->pfnSendData(pProtocal->package, pProtocal->packageSize);
    }
}

/*!
 * @brief send command to host or slave, and wait until the response
 *
 * @param[in] cmd               command id
 * @param[in] pData             arguments of cmd
 * @param[in] lengthOfData      length of @pData
 *
 * @return @ERROR_TYPE
 */
int32_t protocalSendCmdWait(ProtocalStatusType *pProtocal, uint8_t cmdType,
    uint8_t cmd, void *pData, uint8_t lengthOfData)
{
    int count = 0;
    protocalMakePackage(pProtocal, cmdType, cmd, pData, lengthOfData);
    if (pProtocal->pfnSendData)
    {
        pProtocal->waitingStat |= PROTOCAL_WAITING_ACK;
        pProtocal->pfnSendData(pProtocal->package, pProtocal->packageSize);
    }

    if (NULL == pProtocal->pfnWait)
    {
        return EV_SUCCESS;
    }

    while(pProtocal->waitingStat & PROTOCAL_WAITING_ACK)
    {
        if (pProtocal->pfnWait != NULL)
            pProtocal->pfnWait();
        if (count++ > PROTOCAL_WAIT_TIMES)
        {
            pProtocal->waitingStat &= (~PROTOCAL_WAITING_ACK);
            protocalReset(pProtocal);
            return ERROR_TIME_OUT;
        }
    }
    return (int32_t)pProtocal->replyErr;

}

/*!
 * Set a callback function for responsing commands
 *
 * @param[in] pfnOnCmd
 *
 */
void protocalSetOnCmdCallback(ProtocalStatusType *pProtocal, pfnProtocalOnCmd pfnOnCmd)
{
    pProtocal->pfnOnCmd = pfnOnCmd;
}

/*!
 * set a callback function on responsing acknlodgement
 *
 * @param[in] pProtocal instance of protocal
 * @param[in] ackFunc   acknowledge callback function
 *
 * @return none
 */
void protocalSetOnAckCallback(ProtocalStatusType *pProtocal,
 pfnAck ackFunc)
{
    pProtocal->pfnAck = ackFunc;
}

/*!
 * Set a callback function on waiting response
 *
 * @param[in] pProtocal  instance of @ProtocalStatusType
 * @param[in] waitFunc   wait callback function
 *
 * @return none
 */
void protocalSetWaitFunc(ProtocalStatusType *pProtocal,
    pfnWait waitFunc)
{
    pProtocal->pfnWait = waitFunc;
}

/*!
 * Set a callback function to schedule the event handler, while protocal receive
 * an event, it must be processed in an individual task or thread
 *
 * @param[in] pProtocal      instance of @ProtocalStatusType
 * @param[in] scheduleFunc   schedule callback function
 *
 * @return none
 */
void protocalSetScheduleFunc(ProtocalStatusType *pProtocal,
    pfnScheduleDispatch scheduleFunc)
{
    pProtocal->pfnSchedule = scheduleFunc;
}


/*!
 * Set a callback function for sending data
 *
 * @param[in] pfnOnCmd
 *
 * @return none
 */
void protocalSetSendDataFunc(ProtocalStatusType *pProtocal, pfnSendData pfnSend)
{
    pProtocal->pfnSendData = pfnSend;
}

/*!
 * This function alloc memory for pushing asynchoronous events, the reason why
 * pushing events to be handled asynchoronous is because we don't want the process
 * will impact the input of characters. The asynchoronous mechanism allows
 * whatever the process doing, the receiving part will never be impacted
 *
 * @param[i] pProtocal instance of @ProtocalStatusType
 * @param[o] ppEvent   event memory allocated
 *
 * @return @ERROR_TYPE
 */
static int32_t protocalAllocEmptyEventMemory(
        ProtocalStatusType *pProtocal, EventStackType **ppEvent)
{
    EventStackType *pstack = pProtocal->eventPool;
    int i;

    for (i = 0; i < PROTOCAL_MAX_EVENT_NUM; i++)
    {
        if (pstack[i].cmdType == 0)
        {
            break;
        }
    }

    if (i >= PROTOCAL_MAX_EVENT_NUM)
    {
        return ERROR_NO_RESOURCE;
    }

    pstack[i].cmdType |= 0x80;
    *ppEvent = &pstack[i];
    return EV_SUCCESS;
}

/*
 * This function check whether header is received, if header patent is matching
 * it will reset the protocal status and start to receiving new frame data
 *
 * @param[i] pProtocal  an instance of @ProtocalStatusType
 * @param[i] ch         new character
 *
 * @return true or false
 */
int32_t protocalCheckHeader(ProtocalStatusType *pProtocal, uint8_t ch)
{
    uint8_t *pHeader = pProtocal->header;

    /* move and insert new data */
    pHeader[0] = pHeader[1];
    pHeader[1] = pHeader[2];
    pHeader[2] = pHeader[3];
    pHeader[3] = ch;

    if (memcmp(pHeader, headerMagic, 4) == 0)
    {
//        Trace("header:%x, on char:%x", *(uint32_t*)pHeader, ch);
        /* Header hit, reset receiving status, restart receiving new frame */
        pProtocal->protocalStatus = PROTOCAL_STATUS_CMD_TYPE;
        pProtocal->bufferIndex = 0;
        pProtocal->checkSum = 0;
        pProtocal->cmd = CMD_ID_INVALID;

        /* return true to stop process this character, to prevent additional
        status change */
        return 1;
    }

    /* no header deteced, continue further processing */
    return 0;
}


static int32_t protocalPushEvent(ProtocalStatusType *pProtocal)
{
    int32_t ret;
    EventStackType *pEvent;
    int copyLen;

    ret = protocalAllocEmptyEventMemory(pProtocal, &pEvent);
    if (ret)
    {
        Trace("error, no memory!");
        return ret;
    }
    pEvent->cmd = pProtocal->cmd;
    pEvent->cmdType |= pProtocal->cmdType;
    pEvent->buffSize = pProtocal->bufferIndex;
    copyLen = pProtocal->bufferIndex < PROTOCAL_MAX_DATA_LENGTH ?
              pProtocal->bufferIndex : PROTOCAL_MAX_DATA_LENGTH;
    memcpy(pEvent->buff, pProtocal->buffer, copyLen);
    ret = listAdd(&pProtocal->eventStack, (uint32_t)pEvent);
    if (pProtocal->pfnSchedule)
    {
        pProtocal->pfnSchedule();
    }

}

/*!
 * Response for receiving character, and input to protocal analysis
 *
 * @param[in] ch character input into protocal
 *
 * @return @ERROR_TYPE
 */
int32_t protocalUartReceiveChar(ProtocalStatusType *pProtocal,
                                            uint8_t ch)
{
    int32_t ret = EV_SUCCESS;
    EventStackType *pEvent;
    int copyLen;
    int32_t stop;

//    Trace("status:%d, data:%d", pProtocal->protocalStatus, ch);
    ret = protocalCheckHeader(pProtocal, ch);
    /* if header has been detected, switch status to cmdtype and return */
    if (ret)
    {
        return EV_SUCCESS;
    }
    switch(pProtocal->protocalStatus)
    {
        /* while checking header is always a default behavior,
            just keep a status*/
        case PROTOCAL_STATUS_HEADER:
            break;

        case PROTOCAL_STATUS_CMD_TYPE:
            pProtocal->cmdType = ch;
            /* set command type and update status */
            pProtocal->checkSum = 0;
            pProtocal->protocalStatus = PROTOCAL_STATUS_LENGTH;
            pProtocal->bufferIndex = 0;
            pProtocal->cmd = CMD_ID_INVALID;
            break;

        case PROTOCAL_STATUS_LENGTH:
            /* set buffer length and update status */
            pProtocal->cmdSize = ch;
            pProtocal->protocalStatus = PROTOCAL_STATUS_CMD;
            break;

        case PROTOCAL_STATUS_CMD:
            /* set command type and update status */
            pProtocal->cmd = ch;
            pProtocal->protocalStatus = PROTOCAL_STATUS_DATA;
            if (pProtocal->cmdSize == 2) /* if no data */
            {
                pProtocal->protocalStatus = PROTOCAL_STATUS_CHECKSUM;
            }
            break;

        case PROTOCAL_STATUS_DATA:
            /* fill buffer */
            pProtocal->buffer[pProtocal->bufferIndex++] = ch;

            /* command size contain cmd(1byte) + nData + chksum(1byte) */
            if ((pProtocal->bufferIndex + 2) == pProtocal->cmdSize)
            {
                pProtocal->protocalStatus = PROTOCAL_STATUS_CHECKSUM;
            }
            break;

          case PROTOCAL_STATUS_CHECKSUM:
            pProtocal->protocalStatus = PROTOCAL_STATUS_CMD_TYPE;

            /* Get checksum*/
            pProtocal->checkSumRecv = ch;
            /* verify checkSum */
            if (pProtocal->checkSumRecv != pProtocal->checkSum)
            {
                /* if checksum mismatch, notify remote side the error
                    and discard the package */
                protocalAck(pProtocal, ERROR_CHECKSUM);
                protocalReset(pProtocal);
                return ERROR_CHECKSUM;
            }

            if ((pProtocal->cmdType != CMD_TYPE_ACK) && ret)
            {
                protocalAck(pProtocal, (uint8_t)ret);
            }

            /* if customer is waiting the answer, return immediately */
            if ((pProtocal->waitingStat  & PROTOCAL_WAITING_ACK) &&
                (pProtocal->cmdType == CMD_TYPE_ACK))
            {
                pProtocal->waitingStat &= (~PROTOCAL_WAITING_ACK);
                pProtocal->replyErr = pProtocal->buffer[0];
                protocalReset(pProtocal);
                break;
            }
            else if ((pProtocal->waitingStat & PROTOCAL_WAITING_QUERY) &&
                      (pProtocal->cmdType == CMD_TYPE_SET))
            {
                pProtocal->waitingStat &= (~PROTOCAL_WAITING_QUERY);
                pProtocal->replyLen = pProtocal->bufferIndex;
                protocalReset(pProtocal);
                break;
            }

            protocalPushEvent(pProtocal);

            protocalReset(pProtocal);
            break;

          default:
            break;
    }

    pProtocal->checkSum += ch;
    return ret;
}

/*!
 * This function will invoke corresponding handlers
 * which was registerred by uplayer
 *
 * @param[i] pProtocal an instance of @ProtocalStatusType
 *
 * @return ERROR_TYPE
*/
int32_t protocalDispatchEvents(ProtocalStatusType *pProtocal)
{
    uint32_t count;
    int32_t ret;
    EventStackType *pEvent;
    uint8_t cmdType;

    (void)listCount(&pProtocal->eventStack, &count);
    if (count == 0)
    {
        return ERROR_NO_RESOURCE;
    }

    /* pop event from stack */
    ret = listGetAt(&pProtocal->eventStack, 0, (uint32_t *)&pEvent);
    if (ret)
        return ret;

    listRemoveData(&pProtocal->eventStack, (uint32_t)pEvent);

    /* Get command type and reset cmdType to release memory */
    cmdType = pEvent->cmdType & 0x7f;
    switch(cmdType)
    {
        case CMD_TYPE_ACK:
            if (pProtocal->pfnAck)
            {
                pProtocal->pfnAck(pEvent->cmd, pEvent->buff[0]);
            }
            break;

        case CMD_TYPE_SET:
        case CMD_TYPE_QUERY:
            if (pProtocal->pfnOnCmd)
            {
                ret = pProtocal->pfnOnCmd(cmdType, pEvent->cmd,
                    pEvent->buff, pEvent->buffSize);
            }
            break;

        case CMD_TYPE_ROUTER:
            if (pProtocal->pfnSendData)
            {
                ret = pProtocal->pfnSendData(pEvent->buff, pEvent->buffSize);
            }
            break;

        default:
            break;
    }

    /* release the memory */
    pEvent->cmdType = 0;

    return ret;
}

/*!
 * reset protocal status
 *
 * @param[in] pProtocal  instance of protocal
 *
 * @return none
 */
void protocalReset(ProtocalStatusType *pProtocal)
{
    pProtocal->protocalStatus = PROTOCAL_STATUS_HEADER;
    pProtocal->bufferIndex = 0;
    pProtocal->checkSum = 0;
    pProtocal->cmd = CMD_ID_INVALID;
}

/*
 * This function router data to remote device
 *
 *
 */
void protocalRouter(ProtocalStatusType *pProtocal,
    uint8_t ch)
{
    static uint8_t header[4];
    static uint8_t pkg[42];
    static uint8_t idx = 0, pkgSize;
    EventStackType *pEvent;
    int32_t copyLen;
    int32_t ret;

    #define ROUTER_STATE_HEADER    0
    #define ROUTER_STATE_PACKAGE   1

    /* move and insert new data */
    header[0] = header[1];
    header[1] = header[2];
    header[2] = header[3];
    header[3] = ch;

    if (memcmp(header, magicDevice, 4) == 0)
    {
        memcpy(&pkg[0], header, 4);
        idx = 4;
        return;
    }

    pkg[idx] = ch;
    if (idx == 5)
    {
        pkgSize = ch;
    }
    else if (idx == (pkgSize + 5)) /* data size + 4 header + 1 len*/
    {
        protocalRouterPackage(pProtocal, pkg, idx + 1);
    }
    idx++;

}


