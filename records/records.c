#include <stdint.h>
#include <string.h>
#include "ble_flash.h"
#include "trace.h"
#include "protocalTypes.h"
#include "records.h"
#include "pstorage.h"


#define RECORD_MARK               0xfeedbeef

/**@brief Function for performing necessary functions of storing or updating.
 *
 * @param[in] p_dest Destination address where data is stored persistently.
 * @param[in] p_src  Source address containing data to be stored.
 * @param[in] size   Size of data to be stored expressed in bytes. Must be word aligned.
 * @param[in] offset Offset in bytes to be applied when writing to the block.
 *
 * @retval Operation result code.
 */
typedef uint32_t (* storage_operation)(pstorage_handle_t * p_dest,
                                       uint8_t           * p_src,
                                       pstorage_size_t     size,
                                       pstorage_size_t     offset);

static pstorage_handle_t    baseHandle;

volatile static uint8_t     waitingCallBack = false;

typedef struct RecordsType {
    uint8_t data[RECORD_SIZE - 4];
    uint32_t  mark;
}RecordsType;

//RecordsType *pRecordBuff;

/**
 * Dummy callback handler needed by Nordic's pstorage module. This is called
 * after every flash access.
 */
static void recordsCallback(pstorage_handle_t *p_handle,
                                         uint8_t            op_code,
                                         uint32_t           result,
                                         uint8_t           *p_data,
                                         uint32_t           data_len)
{
    /* APP_ERROR_CHECK(result); */
    waitingCallBack = false;
}

pstorage_module_param_t recordConfig = { recordsCallback,
                                                RECORD_SIZE, RECORD_COUNT };


/*
 * initialize record environment
 */
int32_t recordInit(void)
{
    uint32_t ret;

    ret = pstorage_init();
    if (ret)
    {
        Trace("pstorage init failed:%d", ret);
        return ret;
    }

//    Trace("register blksize %d, count%d", recordConfig.block_size,
//              recordConfig.block_count);
    ret = pstorage_register(&recordConfig, &baseHandle);
    if (ret)
    {
        Trace("pstorage register failed:%d", ret);
    }

//    pRecordBuff = malloc(sizeof(RecordsType));
//    pRecordBuff->mark = RECORD_MARK;
    return ret;

}

/*
 * this function erase one page
 *
 * @param[i] recordStartID start id to be erased
 */
int32_t recordClear(uint8_t recordStartID, uint8_t num)
{
    pstorage_handle_t hdl;
    int32_t ret;

    if (recordStartID > RECORD_COUNT)
    {
        return ERROR_INVALID_PARAM;
    }

    if (waitingCallBack)
        return ERROR_BUSY_STAT;

    ret = pstorage_block_identifier_get(&baseHandle, recordStartID, &hdl);
    if (ret)
    {
        Trace("get identifier(%d) failed:%d", recordStartID, ret);
        return ret;
    }

    ret = pstorage_clear(&hdl, RECORD_SIZE * num);
    if (ret)
    {
        Trace("clear failed:%d", ret);
        return ret;
    }
    waitingCallBack = true;
    while(waitingCallBack == true);

    return ret;
}

/*!
 *  save record into ble flash
 *
 * @param[i] recordID  recordID, 0 ~ 39
 * @param[i] pData   data content
 * @param[i] size    size of pData, maximum 24bytes
 *
 * @return error code
 */
int32_t recordSave(uint8_t recordID,
                        uint8_t *pData, uint8_t size)
{

    int32_t ret;
    pstorage_handle_t hdl;
    uint32_t count = 0;
    uint8_t len;
    storage_operation storeFunc;

    if ((recordID >= RECORD_COUNT) || (size > RECORD_SIZE))
    {
        return ERROR_INVALID_PARAM;
    }

    if (waitingCallBack)
    {
        return ERROR_BUSY_STAT;
    }

    ret = pstorage_block_identifier_get(&baseHandle,
                        (pstorage_size_t)recordID, &hdl);
    if (ret)
    {
        Trace("Identifier get failed:%d", ret);
        return ret;
    }


    Trace("record:%d", recordID);
    ret = pstorage_clear(&hdl, RECORD_SIZE);
    if (ret)
    {
        Trace("clear failed:%d", ret);
        return ret;
    }
    waitingCallBack = true;
    while(waitingCallBack == true);
/*
    ret = pstorage_load(pRecordBuff, &hdl, size,0);
    if (ret)
    {
        Trace("pstorage load failed:%d", ret);
    }*/
/*
    len = sizeof(pRecordBuff->data);
    len = size > len ? size : len;
    memcpy(pRecordBuff->data, pData, len); */
    Trace("store %x(%d), %d-%d", pData, size, hdl.block_id, hdl.module_id);
    ret = pstorage_store(&hdl, pData /*pRecordBuff*/, size, 0);
    if (ret)
    {
        Trace("store data failed:%d", ret);
    }

    waitingCallBack = true;
    while(waitingCallBack == true);
    return ret;
}

