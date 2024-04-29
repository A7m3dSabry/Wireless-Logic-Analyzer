






#ifndef MAIN_H_
#define MAIN_H_

/**************	Section: Includes ***************************/
#include "stm32f103c8t.h"
#include "mcal_layer.h"
#include "std_types.h"
#include "delay.h"
#include "esp8266.h"
/**************	Section: Macro Definitions Declarations *****/
/**************	Section: Macro Functions Declarations *******/
/**************	Section: Data Types Declarations ************/

/* Struct Indicates the Msg Packet */
typedef struct {
    uint16_t channelNumber;
    uint16_t pinState;
    uint32_t counter;
    uint16_t oldCapture;
    uint16_t newCapture;
} Channel_Msg_t;


/* A Configuration Holding Channels Pins (used to Increase Abstraction in The Code) */
typedef struct {
    uint8_t channelNumber;
    GPIO_PORT_ID_t port;
    GPIO_Pin_ID_t pin;
} ChannelsInputPins_t;



/**************	Section: Methods Declarations  **************/


/* Prototype of Void Handles Capture of Values */
void captureOccured(uint32_t channel);

/* Prototype of Void Handles Sending Detected Edge Packet */
void sendChannelMessage(uint32_t msgIndex);

/* Prototype of Void Handles Sending Current Channel State */
void sendChannelState(uint8_t channel, uint8_t state);

/* Prototype of Void Handles Sending All Channels States */
void sendAllChannelsState();

/* Prototype of Void Handles Timer Overflow Interrupt */
void updateOccured(uint32_t params);

/* Prototype of Void Handles Initializing Detection Channels Input Pins */
void initializeChannelsInputPins();

/* Prototype of Void Handles Initializing MCU Clock (RCC) */
void initializeMCUClock();

/* Prototype of Void Handles Configuring Timer Channels */
void initTimerChannels();

/* Prototype of Void Handles Configuring Timer Options */
void initTimer1();

/* Prototype of Void Handles Configuring USART Communication Peripheral */
void initUSART();

/* Prototype of Void Handles Configuring WiFi Module */
void initWiFiModule();

/* Prototype of Void Handles Looping Until Connection to Network Happen */
void forceConnectToNetwork(ESP8266_ResponseType_t *res);

/* Prototype of Void Handles Looping Until Connection to Server IP Happen */
void forceConnectToServerIP(ESP8266_ResponseType_t *res);

/* Prototype of Void Handles Enabling Interrupts in NVIC */
void enableInterrupts();

/* Prototype of Void Handles Enabling Peripherals Clock */
void enablePeripherals();



#endif /* MAIN_H_ */




