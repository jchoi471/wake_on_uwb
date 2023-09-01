# cs8803_mci
Repository containing sample code for the Mobile Computing and IOT course at Georgia Tech

## UWB Setup

Install Ardunio:
http://www.arduino.cc/en/Main/Software

Configure Ardunio Environment:
https://learn.adafruit.com/adafruit-feather-m0-basic-proto/setup

Follow these steps:

1. Go to Arduino IDE
2. Go to Tools->Board->Board Manager and search for these two boards and install them (use the exact same version given in the image below):

![MicrosoftTeams-image (1)](https://github.gatech.edu/storage/user/55995/files/aff29eb0-090c-441e-8fbc-bf48a868bf54)
![MicrosoftTeams-image (2)](https://github.gatech.edu/storage/user/55995/files/3d9b4978-c4e5-4b92-b591-1b0f0896759c)


3. Go to Preferences like shown in this image:

<img width="802" alt="Screen Shot 2022-09-15 at 1 39 29 PM" src="https://github.gatech.edu/storage/user/55995/files/2a3f2032-dafc-4d1e-ad83-df86259f9a7b">

There will be a libraries folder inside the PATH. Add all the libraries in (https://github.gatech.edu/rsinghal42/UWB_Basic_Setup/tree/master/Arduino_libraries) to the folder.

4. For verifying if you code is working load the "UWB_Initiator.ino" file in Arduino and click verify on the top left corner.

### Two way Ranging (TWR)

TWR, by contrast, determines the Time of Flight of the UWB radio frequency signal, then calculates the distance between the nodes, by multiplying the time by the speed of light. The TWR process is applied between the tag and the demanded Anchor, and only one Anchor may be actively involved in TWR at a given time slot.

In order to measure the distance, three messages need to be exchanged:

The tag initializes TWR by sending a ‘Poll’ message to the known address of the Anchor in time. This is called the Time of Sending Poll, or TSP.
The Anchor then records the Time of Reception of Poll(TRP), and replies with the response message at the Time of Sending Response(TSR). 
The Tag, upon receiving the response message, then records the Time of Response Reception (TRR) and composes the final message, in which its ID, TSP, TRR and Time Start Final(TSF) are included.

The TWR logic is there in https://github.gatech.edu/rsinghal42/UWB_Basic_Setup/blob/master/UWB_Initiator/RangingContainer.h which you can see below:

<img width="629" alt="Screen Shot 2022-09-15 at 11 42 27 PM" src="https://github.gatech.edu/storage/user/55995/files/dcaff3bb-693d-43a9-9a64-7b3af59fa0b3">

### Execute TWR Code:

To execute the TWR code take two UWB devices like this:

![IMG_20220915_234833](https://github.gatech.edu/storage/user/55995/files/79217ce7-2c61-4bd6-91a3-89dbcfe4b23b)

As you can see above we need one or two USB cable and two battery for both the UWB device. Plug each device in the system and load the "UWB_Tag.ino" for one device and "UWB_Initiator.ino" for the other device. One will be acting as a tag and other as demanded Anchor. Not that there is a reset button like this on each device:

![thumbnail_image004](https://github.gatech.edu/storage/user/55995/files/0514ba61-b5ac-4547-8231-cd1865e4ce3d)

If you double press the reset button, the board enters a bootloader mode which speeds up upload of new code onto the board (through the IDE). It is also possible to just copy paste the 3 important files in the bootloader mode. A single click of that button merely resets the microcontroller and it restarts by running setup() once again. So whenever there is a problem with the device press the reset button and burn the code on the device. To burn any code on the connected device you need to do the following:

1. Go to Tools->Port and select the AdaFruit Feather M0 Port. If you are not able to see this port there is some issue with the setup.
2. Click the Upload button next to verify button on the top left corner.

If code is burned succesfully you will see output like this:

<img width="229" alt="Screen Shot 2022-09-15 at 11 56 04 PM" src="https://github.gatech.edu/storage/user/55995/files/b0a4a488-74f7-4984-b54e-d2f391fe1ebe">

### Verify TWR Results Distance between two UWB Devices

Once you burned both the UWB_Tag and UWB_Initiator code on the devices respectively go to Tools->Serial Monitor and check the Serial Output where we have printed the TWR results. The final output will look something like this:

<img width="684" alt="Screen Shot 2022-09-15 at 11 58 16 PM" src="https://github.gatech.edu/storage/user/55995/files/1b0b3b7b-893b-4efd-ab3a-643a779fc76d">

If all the above steps are succesfully executed for you Voilà. You have successfully completed the UWB Basic Setup and now you are ready to understand the code, play with it and implement it within you Project.

### Debug and Print Channel Impulse Response

Use an option DEBUG_CIR (search of DEBUG_CIR in the code and set ot 1) and set it to 1 in both UWB_Tag.ino and UWB_Initiator.ino so that you can use this to geberate Channel imuplse response like mentioned in the diagram below:

![image](https://github.gatech.edu/storage/user/55995/files/398e394b-4b88-453b-af96-e6a0c74d18df)