/*!
 * load record from ble flash
 *
 * @param[i] recordID  recordID, 0 ~ 39
 * @param[i] pData   data of buffer
 * @param[i] size    size of pData, less than 24bytes
 *
 * @return error code
 */
int32_t recordLoad(uint8_t recordID, uint8_t *pData, uint8_t size)
{
    int32_t ret;
    pstorage_handle_t hdl;

    if ((recordID >= RECORD_COUNT) || (size > RECORD_SIZE))
    {
        return ERROR_INVALID_PARAM;
    }
    ret = pstorage_block_identifier_get(&baseHandle, recordID, &hdl);
    if (ret)
    {
        Trace("Identifier get failed:%d", ret);
        return ret;
    }


    ret = pstorage_load(pData, &hdl, size,0);
    if (ret)
    {
        Trace("pstorage load failed:%d", ret);
    }
    return ret;
}




#if 0
__I  uint32_t  RESERVED0[4];
__I  uint32_t  CODEPAGESIZE;                      /*!< Code memory page size in bytes.                                       */
__I  uint32_t  CODESIZE;                          /*!< Code memory size in pages.                                            */
__I  uint32_t  RESERVED1[4];
__I  uint32_t  CLENR0;                            /*!< Length of code region 0 in bytes.                                     */
__I  uint32_t  PPFC;                              /*!< Pre-programmed factory code present.                                  */
__I  uint32_t  RESERVED2;
__I  uint32_t  NUMRAMBLOCK;                       /*!< Number of individualy controllable RAM blocks.                        */

union {
  __I  uint32_t  SIZERAMBLOCK[4];                 /*!< Deprecated array of size of RAM block in bytes. This name is
                                                       kept for backward compatinility purposes. Use SIZERAMBLOCKS
                                                        instead.                                                             */
  __I  uint32_t  SIZERAMBLOCKS;                   /*!< Size of RAM blocks in bytes.                                          */
};
__I  uint32_t  RESERVED3[5];
__I  uint32_t  CONFIGID;                          /*!< Configuration identifier.                                             */
__I  uint32_t  DEVICEID[2];                       /*!< Device identifier.                                                    */
__I  uint32_t  RESERVED4[6];
__I  uint32_t  ER[4];                             /*!< Encryption root.                                                      */
__I  uint32_t  IR[4];                             /*!< Identity root.                                                        */
__I  uint32_t  DEVICEADDRTYPE;                    /*!< Device address type.                                                  */
__I  uint32_t  DEVICEADDR[2];                     /*!< Device address.                                                       */
__I  uint32_t  OVERRIDEEN;                        /*!< Radio calibration override enable.                                    */
__I  uint32_t  NRF_1MBIT[5];                      /*!< Override values for the OVERRIDEn registers in RADIO for NRF_1Mbit
                                                       mode.                                                                 */
__I  uint32_t  RESERVED5[10];
__I  uint32_t  BLE_1MBIT[5];                      /*!< Override values for the OVERRIDEn registers in RADIO for BLE_1Mbit
                                                       mode.                                                                 */
FICR_INFO_Type INFO;                              /*!< Device info                                                           */

#endif

#if 0
__IO uint32_t  CLENR0;                            /*!< Length of code region 0.                                              */
__IO uint32_t  RBPCONF;                           /*!< Readback protection configuration.                                    */
__IO uint32_t  XTALFREQ;                          /*!< Reset value for CLOCK XTALFREQ register.                              */
__I  uint32_t  RESERVED0;
__I  uint32_t  FWID;                              /*!< Firmware ID.                                                          */
__IO uint32_t  BOOTLOADERADDR;                    /*!< Bootloader start address.*/
#endif

