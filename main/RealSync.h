#ifndef REALSYNC_H

#define REALSYNC_H

#include "LoRa.h"

class RealSync{
public:
    /*
    *   @brief
    *   @param priority, ID, GeneralCurrent, Voltage
    */
    void SendPacket1(uint8_t priority, uint8_t MID, float GeneralCurrent, float Voltage);
    /*
    *   @brief
    *   @param priority, ID, Hourmeter
    */
    void SendPacket2(uint8_t priority, uint8_t MID, int Hourmeter);
    /*
    *   @brief
    *   @param priority, ID, Answer1, Answer2, Answer3, Answer4, Answer5, uid_batery, uid_operator
    */
    void SendPacket3(uint8_t priority, uint8_t MID, Answer1, Answer2, Answer3, Answer4, Answer5, uid_batery, uid_operator);


private:
    uint8_t priority;
    uint8_t MID;
    float GeneralCurrent;
    float Voltage;
    int Hourmeter;
    uint8_t Answer1;
    uint8_t Answer2;
    uint8_t Answer3;
    uint8_t Answer4;
    uint8_t Answer5;
    char uid_battery;
    char uid_operator;
}


#endif
