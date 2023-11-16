# 2023 Daniel Carvalho Frulane de Souza

from datetime import datetime, timedelta
import serial
import time
import os

import Adafruit_BBIO.UART as UART

UART.setup("UART4")

CAN_DEFAULT_LENGTH = 8 # size of data messages in bytes
delayInitializationTime = 5 # makes sure all the nodes have initialized
delayConnectingTime = 0.3 # makes sure all the nodes have sent their respective connecting data
delayResetTime = 3 # waits for the system reset on a full reset

# Constants
CENTRAL_PROCESSING_ID = 0x000 # this node's ID
CENTRAL_PROCESSING_MESSAGE_FATAL_ERROR = 0.0 #TODO: fatal error identification
CENTRAL_PROCESSING_MESSAGE_CONNECTION = 1.0
CENTRAL_PROCESSING_MESSAGE_READING_STATE = 2.0

AUTOMATION_PROCESSING_ID = 0x001
AUTOMATION_PROCESSING_MESSAGE_TURN_OFF = 3.0
AUTOMATION_PROCESSING_MESSAGE_TURN_ON = 4.0

stringDelimiter = ';' # special character

currentTime = 0;

readingStateIsTrue = False;

baudrateStandard = 9600;
serialPortCAN = None
serialPortGPS = None
serialPortUSB = None

serialPortCANpath = "/dev/ttyO4" #pins 11 and 13
serialPortGPSpath = "/dev/ttyO1" #pins 24 and 26
serialPortUSBpath = "/dev/ttyUSB0" #USB port


# generic information holder for data about an specific sensor
class Infograph:
    def __init__(self, hexadecimalIdentifier, nameOfGraph, stepOfGraph, unitOfMeasurement, maximumValueAlert = None, minimumValueAlert = None):
        self.hexadecimalIdentifier = hexadecimalIdentifier
        self.nameOfGraph = nameOfGraph
        self.stepOfGraph = stepOfGraph
        self.unitOfMeasurement = unitOfMeasurement
        self.maximumValueAlert = maximumValueAlert
        self.minimumValueAlert = minimumValueAlert

        self.informationString = ""

listOfInfographs = []


def initializeCentralProcessing():
    while True:
        try:
            serialPortCAN = serial.Serial(port = serialPortCANpath, baudrate = baudrateStandard) 
            serialPortCAN.close()
            serialPortCAN.open()
            break
        except serial.SerialException:
            pass
        time.sleep(1)

    while True:
        try:
            serialPortGPS = serial.Serial(port = serialPortGPSpath, baudrate = baudrateStandard)  
            serialPortGPS.close()
            serialPortGPS.open()
            break
        except serial.SerialException:
            pass
        time.sleep(1)

    while True:
        try:
            serialPortUSB = serial.Serial(port = serialPortUSBpath, baudrate = baudrateStandard)  
            serialPortUSB.close()
            serialPortUSB.open()
            break
        except serial.SerialException:
            pass
        time.sleep(1)


# execution functions

def sendFloatMessageToUART(floatMessage):
    stringMessage = str(floatMessage) + "\n"
    serialPortCAN.write(stringMessage.encode())

# will only return something different from "" if there is something waiting in the buffer.
# in this case, reads the string until the "\n" character
def getNextStringFromUART():
    concatenated = ""
    while serialPortCAN.in_waiting:
        try:
            character = serialPortCAN.read().decode('utf-8')
            if (character == "\n"):
                break
            else:
                concatenated += character
        except UnicodeDecodeError:
            pass
    return concatenated

def getMessageHexadecimalIdentifier(string):
    listOfSplits = string.split(stringDelimiter)
    return int(listOfSplits[0])

def convertCompleteStringToFloat(string):
    return float(string)

'''SUBSTITUTE HERE THE EXPECTED PROCESSING'''

def processReadingData(canID, value):
    pass
    # TODO: save data on SD card: use "os" library;
    # TODO: send data to XBee: write on "serialPortUSB";
    # TODO: send data to cloud;


# runtime functions

def setup():
    time.sleep(delayInitializationTime)
    initializeCentralProcessing()


def receiveConnectionMessages():
    currentTime = datetime.now()
    while(datetime.now() - currentTime < timedelta(seconds=delayConnectingTime)): # timeout
        receivedString = getNextStringFromUART()
        if(receivedString != "" and (getMessageHexadecimalIdentifier(receivedString) != AUTOMATION_PROCESSING_ID)):
            # id; name; step; unit; max; min
            cropedInformationList = receivedString.split(stringDelimiter)
            newInfograph = Infograph(cropedInformationList[0], cropedInformationList[1], cropedInformationList[2], cropedInformationList[3], cropedInformationList[4], cropedInformationList[5])
            listOfInfographs.append(newInfograph)
            currentTime = datetime.now()
    if(len(listOfInfographs)):
        # TODO: add infographs data to SD card
        return True
    return False


def main():
    sendFloatMessageToUART(CENTRAL_PROCESSING_MESSAGE_CONNECTION)
    readingStateIsTrue = receiveConnectionMessages()
    if (readingStateIsTrue):
        sendFloatMessageToUART(CENTRAL_PROCESSING_MESSAGE_READING_STATE)
    while(readingStateIsTrue):
        receivedString = getNextStringFromUART()
        if(receivedString != "" and (getMessageHexadecimalIdentifier(receivedString) != AUTOMATION_PROCESSING_ID)):
            # id; value
            cropedInformationList = receivedString.split(stringDelimiter)
            canID = cropedInformationList[0]
            value = convertCompleteStringToFloat(cropedInformationList[1])
            processReadingData(canID, value)


# run

setup()
main()