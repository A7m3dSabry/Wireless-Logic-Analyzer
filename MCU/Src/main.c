#include "main.h"


/* Buffer to Hold Old Captures Values */
uint16_t oldCaptures[LOGIC_ANALYZER_NUM_CHANNELS] = {0};

/* Buffer to Hold How Many Times the Timer Had Overflowed in Current Logic Level */
uint32_t counter =0;

/* Buffer Holds Every Channel Last Edge Value */
Timers_CaptureMode_CaptureEdge_t currentEdge[LOGIC_ANALYZER_NUM_CHANNELS];

/* Index Indicates the Last Registered Msg */
uint32_t lastMsgIndex = 0;

/* WiFi Module Config */
ESP8266_Config_t espConfig = {0};

/* Buffer Stores Messages to Be Sent */
Channel_Msg_t msgsBuff[MESSAGES_BUFFER_SIZE] = {0};

/* Array of Struct to Hold Mapping to the Channels Input Pins */
ChannelsInputPins_t channelsInputPins[LOGIC_ANALYZER_NUM_CHANNELS] = {0};



int main() {


    Std_ReturnType ret = E_OK;
    ESP8266_ResponseType_t res = ESP8266_Return_OK;

    /* Initialize RCC */
    initializeMCUClock();

    /* Enable RCC Peripherals */
    enablePeripherals();

    /* Config USART to Communicate with WiFi Module */
    initUSART();

    /* Config WiFi Module */
    initWiFiModule();

    /* Loop Until Join The Network */
    forceConnectToNetwork(&res);

    /* Loop Until Connection Started */
    forceConnectToServerIP(&res);

    /* Config Input Pins to Logic Analyzer */
    initializeChannelsInputPins();

    /* Send Current States On GPIO */
    sendAllChannelsState();

    /* Init Timer Channels */
    initTimerChannels();

    /* Enable Timer1 */
    initTimer1();

    /* Enable TimerCapture and Update Interrupts */
    enableInterrupts();

    /* Counter to Know the Index of the Last Message Sent */
    uint32_t lastSentMsgIndex = 0;

    /* Loop if There is Any Messages to Send */
    while (1) {
        if (lastSentMsgIndex != lastMsgIndex) {
            sendChannelMessage(lastSentMsgIndex);
            lastSentMsgIndex++;
        }
    }
    return ret;
}

void enablePeripherals() {
    /* Enable Peripherals */
    HAL_RCC_ChangePeripheralCLKState(PeripheralID_PORTA, Peripheral_State_ON);
    HAL_RCC_ChangePeripheralCLKState(PeripheralID_USART2, Peripheral_State_ON);
    HAL_RCC_ChangePeripheralCLKState(PeripheralID_TIMER1, Peripheral_State_ON);
}

void enableInterrupts() {
    /* Enabling Interrupts */
    HAL_NVIC_ChangeInterruptState(IRQn_TIM1_CC, InterruptState_Enable);
    HAL_NVIC_ChangeInterruptState(IRQn_TIM1_UP, InterruptState_Enable);
}

void forceConnectToServerIP(ESP8266_ResponseType_t *res) {
    /* Loop Until Connection Started */
    do {

        (*res) = HAL_ESP8266_startUART_WIFI_PassThroughConnection(&espConfig, ESP8266_ProtocolType_TCP, RHOST,
                                                                  RPORT,
                                                                  NULL);
        if ((*res) == ESP8266_Return_ERROR) {
            HAL_Delay_ms(100);
        }
    } while ((*res) == ESP8266_Return_ERROR);
}

void forceConnectToNetwork(ESP8266_ResponseType_t *res) {

    /* Loop Until Join The Network */
    do {
        (*res) = HAL_ESP8266_joinAccessPoint(&espConfig, SSID_NAME, SSID_PASS);
        if ((*res) == ESP8266_Return_ERROR) {
            HAL_Delay_ms(100);
        }
    } while ((*res) == ESP8266_Return_ERROR || (*res) == ESP8266_Return_FAIL);
}

