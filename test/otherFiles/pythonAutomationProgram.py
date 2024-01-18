import pyautogui
import serial
import time

"""
Custom Sound steps for fittable population
General steps for programming
    Setting up the patient and implant
        1. Press the "Add button"
        2. Enter text boxes for the following
            a. Given Name
            b. Family Name
        3. Change the last number of the date of birth
        4. Press Next
        5. On the hardware page, select dropdown list for right side
            a. Choose 'cochlear implant'
        6. On the implants pop up, click on the drop down for Model
            a. Choose the appropiate implant
            b. Press Save
        7. Press Next on the hardware page
        8. Press the note button on the section outline (under the patient detials section)
        9. Press Save on the notes page
    Programming the sp
        1, Assume that SP is automatically selected by custom sound on the dashboard
        2. Click on the measure tab
            a. Click on the measure button
        3. Click on the adjust tab
        4. On the adjust page
            a. Click on create
            b. Click on thresholds
        5. On the thresholds page, click on the gear icon
        6. On the gear page
            a. Click on the leftmost number at the top of the graph
            b. Click on the Make only selected channels measureable
            c. Click on continue to finalise
        7. On the finalise page, click on Save

pyautogui functions I will need:
    1. locateOnScreen(): I will take a screenshot of the buttons and use agui to find the image on the screen
        2. center(): It will return the center of the object found. This is useful for finding where to click
    2.locateCenterOnScreen(): Combines locateonscreen and center
    2. moveTo(): Moves to the coordinates inside 
"""


"""
What do i need to do with serial?
    1. Disconnect custom Sound from serial once the SP has been programmed
    2. Connect the com port to the microcontroller, send it a number via serial message to transition to the next state
    3. Reconnect the com port to custom sound
"""



#Full python automation program
#classes
sampleArr = []  #declaring an array
class sampleData:   #storing the model number and serial number the implant (just the serial number for the sp)
    def __init__(self, spSerial, implantSerial, implantModel):
        self.spSerial = spSerial
        self.implantSerial = implantSerial
        self.implantModel = implantModel

#USER PRESSES START ON THE UC
    #wait for a message from the uc indicating the automation has started

def waitingForStart():
    print("Waiting for data from uC")
    ucData = '0'
    while ucData != b's\r\n':
        ucData = ser.readline()
        #print("Data received" + str(ucData))
    print("Received the start command from uC")
    
#GETTING THE NAMES AND MODELS OF THE SAMPLES
def getSampleData():
    sampleNum = 0
    spSN = 0
    implantSN = 0
    implantModNum = 0
    ucData = '0'

    while sampleNum <= 4:
        print("Sample No: "+ str(sampleNum))
        #run the deviceInfo and the python script to read the implant
        
        #use python to extract the data and store it (the sp's serial number, and the implants serial and model num)
        sampleArr.append(sampleData(spSN,implantSN, implantModNum))
        
        #send a message to uc once a sample is finished being read, tell it to rotate to the next sample
        ser.write(b'1')
        print("Sent Notification to uC to rotate to the next sample")
        
        while ucData != b'e\r\n':
            ucData = ser.readline()
            continue    #do nothing whilst waiting for the received clarification from uC
        print("Received acknowledgement from uC")    
        sampleNum += 1  #incrementing the sampleNum\
        time.sleep(8)
        #do this 4 more times

#CUSTOM SOUND AUTOMATION BEGINS
    #just run the python code
def customSoundProgramming():
    testVal = 0
    #PROGRAMMING FINISHES
    #send a notification to the uC
    ser.write(b'1')

########################################    MAIN EXECUTION OF CODE      #####################################
ser = serial.Serial(port='COM5', baudrate=9600)
waitingForStart()
getSampleData()
customSoundProgramming()



