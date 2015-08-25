#ifndef records_h
#define records_h


#define RECORD_SIZE               16

#define FLASH_AREA_SPACE          1024

#define RECORD_COUNT              (FLASH_AREA_SPACE / RECORD_SIZE - 4)


#ifdef __cplusplus
extern "C" {
#endif

int32_t recordsTest(void);

/*
 * initialize record environment
 */
int32_t recordInit(void);

/*
 * this function erase one page
 *
 * @param[i] pageID to be erased
 */
int32_t recordClear(uint8_t recordStartID, uint8_t num);

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
                        uint8_t *pData, uint8_t size);

/*!
 * load record from ble flash
 *
 * @param[i] recordID  recordID, 0 ~ 39
 * @param[i] pData   data of buffer
 * @param[i] size    size of pData, less than 24bytes
 *
 * @return error code
 */
int32_t recordLoad(uint8_t recordID, uint8_t *pData, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif
