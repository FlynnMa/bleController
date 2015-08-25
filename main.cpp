/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
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

#include <string.h>
#include "mbed.h"
#include "BLE.h"

#include "UARTService.h"
#include "trace.h"
#include "records.h"

#include "bicProcess.h"
#include "list.h"
#define NEED_CONSOLE_OUTPUT 1 /* Set this if you need debug messages on the console;
                               * it will have an impact on code-size and power consumption. */

#if NEED_CONSOLE_OUTPUT
#define DEBUG(STR) { if (uart) uart->write(STR, strlen(STR)); }
#else
#define DEBUG(...) /* nothing */
#endif /* #if NEED_CONSOLE_OUTPUT */

typedef struct BTSendStackType {
    uint8_t buff[32]; /*32 is the max package length */
    uint8_t len;
} BTSendStackType;

BLEDevice  ble;

DigitalOut *led1;

Ticker *tick;

UARTService *uart;

list_t *pList;

Serial *serialPort;//(USBTX, USBRX);//p0, p1);  // TX pin, RX pin
volatile uint8_t bicProcessDispatch;
volatile uint8_t isBTSending;
extern volatile uint8_t g_isConnected;

uint8_t   *pDevBuffer; /* data received from device */
int       devBufferLength;

uint8_t  isConnected;
Ticker *ticker;

extern "C"
{
    int32_t bleSendData(uint8_t *pData, uint32_t len);
}

uint8_t ledToggle = 0;
void periodicCallback(){

    ledToggle = 1;
//    (*led1) = !(*led1);
}


void disconnectionCallback(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    isConnected = 0;
    (*led1) = 0;

    ticker->detach();
    ticker->attach(periodicCallback, 0.4f); // blink LED every second

    ble.startAdvertising();
}

void connectionEventCallback(const Gap::ConnectionCallbackParams_t *params)
{
    ticker->detach();
    ticker->attach(periodicCallback, 1); // blink LED every second
    isConnected = 1;
}


void onDataSent(unsigned num)
{
    uint32_t count;
    BTSendStackType *pBTSend;

  //  led1 = !led1;
    isBTSending = 0;
//    serialPort->printf("sent%d\n\r", num);
    /*
    listCount(pList, &count);
    if (count == 0)
    {
        return;
    }

    listGetAt(pList, 0, (uint32_t*)&pBTSend);
    listRemoveData(pList,(uint32_t) pBTSend);
    pBTSend->buff[0] = '!';
    bleSendData(pBTSend->buff, pBTSend->len);
    free(pBTSend);
    */
}

void rxInterrupt(void)
{
    int ch;

    while (serialPort->readable()) {
        ch = (int)serialPort->getc();
        /* here rout the data into stack,
            so the data will be delivered in order*/
        bicRouter((uint8_t)ch);
    }
}

int main(void)
{
    int ret = 0;
    uint8_t *pTemp;
    int len;
    int ch;

//    led1 = new DigitalOut(P0_4);
    led1 = new DigitalOut(LED2);
     ticker = new Ticker();
    ticker->attach(periodicCallback, 0.4f); // blink LED
    serialPort = new Serial(USBTX, USBRX);
//    serialPort = new Serial(P0_0, P0_1);
    (*led1) = 1;
    serialPort->baud(115200);
    serialPort->attach(&rxInterrupt,SerialBase::RxIrq);

//    serialPort->printf("Yeth sport\r\n");
    wait_ms(100);
    ret = bicProcessInit();
    if (ret)
    {
        serialPort->printf("bic process init failed:%d\r\n", ret);
        while(1);
    }
//    serialPort->printf("%s\r\n", __TIME__);
    ble.init();
    ble.onDisconnection(disconnectionCallback);
    ble.onConnection(connectionEventCallback);

    uart = new UARTService(ble);

    recordInit();

    /* setup advertising */
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"ysport", sizeof("ysport") - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));

    ble.setAdvertisingInterval(50); /* 100ms; in multiples of 0.625ms. */
    ble.startAdvertising();

    while (true) {
        ble.waitForEvent();
        ch = EOF;
        if (ledToggle) {
            (*led1) = !(*led1);
            ledToggle = 0;
        }
        while(1)
        {
            ch = uart->_getc();
            if (ch == EOF)
                break;
            serialPort->putc(ch);
            bicReceiveChar((uint8_t)ch);
        }

        while(0 == bicProcessDispatchEvents()){
            wait_ms(20);
        }
    }
}

extern "C"
{

void flashLed(int num)
{
    switch(num)
    {
        case 0:
            break;

        default:
            break;
    }
}

int32_t bleSendData(uint8_t *pData, uint32_t len)
{
    uint32_t maxLen = 20;
    uint32_t sendLen, copyLen;
    uint32_t count;

    count = 0;
    while(isBTSending){
        wait_us(10);
        if (count++ >= 400)
            break;
    }

    /* if over length, make division, and push remaining to stack */
    if (len > maxLen)
    {
        sendLen = maxLen;
        copyLen = len - maxLen;
    }
    else
    {
        sendLen = len;
        copyLen = 0;
    }

    isBTSending = 1;
    uart->write(pData, (size_t)sendLen);
//    uartSendData(pData, (size_t)sendLen);

    count = 0;
    while(isBTSending){
        wait_us(10);
        if (count++ >= 400)
            break;
    }

    isBTSending = 1;
    uart->write(&pData[sendLen], (size_t)copyLen);
//    uartSendData(pData, (size_t)sendLen);

    return 0;
}

int32_t uartSendData(void *pData, uint32_t len)
{
//    if (uart) uart->write(pData, (size_t)len);
//    printf(pData, (size_t)len);
    uint32_t i;
    uint8_t *ptr = (uint8_t*)pData;

    for (i = 0; i < len; i++)
    {
        serialPort->putc(ptr[i]);
    }

    return i;
}


void bleSChedule(void)
{
    if (bicProcessDispatch)
        return;
    bicProcessDispatch = 1;
    __SEV();
}
}


