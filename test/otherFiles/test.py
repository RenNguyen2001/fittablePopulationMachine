import os
import subprocess

def sp17DeviceInfo(condition):
    if condition == 1:
        print("COM port:" + "               COM5")
        print("App F/W Running:" + "        True")
        print("WhoIsThere:" + "             ('1.0.0', '1270A03F00', '1-01-020-1002959')")
    elif condition == 0:
        print("Traceback (most recent call last):")


sp17DeviceInfo(1)

os.chdir(r"C:\Users\UTS\Documents\PlatformIO\Projects\fittablePopulationMachine\test\otherFiles")
subprocess.call('python -m PyInstaller --onefile test.py',shell = True) 

#remember to use cd to change file path cd C:\Users\UTS\Documents\PlatformIO\Projects\fittablePopulationMachine\test\otherFiles
#use python -m PyInstaller --onefile test.py