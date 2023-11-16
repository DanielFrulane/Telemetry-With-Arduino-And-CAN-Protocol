// 2023 Daniel Carvalho Frulane de Souza

// This code will receive any complete system instructions on UART pins and send them to CAN.
// The instructions may have more or less than 8 characters.
// Also, it will receive any 8-character inputs on CAN, process them,
// and send the appropriate complete instructions to UART.
// Since the CAN ID is relevant, the conversion of a CAN message to UART will be:
// "[CAN ID][Special Character][CAN message][Special Character][\n]".
// As character conversion in encryption may lead to the [\n] character,
// Cryptography encoding and decoding should be made in this module, 
// and not in the central processing.

#include <SPI.h>
#include <mcp2515.h>
#include <SoftwareSerial.h>

int CAN_DEFAULT_LENGTH = 8; // size of data messages in bytes
int delayResetTimeout = 300;

int CENTRAL_PROCESSING_ID = 0x000;
float CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR = 0.0;
float CENTRAL_PROCESSING_MESSAGE_CONNECTION = 1.0;
float CENTRAL_PROCESSING_MESSAGE_READING_STATE = 2.0;

int AUTOMATION_PROCESSING_ID = 0x001;
float AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0;
float AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0;

boolean readingStateIsTrue = 0;
String* pointerToACompleteString = NULL;

char stringDelimiter[] = ";"; // special character

SoftwareSerial mySerial(0, 1); // pins for: RX, TX

struct can_frame messageToSend;
struct can_frame messageToReceive;

MCP2515 mcp2515(10);

struct sensor{
    int hexadecimalIdentifier;
    String messageConcatenatedString;
    struct sensor* next;
};
typedef struct sensor sensor;

sensor* headListOfSensors = NULL;

// -- linked list codes for individual sensors used by the actuator
// Each message must be processed either as a sensor or as a operation function from the main nodes

void addSensorToActuator(int hexadecimalIdentifier){ // sensors are dynamically added and removed as messages are being received
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
            if(current->hexadecimalIdentifier == hexadecimalIdentifier){
                return;
            }
            current = current->next;
        }
        current->next = newSensor;
    }
}


void removeSensorFromActuator(int hexadecimalIdentifier) {
    struct sensor* current = headListOfSensors;
    struct sensor* previous = NULL;

    if (current != NULL && current->hexadecimalIdentifier == hexadecimalIdentifier) {
        headListOfSensors = current->next;
        free(current);
        return;
    }
    while (current != NULL && current->hexadecimalIdentifier != hexadecimalIdentifier) {
        previous = current;
        current = current->next;
    }
    if (current == NULL){
        return;
    }

    previous->next = current->next;
    free(current);
}


// -- Cryptography

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


void setup() {
    SPI.begin();
    mySerial.begin(9600);
    mcp2515.reset();
    mcp2515.setBitrate(CAN_500KBPS,MCP_8MHZ);
    mcp2515.setNormalMode();
}

void loop() {
    if (mcp2515.readMessage(&messageToReceive) == MCP2515::ERROR_OK) {
        decryptCharListOfSize((char*) messageToReceive.data, CAN_DEFAULT_LENGTH);
        String messageConvertedToString = convertCharArrayToString((char*) messageToReceive.data, CAN_DEFAULT_LENGTH);
        addSensorToActuator(messageToReceive.can_id);
        appendStringToAdequateMessageAndUpdatePointer(messageConvertedToString, messageToReceive.can_id);
        if(isACompleteString(*pointerToACompleteString)){
            
            String messageUART = convertCANMessageToUART(*pointerToACompleteString, messageToReceive.can_id);
            sendMessageToUART(messageUART);

            *pointerToACompleteString = "";
            removeSensorFromActuator(messageToReceive.can_id);
        }
    }
  
    if (mySerial.available()){
        String messageUART = mySerial.readStringUntil('\n');
        convertAndSendUARTMessageToCAN(&messageUART);
    }
}

// -- Conversion Functions

String convertCANMessageToUART(String messageCAN, int canID){
    return String(canID) + stringDelimiter + messageCAN + stringDelimiter + "\n";
}

void sendMessageToUART(String messageUART){
    mySerial.print(messageUART);
}

void convertAndSendUARTMessageToCAN(String* messageUART){ // all messages will be from the central processing
    String treated = encryptString(*messageUART);
    int numberOfSplits = calculateNumberOfSplits(treated, CAN_DEFAULT_LENGTH); 
    char** listOfMessagesToSend = splitStringToListOfListsOfChar(treated, CAN_DEFAULT_LENGTH, numberOfSplits);
    sendListOfListOfCharToCAN(listOfMessagesToSend, CENTRAL_PROCESSING_ID, numberOfSplits);
    freeListOfListsOfChar(listOfMessagesToSend, numberOfSplits);
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
    if (readingStateIsTrue){ // special character will appear 1 time
        return toCheck.indexOf(stringDelimiter) != -1;
    }
    else{ // special character will appear 5 times
        return countNumberOfCharacterOccurences(toCheck, stringDelimiter[0]) == 5;
    }
}

int countNumberOfCharacterOccurences(String toCheck, char toCount){
    int size = sizeof(toCheck) / sizeof(toCheck[0]);
    int count = 0;
    for (int i = 0; i < size; i++){
        if (toCheck[i] == toCount){
            count++;
        }
    }
    return count;
}

int calculateNumberOfSplits(String text, int sizeOfSplit){
    int textSize = text.length();
    return (textSize / sizeOfSplit) + ((textSize % sizeOfSplit) ? 1 : 0);
}

// returns a splited string into a list of list of char of a certain size
char** splitStringToListOfListsOfChar(String text, int sizeOfSplit, int numberOfSplits){
    char** listOfStrings = (char**) calloc(numberOfSplits, sizeof(char*));
    char subtext[sizeOfSplit];
    for (int i = 0; i < numberOfSplits; i++){
        listOfStrings[i] = (char*) calloc(sizeOfSplit, sizeof(char));
        for (int j = 0; j < sizeOfSplit; j++){
            listOfStrings[i][j] = text[i*8 + j];
        }
    }
    return listOfStrings;
}

void freeListOfListsOfChar(char** listPointer, int numberOfSplits){
    for (int i = 0; i < numberOfSplits; i++){
        free(listPointer[i]);
    }
    free(listPointer);
}

// sends a list of list of char associated to an infograph as data to CAN
void sendListOfListOfCharToCAN(char** listOfMessagesToSend, int hexadecimalIdentifier, int sizeOfList){
    messageToSend.can_id = hexadecimalIdentifier;
    for (int i = 0; i < sizeOfList; i++){
        sendCharListToCAN(listOfMessagesToSend[i]);
    }
}

// sends a char list associated to an infograph as data to CAN
void sendCharListToCAN(char* text){
    for (int i = 0; i < CAN_DEFAULT_LENGTH; i++){
        messageToSend.data[i] = text[i];
    }
    mcp2515.sendMessage(&messageToSend);
}