// 2023 Daniel Carvalho Frulane de Souza

// The automation processing is basically a actuator wich can also send messages through CAN.

#include <stdio.h>
#include <SPI.h>
#include <mcp2515.h>
#include <string.h>

/* For adding a new node based on this template, change the values of all "SUBSTITUTE" sections */

/* INCLUDE CONSTANT VALUES FOR EACH MESSAGE, THEN ADD RESTRICTIONS ON THE activateFunctions CALLS*/
// example:
// int HEXADECIMAL_IDENTIFIER_SENSOR_BIOMETRIC = 0x032;
// int HEXADECIMAL_IDENTIFIER_SENSOR_RFID = 0x032;

// it is possible to add other messages
int CAN_DEFAULT_LENGTH = 8; // size of data messages in bytes
int delayResetTimeout = 300;

int CENTRAL_PROCESSING_ID = 0x000;
float CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR = 0.0;
float CENTRAL_PROCESSING_MESSAGE_CONNECTION = 1.0;
float CENTRAL_PROCESSING_MESSAGE_READING_STATE = 2.0;

int AUTOMATION_PROCESSING_ID = 0x001; // this is it
float AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0;
float AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0;

boolean automationStateCanBeSetToTrue = 0;
boolean automationStateIsTrue = 0;
String* pointerToACompleteString = NULL; // points to a string finished with the special character

char stringDelimiter[] = ";"; // special character

int SPI_CS_Pin = 10; // pinout for the mcp2515

MCP2515 mcp2515(SPI_CS_Pin);
can_frame canMessageIncoming;
can_frame canMessageSending;


// -- linked list codes for individual sensors used by the actuator
// Each message must be processed either as a sensor or as a operation function from the main nodes

struct sensor{
    int hexadecimalIdentifier;
    String messageConcatenatedString;
    struct sensor* next;
};
typedef struct sensor sensor;

sensor* headListOfSensors = NULL;

void addSensorToActuator(int hexadecimalIdentifier){
    sensor* newSensor = (sensor*) calloc(1,sizeof(sensor));
    newSensor->hexadecimalIdentifier = hexadecimalIdentifier;
    newSensor->messageConcatenatedString = "";
    newSensor->next = NULL;

    if (headListOfSensors == NULL) {
        headListOfSensors = newSensor;
    } 
    else {
        sensor* current = headListOfSensors;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newSensor;
    }
}


// -- Cryptography to be implemented

void decryptCharListOfSize(char* encryptedCharList, int size){
    for(int i = 0; i < size; i++){
        encryptedCharList[i] = decryptCharacter(encryptedCharList[i]);
    }
}

char decryptCharacter(char encryptedCharacter){
    return encryptedCharacter;
}

String encryptString(String toEncrypt){
    return toEncrypt;
}


// -- Main Functions

void setup() {
    SPI.begin();

    mcp2515.reset(); // resets CAN
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();
    canMessageSending.can_dlc = CAN_DEFAULT_LENGTH;
    canMessageSending.can_id = CENTRAL_PROCESSING_ID;

    /* SUBSTITUTE HERE 1 FUNCTION CALL FOR ADDING ON THE LINKED LIST FOR EACH SENSOR */
    // example: addSensorToActuator(0x123);

    addSensorToActuator(0x000); // Central processing
}

