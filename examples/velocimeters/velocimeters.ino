// 2023 Daniel Carvalho Frulane de Souza

// This is an example for an Local Processing unit 
// that measures speed values from 2 different sensors.

#include <stdio.h>
#include <SPI.h>
#include <mcp2515.h>
#include <string.h>

int HEXADECIMAL_XX_IDENTIFIER = 0x110;

int MAXIMUM_NUMBER_OF_PROCESSING_SENSORS = 16; // from 0x00 to 0x0F
int CAN_DEFAULT_LENGTH = 8; // size of data messages in bytes
int delayResetTime = 3000;

// Constants
int CENTRAL_PROCESSING_ID = 0x000;
float CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR = 0.0;
float CENTRAL_PROCESSING_MESSAGE_CONNECTION = 1.0;
float CENTRAL_PROCESSING_MESSAGE_READING_STATE = 2.0;

int AUTOMATION_PROCESSING_ID = 0x001;
float AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0;
float AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0;

char stringDelimiter[] = ";"; // special character

unsigned long currentTime = 0;

boolean readingStateIsTrue = 0;

int SPI_CS_Pin = 10;

MCP2515 mcp2515(SPI_CS_Pin);
can_frame canMessageIncoming;
can_frame canMessageSending;


// -- Object containing all information of a specific sensor

struct infograph{
    String nameOfGraph;
    String unitOfMeasurement;
    double stepOfGraph;
    String maximumValueAlert; // may be null
    String minimumValueAlert; // may be null
    unsigned long lastCollectingTime;
    int hexadecimalYIdentifier;
    int canID;
};
typedef struct infograph infograph;

// Infograph handling (basic structure for storing a specific sensor information an graph)
infograph** listOfInfographs = (infograph**) calloc(MAXIMUM_NUMBER_OF_PROCESSING_SENSORS, sizeof(infograph*)); //list of all sensors
int sizeOfListOfInfographs = 0;