void initWiFiModule() {
    /* Configuring ESP8266 Module */
    espConfig.usartNumber = USART_Number_2;

    espConfig.selectedConfigs = ESP8266_SelectedConfigs_BasicConfigs | ESP8266_SelectedConfigs_WifiConfigs;

    espConfig.basicConfig.echoMode = ESP8266_EchoMode_Disable;
    espConfig.basicConfig.sleepMode = ESP8266_SleepMode_Disable;
    espConfig.uartConfig.flowControl = ESP8266_UART_flowControl_Disable;
    espConfig.uartConfig.parityBits = ESP8266_UART_parityBits_None;
    espConfig.uartConfig.stopBits = ESP8266_UART_stopBits_1;
    espConfig.uartConfig.dataBits = ESP8266_UART_dataBits_8;
    espConfig.wifiConfig.workingMode = ESP8266_Wifi_WorkingMode_StationMode;

    HAL_ESP8266_Init(&espConfig);
}

void initUSART() {
    /* Configuring USART Peripheral */
    USART_Config_t usartConfig = {0};
    usartConfig.mode = USART_Mode_Async;
    usartConfig.BaudRate = 115200;
    usartConfig.BasicConfig.stopBits = USART_StopBits_1Bit;
    usartConfig.BasicConfig.dataDirectionMode = USART_Mode_TransmitterAndReceiver;
    usartConfig.BasicConfig.wordLength = USART_FullPacket_8Bits;

    HAL_USART_Init(USART_Number_2, &usartConfig);
    HAL_USART_GPIOInit(USART_Number_2, &usartConfig);
}

void initTimer1() {

    /* Timer Configurations */
    Timers_A_Config_t timer1Config = {0};

    timer1Config.BasicConfigurations.runImmediately = TIMERS_RunningState_ON;
    timer1Config.BasicConfigurations.countingMode = Timers_A_CountingMode_CountingUp;
    timer1Config.ClockBasicConfigurations.clockSourceMode = Timers_A_ClockSourceMode_InternalClock;
    timer1Config.BasicConfigurations.autoReload = (uint16_t) (~0UL);
    timer1Config.interrupts = Timers_A_Interrupts_UpdateInterrupt | Timers_A_Interrupts_CaptureCompare_1|
                              Timers_A_Interrupts_CaptureCompare_2 |
                              Timers_A_Interrupts_CaptureCompare_3 | Timers_A_Interrupts_CaptureCompare_4;


    /* Capture Frequencies 1 MHZy */
    timer1Config.BasicConfigurations.prescaller = 51;

    /* Register Callback Function For Channel Capture Interrupt */
    HAL_TIMERS_A_registerInterruptCallback(captureOccured, Timers_A_Interrupts_CaptureCompare_1);
    HAL_TIMERS_A_registerInterruptCallback(captureOccured, Timers_A_Interrupts_CaptureCompare_2);
    HAL_TIMERS_A_registerInterruptCallback(captureOccured, Timers_A_Interrupts_CaptureCompare_3);
    HAL_TIMERS_A_registerInterruptCallback(captureOccured, Timers_A_Interrupts_CaptureCompare_4);
    /* Register Call Back for Timer Update Interrupt */
    HAL_TIMERS_A_registerInterruptCallback(updateOccured, Timers_A_Interrupts_UpdateInterrupt);


    /* Enable Timer To Run */
    HAL_TIMERS_A_Init(Timers_A_TimerNumber_1, &timer1Config);
}

