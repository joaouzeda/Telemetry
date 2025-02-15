/*
*   @file RealSybnc.cpp
*   @author João Uzêda
*   @date 2025-01-30
*
*
*/

#include "RealSync.h"

void RealSync::SendPacket1(uint8_t priority, uint8_t MID, float GeneralCurrent, float Voltage){
    LoRa.beginPacket();
    LoRa.print(priority);
    LoRa.print(" ");
    LoRa.print(MID);
    LoRa.print(" ");
    LoRa.print(GeneralCurrent);
    LoRa.print(" ");
    LoRa.print(Voltage);
    LoRa.print(" ");
    LoRa.endPacket();
}

void RealSync::SendPacket2(uint8_t priority, uint8_t MID, int Hourmeter){
    LoRa.beginPacket();
    LoRa.print(priority);
    LoRa.print(" ");
    LoRa.print(MID);
    LoRa.print(" ");
    LoRa.print(Hourmeter);
    LoRa.print(" ");
    LoRa.endPacket();
}

void RealSync::SendPacket3(uint8_t priority, uint8_t MID, Answer1, Answer2, Answer3, Answer4, Answer5, uid_batery, uid_operator){
    LoRa.beginPacket(); 
    LoRa.print(priority);
    LoRa.print(" ");
    LoRa.print(MID);
    LoRa.print(" ");
    LoRa.print(uid_operator);
    LoRa.print(" ");
    LoRa.print(uid_battery);
    LoRa.print(" ");
    LoRa.print(Answer1);
    LoRa.print(" ");
    LoRa.print(Answer2);
    LoRa.print(" ");
    LoRa.print(Answer3);
    LoRa.print(" ");
    LoRa.print(Answer4);
    LoRa.print(" ");
    LoRa.print(Answer5);
    LoRa.print(" ");
    LoRa.endPacket(); 
}