void addInfograph(String nameOfGraph, String unitOfMeasurement, double stepOfGraph, int hexadecimalYIdentifier, String maximumValueAlert = "", String minimumValueAlert = ""){
    int i = 0;
    while(listOfInfographs[i] != NULL){
        i++;
        if(i >= MAXIMUM_NUMBER_OF_PROCESSING_SENSORS){
            return;
        }
    }
    infograph* added = (infograph*) calloc(1, sizeof(infograph));
    added->nameOfGraph = nameOfGraph;
    added->unitOfMeasurement = unitOfMeasurement;
    added->stepOfGraph = stepOfGraph;
    added->maximumValueAlert = maximumValueAlert;
    added->minimumValueAlert = minimumValueAlert;
    added->lastCollectingTime = 0;
    added->hexadecimalYIdentifier = hexadecimalYIdentifier; // from 0x00 to 0x0F, or 0 to 15
    added->canID  = HEXADECIMAL_XX_IDENTIFIER + hexadecimalYIdentifier; //message identifier

    listOfInfographs[i] = added;
    sizeOfListOfInfographs++; 
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


// -- Main functions

void setup() {
    SPI.begin();

    mcp2515.reset(); // resets CAN
    mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();
    canMessageSending.can_dlc = CAN_DEFAULT_LENGTH;

    addInfograph("Speed1", "km/h", 0.3, 0, "60", "2");
    addInfograph("Speed2", "km/h", 0.3, 1, "60", "2");
}


void loop() {
    if (mcp2515.readMessage(&canMessageIncoming) == MCP2515::ERROR_OK){
        if (canMessageIncoming.can_id == CENTRAL_PROCESSING_ID){ 
            decryptCharListOfSize((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            String messageConvertedToString = convertCharArrayToString((char*) canMessageIncoming.data, CAN_DEFAULT_LENGTH);
            float value = convertCompleteStringToFloat(&messageConvertedToString);
            if (value == CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR){
                resetSystem();
            }
            else if (value == CENTRAL_PROCESSING_MESSAGE_CONNECTION){
                sendAllInfographsInformationToCAN();
            }
            else if (value == CENTRAL_PROCESSING_MESSAGE_READING_STATE){
                readingStateIsTrue = 1;
            }
        }
    }

    if (readingStateIsTrue){ // when all nodes are connected
        currentTime = millis();
        sendAllInfographsReadingsToCAN();
    }
}

void resetSystem(){
    delay(delayResetTime);
    readingStateIsTrue = 0;
    for (int i = 0; i < sizeOfListOfInfographs; i++){
        listOfInfographs[i]->lastCollectingTime = 0;
    }
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

// turns all relevant information of a infograph into an interpretable string
String getInfographInfo(infograph* infog){ 
    return infog->nameOfGraph + stringDelimiter + infog->stepOfGraph + stringDelimiter + infog->unitOfMeasurement + stringDelimiter + 
    infog->maximumValueAlert + stringDelimiter + infog->minimumValueAlert + stringDelimiter;
}

String getFormatedReading(double reading){
    return (String) reading + stringDelimiter;
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

// sends a char list associated to an infograph as data to CAN
void sendCharListToCAN(char* text){
    for (int i = 0; i < CAN_DEFAULT_LENGTH; i++){
        canMessageSending.data[i] = text[i];
    }
    mcp2515.sendMessage(&canMessageSending);
}

// sends a list of list of char associated to an infograph as data to CAN
void sendListOfListOfCharToCAN(char** listOfMessagesToSend, infograph* infog, int sizeOfList){
    canMessageSending.can_id = infog->canID;
    for (int i = 0; i < sizeOfList; i++){
        sendCharListToCAN(listOfMessagesToSend[i]);
    }
}

void freeListOfListsOfChar(char** listPointer, int numberOfSplits){
    for (int i = 0; i < numberOfSplits; i++){
        free(listPointer[i]);
    }
    free(listPointer);
}

// sends all information of the list of infographs
void sendAllInfographsInformationToCAN(){
    for (int i = 0; i < sizeOfListOfInfographs; i++){
        infograph* currentInfograph = listOfInfographs[i];
        String infographInfo = getInfographInfo(currentInfograph);
        infographInfo = encryptString(infographInfo);
        int numberOfSplits = calculateNumberOfSplits(infographInfo, CAN_DEFAULT_LENGTH); 
        char** listOfMessagesToSend = splitStringToListOfListsOfChar(infographInfo, CAN_DEFAULT_LENGTH, numberOfSplits);
        sendListOfListOfCharToCAN(listOfMessagesToSend, currentInfograph, numberOfSplits);
        freeListOfListsOfChar(listOfMessagesToSend, numberOfSplits);
    }
}

// sends all readings of the list of infographs
void sendAllInfographsReadingsToCAN(){
    unsigned long currentTime = millis();
    for (int i = 0; i < sizeOfListOfInfographs; i++){
        infograph* selectedInfograph = listOfInfographs[i];
        if(currentTime - selectedInfograph->lastCollectingTime >= 1000 * selectedInfograph->stepOfGraph){
            selectedInfograph->lastCollectingTime = currentTime;
            float infographReading = getInfographReading(selectedInfograph->hexadecimalYIdentifier);
            String infographFormatedReading = getFormatedReading(infographReading);
            infographFormatedReading = encryptString(infographFormatedReading);
            int numberOfSplits = calculateNumberOfSplits(infographFormatedReading, CAN_DEFAULT_LENGTH); 
            char** listOfMessagesToSend = splitStringToListOfListsOfChar(infographFormatedReading, CAN_DEFAULT_LENGTH, numberOfSplits);
            sendListOfListOfCharToCAN(listOfMessagesToSend, selectedInfograph, numberOfSplits);
            freeListOfListsOfChar(listOfMessagesToSend, numberOfSplits);
        }
    }
}

// gets a reading of a certain sensor
float getInfographReading(int infographYValue){
    if(infographYValue == 0){
        // TODO: return measure for first sensor
    }
    else if(infographYValue == 1){
        // TODO: return measure for second sensor
    }
    return 0; // error
}