void initTimerChannels() {

    /* Config Timer Channels */

    Timers_A_Channel_Config_t channelConfig = {0};
    channelConfig.Options.mode = Timers_A_CaptureCompareSelection_Capture_Same_TIFxP;
    channelConfig.Options.CaptureOptions.inputCaptureFilter = Timers_A_InputCaptureFilterFreq_NoFilter;
    channelConfig.Options.CaptureOptions.inputCapturePrescaller = Timers_A_InputCapturePrescaller_NoPrescalling;

    /* Initialize Edge Based on Current Level */
    Pin_State_t state;

    /* Channel 1 */
    HAL_GPIO_ReadPinValue(channelsInputPins[LOGIC_ANALYZER_CH_1].port,channelsInputPins[LOGIC_ANALYZER_CH_1].pin,&state);
    channelConfig.Options.CaptureOptions.captureEdge = (state == Pin_State_OFF)? Timers_CaptureMode_CaptureEdge_Rising: Timers_CaptureMode_CaptureEdge_Falling ;
    currentEdge[LOGIC_ANALYZER_CH_1] = channelConfig.Options.CaptureOptions.captureEdge;
    HAL_TIMERS_A_ChannelInit(Timers_A_ChannelNumber_1, &channelConfig);

    /* Channel 2 */
    HAL_GPIO_ReadPinValue(channelsInputPins[LOGIC_ANALYZER_CH_2].port,channelsInputPins[LOGIC_ANALYZER_CH_1].pin,&state);
    channelConfig.Options.CaptureOptions.captureEdge = (state == Pin_State_OFF)? Timers_CaptureMode_CaptureEdge_Rising: Timers_CaptureMode_CaptureEdge_Falling ;
    currentEdge[LOGIC_ANALYZER_CH_2] = channelConfig.Options.CaptureOptions.captureEdge;
    HAL_TIMERS_A_ChannelInit(Timers_A_ChannelNumber_2, &channelConfig);

    /* Channel 3 */
    HAL_GPIO_ReadPinValue(channelsInputPins[LOGIC_ANALYZER_CH_1].port,channelsInputPins[LOGIC_ANALYZER_CH_1].pin,&state);
    channelConfig.Options.CaptureOptions.captureEdge = (state == Pin_State_OFF)? Timers_CaptureMode_CaptureEdge_Rising: Timers_CaptureMode_CaptureEdge_Falling ;
    currentEdge[LOGIC_ANALYZER_CH_3] = channelConfig.Options.CaptureOptions.captureEdge;
    HAL_TIMERS_A_ChannelInit(Timers_A_ChannelNumber_3, &channelConfig);

    /* Channel 4 */
    HAL_GPIO_ReadPinValue(channelsInputPins[LOGIC_ANALYZER_CH_1].port,channelsInputPins[LOGIC_ANALYZER_CH_1].pin,&state);
    channelConfig.Options.CaptureOptions.captureEdge = (state == Pin_State_OFF)? Timers_CaptureMode_CaptureEdge_Rising: Timers_CaptureMode_CaptureEdge_Falling ;
    currentEdge[LOGIC_ANALYZER_CH_4] = channelConfig.Options.CaptureOptions.captureEdge;
    HAL_TIMERS_A_ChannelInit(Timers_A_ChannelNumber_4, &channelConfig);

}

void initializeMCUClock() {
    /* Configuring MCU Clock */
    /* CPU_Frequency = 56 MHZ */
    RCC_Config_t rccConfig = {0};
    rccConfig.sys_clk_src = SYS_CLK_SRC_PhaseLockedLoop;
    rccConfig.PLL.src = PLL_SRC_HSI_DIV_BY_2;
    rccConfig.PLL.mul_factor = PLL_MUL_FACTOR_13;
    rccConfig.APB1.prescaller = APB1_PRESCALLER_2;

    /* Init RCC */
    HAL_RCC_Init(&rccConfig);
}

