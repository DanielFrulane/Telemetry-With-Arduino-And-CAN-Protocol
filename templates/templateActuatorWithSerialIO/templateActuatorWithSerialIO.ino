// 2023 Daniel Carvalho Frulane de Souza

// NOTE: For using the Serial IO for debugging as CAN, each input must be of size 8 bytes, 
// since every CAN message in the system is going to be of this size.
// input values equal to the predefined messages are reserved for their messages.
// For instance, to represent the ERROR value 0.0 we must write on serial: 00000.0;
// The SerialIO debugging simplifies some functionalities, it is important to check what is different

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

int AUTOMATION_PROCESSING_ID = 0x001;
float AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0;
float AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0;

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
    Serial.begin(9600);
    while(Serial.available() > 0) { // flushes buffer
        char t = Serial.read();
    }

    SPI.begin();

    mcp2515.reset(); // resets CAN
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();

    /* SUBSTITUTE HERE 1 FUNCTION CALL FOR ADDING ON THE LINKED LIST FOR EACH SENSOR */
    // example: addSensorToActuator(0x123);

    // both Central Processing and Automation Processing must be in all actuator nodes
    addSensorToActuator(0x000); // Central processing (adaptation)
    addSensorToActuator(0x001); // Central processing (adaptation)
    addSensorToActuator(0x002); // Central processing (adaptation)
    addSensorToActuator(0x003); // Automation procesing (adaptation)
    addSensorToActuator(0x004); // Automation procesing (adaptation)
}

void loop() {
    if (Serial.available() > 0) {
        String receivedString = Serial.readStringUntil('\n');
        Serial.println("input: " + receivedString);
        appendStringToAdequateMessageAndUpdatePointer(receivedString, convertCompleteStringToFloat(&receivedString));
        if(isACompleteString(*pointerToACompleteString)){
            float value = convertCompleteStringToFloat(pointerToACompleteString);
            float canId = convertCompleteStringToFloat(pointerToACompleteString);
            Serial.println("CanId and Value = " + (String) value); 
            if(canId == 0x000 || canId == 0x001 || canId == 0x002){
                Serial.println("Central Processing Message");
                if (value == CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR){
                    resetSystem();
                    Serial.println("Reseted system");  
                }
            }
            else if (canId == 0x003 || canId == 0x004) {
                Serial.println("Automation Processing Message"); 
                if (value == AUTOMATION_PROCESSING_MESSAGE_TURN_ON){
                    automationStateIsTrue = 1;
                    Serial.println("Activated Automation State");
                }
                else if (value == AUTOMATION_PROCESSING_MESSAGE_TURN_OFF){
                    automationStateIsTrue = 0;
                    Serial.println("Deactivated Automation State");
                }
            }
            else if(automationStateIsTrue){
                activateFunctions(canId, value);
            }
            *pointerToACompleteString = "";
        }
    }
}

void resetSystem(){ // in case of an error, the actuator must return to it's original state
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

/* SUBSTITUTE APROPRIATE ACTIVATION FUNCTIONS ACTUATORS FOR EACH HEXADECMAL IDENTIFIER */
void activateFunctions(int hexadecimalIdentifier, float reading){
    
    //activateFunction1(reading);
    //activateFunction2(reading);
    // other functions
}

/* CREATE ACTIVATION FUNCTIONS */

void activateFunction1(float reading){
    Serial.println("output1: " + (String) reading);
}

void activateFunction2(float reading){
    Serial.println("output2: " + (String) reading);
}
