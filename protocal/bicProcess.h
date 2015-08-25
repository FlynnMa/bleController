#ifndef protocalInternal_h
#define protocalInternal_h

#define PROTOCAL_VERSION_MAJOR     0

#define PROTOCAL_VERSION_SUB       1

#define PROTOCAL_VERSION_MODIFY    0

#define PROTOCAL_VERSION_NUM       0


/*! save or load record, each record with maximum 20 bytes
 *  query param : recordID 1byte + lengthToQuery1byte
 *  set   param : recordID 1byte + data nB(n<=16)
 */
#define BIC_CMD_ID_RECORD                   1



#ifdef __cplusplus
extern "C" {
#endif

/*
 * initialize protocal stack
 *
 * @param none
 *
 * @return @ERROR_TYPE
 */
int bicProcessInit(void);

/*
 * @brief Do the protocal level command handler
 *
 * @param[i] pProtocal  instance of ProtocalStatusType
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
                            uint8_t cmd, uint8_t *pData, uint8_t len);

/*!
 * process response the acknowledge
 *
 * @param[i] cmd     command id
 * @param[i] result  error code
 *
 * @return none
 */
void bicProcessAck(uint8_t cmd, uint8_t result);

/*!
 * Dispatch events for high level processing
 *
 */
int32_t bicProcessDispatchEvents(void);

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
int32_t bicReceiveChar(uint8_t ch);

/*
 * This function router data to remote device
 *
 *
 */
void bicRouter(uint8_t ch);



#ifdef __cplusplus
}
#endif


#endif
