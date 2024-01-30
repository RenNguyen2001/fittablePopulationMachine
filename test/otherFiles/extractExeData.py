import os

def passFailCheck(passWord, failWord, line): #read the text from the text file
    firstWord = line.split()    #seperate the first line into words using the whitespaces as delimiters
    if firstWord[0] == passWord:    #check if the first word is COM
        print("Passed")
        return 1
    elif firstWord[0] == failWord:
        print("Failed")
        return 0

def readspSerial():
    #run the deviceInfo.exe file and store its output into a text file
    os.system(r"C:\Users\UTS\Documents\PlatformIO\Projects\fittablePopulationMachine\test\otherFiles\dist\test.exe -a >sp17DeviceInfoOutput.txt")   
    file = open("sp17DeviceInfoOutput.txt","r")
    line = file.readlines()

    passFailCheck("COM", "Traceback", line[0])
    serialWordNum = 3   #serial number is the 4th word
    #getting the line containing the serial number
    word = line[2].split()  
    #separating the line into words
    tempWord = word[serialWordNum].split("'")   #remove the apostrophoes from the side
    serialNumber = tempWord[1].split(")")    #remove the end comma
    print("Serial Number found is: " + str(serialNumber[0]))

def readImplantSerial():
    #run the script to read implant info sys7DeviceInfo.exe
    print("test")

#MAIN EXECUTION CODE
readspSerial()
readImplantSerial()

        