void initializeChannelsInputPins() {

    channelsInputPins[LOGIC_ANALYZER_CH_1].channelNumber = 0;
    channelsInputPins[LOGIC_ANALYZER_CH_1].port = PORT_ID_PORTA;
    channelsInputPins[LOGIC_ANALYZER_CH_1].pin = GPIO_PIN_ID_8;

    channelsInputPins[LOGIC_ANALYZER_CH_2].channelNumber = 0;
    channelsInputPins[LOGIC_ANALYZER_CH_2].port = PORT_ID_PORTA;
    channelsInputPins[LOGIC_ANALYZER_CH_2].pin = GPIO_PIN_ID_9;

    channelsInputPins[LOGIC_ANALYZER_CH_3].channelNumber = 0;
    channelsInputPins[LOGIC_ANALYZER_CH_3].port = PORT_ID_PORTA;
    channelsInputPins[LOGIC_ANALYZER_CH_3].pin = GPIO_PIN_ID_10;

    channelsInputPins[LOGIC_ANALYZER_CH_4].channelNumber = 0;
    channelsInputPins[LOGIC_ANALYZER_CH_4].port = PORT_ID_PORTA;
    channelsInputPins[LOGIC_ANALYZER_CH_4].pin = GPIO_PIN_ID_11;


    /* Initialize Every Channel Input Pin */
    for (uint8_t channelNumber = 0; channelNumber < LOGIC_ANALYZER_NUM_CHANNELS; channelNumber++) {

        /* Config Time Channels as Input */
        HAL_GPIO_ConfigPinsDirect(channelsInputPins[channelNumber].port,
                                  channelsInputPins[channelNumber].pin,
                                  Pin_Mode_Input,
                                  Pin_Mode_Config_INPUT_FLOATING);
    }
}


void sendChannelMessage(uint32_t msgIndex) {

    HAL_ESP8266_connSendBytes(&espConfig, ESP8266_LinkNumber_Default, &packetStart, packetStartSize,
                              ESP8266_UART_WiFi_PassThroughMode_On);


    HAL_ESP8266_connSendBytes(&espConfig, ESP8266_LinkNumber_Default,
                              (uint8_t *) &msgsBuff[msgIndex % MESSAGES_BUFFER_SIZE],
                              channelMsgSize,
                              ESP8266_UART_WiFi_PassThroughMode_On);


}


void sendAllChannelsState() {

    Pin_State_t state;


    for (uint8_t channelNumber = 0; channelNumber < LOGIC_ANALYZER_NUM_CHANNELS; channelNumber++) {
        /* Since Only 4 Channels in Port A then read it  */
        HAL_GPIO_ReadPinValue(channelsInputPins[channelNumber].port, channelsInputPins[channelNumber].pin, &state);
        /* Send State */
        sendChannelState(channelNumber, state);

    }


}

struct {
    uint8_t channel;
    uint8_t state;
} channelUpdateMsg;

void sendChannelState(uint8_t channel, uint8_t state) {

    channelUpdateMsg.channel = channel;
    channelUpdateMsg.state = state;


    /* Sending Packet Header */
    HAL_ESP8266_connSendBytes(&espConfig, ESP8266_LinkNumber_Default, &packetChannelsUpdate, packetChannelsUpdateSize,
                              ESP8266_UART_WiFi_PassThroughMode_On);


    /* Sending Msg */
    HAL_ESP8266_connSendBytes(&espConfig, ESP8266_LinkNumber_Default,
                              (uint8_t *) &channelUpdateMsg, sizeof(channelUpdateMsg),
                              ESP8266_UART_WiFi_PassThroughMode_On);

}

void captureOccured(uint32_t channel) {

    /* Getting Pointer to The Current Empty Channel Message */
    Channel_Msg_t *currentMsg = &msgsBuff[lastMsgIndex % MESSAGES_BUFFER_SIZE];

    /* Filling The Message Params */
    currentMsg->channelNumber = channel;
    currentMsg->pinState = currentEdge[channel];
    currentMsg->counter = counter;


    HAL_TIMERS_ChannelCapture_readCaptureValue_Fast(channel, &currentMsg->newCapture);

    /* Restart the Channel Counter */
    counter = 0;


    /* Flip Current Edge Detection */
    currentEdge[channel] = (~currentEdge[channel]) & 0x1;
    HAL_TIMERS_ChannelCapture_setCaptureEdge_Fast(channel, currentEdge[channel]);

    /* Store Old Capture Value */
    currentMsg->oldCapture = oldCaptures[channel];
    oldCaptures[channel] = currentMsg->newCapture;
    /* Increase Current Message Index */
    lastMsgIndex++;

}


void updateOccured(uint32_t params) {
	counter++;

}


