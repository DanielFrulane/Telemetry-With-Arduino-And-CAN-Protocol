// 2023 Daniel Carvalho Frulane de Souza

// This is an example for an Actuator Unit that activate a set of headlights 
// whenever it receives a luminosity reading > 0. 
// Scale of luminosity would be defined by the user 
// in the Local Processing of the luminosity sensor.

#include <stdio.h>
#include <SPI.h>
#include <mcp2515.h>
#include <string.h>

int CAN_DEFAULT_LENGTH = 8; // size of data messages in bytes
int delayResetTimeout = 300;

int CENTRAL_PROCESSING_ID = 0x000;
float CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR = 0.0;
float CENTRAL_PROCESSING_MESSAGE_CONNECTION = 1.0;
float CENTRAL_PROCESSING_MESSAGE_READING_STATE = 2.0;

int AUTOMATION_PROCESSING_ID = 0x001;
float AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0;
float AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0;

int LIGHTS_ACTUATOR_ID = 0x123;

boolean automationStateIsTrue = 0;
String* pointerToACompleteString = NULL; // points to a string finished with the special character

char stringDelimiter[] = ";"; // special character

int SPI_CS_Pin = 10; // pinout for the mcp2515

MCP2515 mcp2515(SPI_CS_Pin);
can_frame canMessageIncoming;


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


// -- Main Functions

void setup() {
    SPI.begin();

    mcp2515.reset(); // resets CAN
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

    addSensorToActuator(LIGHTS_ACTUATOR_ID);

    // both Central Processing and Automation Processing must be in all actuator nodes
    addSensorToActuator(0x000); // Central processing
    addSensorToActuator(0x001); // Automation procesing
}

void loop() {
    if (mcp2515.readMessage(&canMessageIncoming) == MCP2515::ERROR_OK) {
        if((automationStateIsTrue && thisActuatorReceivesMessagesFromThisNode(canMessageIncoming.can_id)) || canMessageIncoming.can_id == CENTRAL_PROCESSING_ID || canMessageIncoming.can_id == AUTOMATION_PROCESSING_ID){
            decryptCharListOfSize((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            String messageConvertedToString = convertCharArrayToString((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            appendStringToAdequateMessageAndUpdatePointer(messageConvertedToString, canMessageIncoming.can_id);
            if(isACompleteString(*pointerToACompleteString)){
                float value = convertCompleteStringToFloat(pointerToACompleteString);
                if(canMessageIncoming.can_id == CENTRAL_PROCESSING_ID){
                    if (value == CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR){
                        resetSystem();
                    }
                }
                else if (canMessageIncoming.can_id == AUTOMATION_PROCESSING_ID) {
                    if (value == AUTOMATION_PROCESSING_MESSAGE_TURN_ON){
                        automationStateIsTrue = 1;
                    }
                    else if (value == AUTOMATION_PROCESSING_MESSAGE_TURN_OFF){
                        automationStateIsTrue = 0;
                    }
                }
                else if(automationStateIsTrue){
                    activateFunctions(canMessageIncoming.can_id, value);
                }
                *pointerToACompleteString = "";
            }
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
    automationStateIsTrue = 0;
    resetMessageStrings();
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

void activateFunctions(int hexadecimalIdentifier, float reading){
    if (hexadecimalIdentifier == LIGHTS_ACTUATOR_ID){
        activateLights(reading);
    }
}

void activateLights(float reading){
    if(reading > 0.0){
        // TODO: activate lights
    }
    else{
        // TODO: deactivate lights
    }
}