void loop() {
    if (mcp2515.readMessage(&canMessageIncoming) == MCP2515::ERROR_OK) {
        if((automationStateIsTrue && thisActuatorReceivesMessagesFromThisNode(canMessageIncoming.can_id)) || canMessageIncoming.can_id == CENTRAL_PROCESSING_ID){
            decryptCharListOfSize((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            String messageConvertedToString = convertCharArrayToString((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            appendStringToAdequateMessageAndUpdatePointer(messageConvertedToString, canMessageIncoming.can_id);
            if(isACompleteString(*pointerToACompleteString)){
                float value = convertCompleteStringToFloat(pointerToACompleteString);
                if(canMessageIncoming.can_id == CENTRAL_PROCESSING_ID){
                    if (value == CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR){
                        resetSystem();
                    }
                    else if (value == CENTRAL_PROCESSING_MESSAGE_READING_STATE){
                        automationStateCanBeSetToTrue = 1;
                    }
                }
                else if(automationStateIsTrue){
                    activateFunctions(canMessageIncoming.can_id, value);
                }
                *pointerToACompleteString = "";
            }
        }
    }

    if(automationStateCanBeSetToTrue){
        if(pushedAutomationButton()){
            automationStateIsTrue = !automationStateIsTrue;
            float messageToSend = automationStateIsTrue ? AUTOMATION_PROCESSING_MESSAGE_TURN_ON : AUTOMATION_PROCESSING_MESSAGE_TURN_OFF;
            String formatedMessage = getFormatedMessage(messageToSend);
            formatedMessage = encryptString(formatedMessage);
            copyStringToCharArray(formatedMessage, (char*) canMessageSending.data, CAN_DEFAULT_LENGTH);
            mcp2515.sendMessage(&canMessageSending);
        }
    }
}

void resetSystem(){ // in case of an error, the actuator must return to it's original state
    int lastTime = millis();
    while (millis() - lastTime < delayResetTimeout){
        if (mcp2515.readMessage(&canMessageIncoming) == MCP2515::ERROR_OK){ // flushes CAN
            lastTime = millis();
        }
    }
    automationStateCanBeSetToTrue = 0;
    automationStateIsTrue = 0;
    resetMessageStrings();
    /* SUBSTITUTE HERE ANY ADDITIONAL RESETS*/
}

void resetMessageStrings(){
    sensor* current = headListOfSensors;
    while (current != NULL) {
        current->messageConcatenatedString = "";
        current = current->next;
    }
}

bool thisActuatorReceivesMessagesFromThisNode(int hexadecimalIdentifier){
    sensor* current = headListOfSensors;
    while (current != NULL) {
        if(current->hexadecimalIdentifier == hexadecimalIdentifier){
            return true;
        }
        current = current->next;
    }
    return false;
}

bool pushedAutomationButton(){ // this needs to be implemented with the given design
    return false;
}

void appendStringToAdequateMessageAndUpdatePointer(String toAppend, int hexadecimalIdentifier){
    sensor* current = headListOfSensors;
    while (current != NULL) {
        if(current->hexadecimalIdentifier == hexadecimalIdentifier){
            current->messageConcatenatedString += toAppend;
            break;
        }
        current = current->next;
    }
    if(isACompleteString(current->messageConcatenatedString)){
        pointerToACompleteString = &(current->messageConcatenatedString);
    }
}

bool isACompleteString(String toCheck){ // verifies if the special character exists in the string
    return toCheck.indexOf(stringDelimiter) != -1;
}

String getFormatedMessage(double reading){
    return (String) reading + stringDelimiter;
}

float convertCompleteStringToFloat(String* messagePointer){    
    String message = *messagePointer;
    message.remove(message.length() - 1, 1); // removes the special character, which is in the last position
    return atof(message.c_str()); // conversion string -> float
}

String convertCharArrayToString(char* list, int sizeOfCopy){
    String toReturn = "";
    for(int i = 0; i < sizeOfCopy; i++){
        if(list[i] == '\0'){
            return toReturn;
        }
        toReturn += list[i];
    }
    return toReturn;
}

void copyStringToCharArray(String toCopy, char* toReceive, int sizeOfCopy){
    for (int i = 0; i < sizeOfCopy && i < toCopy.length(); i++){
        if(toCopy[i] != '\0'){
            toReceive[i] = toCopy[i];
        }
    }
}

/* SUBSTITUTE APROPRIATE ACTIVATION FUNCTIONS ACTUATORS FOR EACH HEXADECMAL IDENTIFIER */
void activateFunctions(int hexadecimalIdentifier, float reading){
    /*
    if (hexadecimalIdentifier == HEXADECIMAL_IDENTIFIER_SENSOR_BIOMETRIC){
        activateFunction1(reading);
    }
    else if (hexadecimalIdentifier == HEXADECIMAL_IDENTIFIER_SENSOR_RFID){
        activateFunction2(reading);
    }
    */
    // other functions
}

/* CREATE ACTIVATION FUNCTIONS */
/*
void activateFunction1(float reading){
    ;
}

void activateFunction2(float reading){
    ;
}
*/