#define FLASH_TEST_SIZE   32
int32_t recordsTest(void)
{
    uint32_t *pData;
    uint8_t   pageNum, *pBuff;
    uint8_t   buff[FLASH_TEST_SIZE], v;
    uint8_t i;
    int32_t ret;

    Trace(">>>>>>>>>>>>>>>>>>>>>>>>>");
    Trace("%s", __TIME__);
    ret = recordInit();
    if (ret)
    {
        return ret;
    }
    Trace("loading...");
    for (i = 0; i < RECORD_COUNT; i++)
    {
        recordLoad(i, buff, RECORD_SIZE);
        if (0 == (i % 8))
        {
            Trace("");
            TracePrint("%d\t:\t", i);
        }
        TracePrint("%d%d, ", buff[0],buff[1]);
    }

    Trace("\r\nsaving record...");
    for (i = 0; i < RECORD_COUNT; i++)
    {
        TracePrint("%d", i);
        buff[i] = buff[i] + 1;
        recordSave(i, &buff[0], 16/4);
    }
    Trace("loading again...");
    memset(buff, 0, FLASH_TEST_SIZE);
    for (i = 0; i < RECORD_COUNT; i++)
    {
        recordLoad(i, buff, 1);
        if (0 == (i % 8))
        {
            Trace("");
            TracePrint("%d\t:\t", i);
        }
        TracePrint("%d, ", buff[0]);
    }
    Trace("\r\n***********************");

    #if 1
    Trace("\r\n\r\n======");
    Trace("code pagesize:%d", NRF_FICR->CODEPAGESIZE);
    Trace("code size:%d", NRF_FICR->CODESIZE);
    Trace("clenr0:%x", NRF_FICR->CLENR0);
    Trace("PPFC:%x", NRF_FICR->PPFC);

    Trace("num ram block:%x", NRF_FICR->NUMRAMBLOCK);
    Trace("config id:%x", NRF_FICR->CONFIGID);
    Trace("device id:%x, %x", NRF_FICR->DEVICEID[0],NRF_FICR->DEVICEID[1]);
    Trace("device addr:%x, %x", NRF_FICR->DEVICEADDR[0], NRF_FICR->DEVICEADDR[1]);

    Trace("bootloader:%x", NRF_UICR->BOOTLOADERADDR);
    Trace("readback protection:%x", NRF_UICR->RBPCONF);
#if 0
    pData = (uint32_t*)FLASH_START_ADDR;
    Trace("before write:%x", *pData);
    ble_flash_word_write((uint32_t*)FLASH_START_ADDR, 0X55AA);

    Trace("after write:%x", *pData);
    //    ble_flash_page_erase(uint8_t page_num);
    pageNum = FLASH_START_ADDR / BLE_FLASH_PAGE_SIZE;
    Trace("erase page:%d", pageNum);
    ble_flash_page_erase(pageNum);
    pData = (uint32_t*)FLASH_START_ADDR;
    Trace("after erase data:%x", *pData);
    ble_flash_word_write((uint32_t*)FLASH_START_ADDR, 0X55AA);

    /* test write block */
    Trace("\r\n==== test write block ====");
    ble_flash_page_erase(pageNum);
    pBuff = (uint8_t*)FLASH_START_ADDR;
    for (i = 0; i < FLASH_TEST_SIZE; i++)
    {
        buff[i] = 56;
    }
    ble_flash_block_write((uint32_t*)FLASH_START_ADDR, buff, FLASH_TEST_SIZE / 4);
    for (i = 0; i < FLASH_TEST_SIZE; i++)
    {
        if (0 == (i % 8))
        {
            Trace("");
            TracePrint("%d\t:\t", i);
        }
        TracePrint("%d,\t",pBuff[i]);
    }

    Trace("\r\n==== test erase block ====");
    ble_flash_page_erase(pageNum);
    for (i = 0; i < FLASH_TEST_SIZE; i++)
    {
        if (0 == (i % 8))
        {
            Trace("");
            TracePrint("%d\t:\t", i);
        }
        TracePrint("%d,\t",pBuff[i]);
    }
    ble_flash_page_addr(pageNum, &pData);
    Trace("page %d, addr:%x", pageNum, pData);
    #endif
    #endif
    return 0;
}

