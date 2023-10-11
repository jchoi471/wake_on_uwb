#include <SPI.h>
#include <SdFat.h>

int SDChipSelect = 10;
SdFat sd;

SdFile myFile1;
SdFile myFile2;

char filename1[] = "test1.txt";
char filename2[] = "test2.txt";
char line[100];
ArduinoOutStream cout(Serial);

void setup(){
    delay(10000);
    Serial.println("Initializing SD card");
    // Initialize SD card
    if (!sd.begin(SDChipSelect, SPI_FULL_SPEED)){
        Serial.println("SDCard Initialization failed!");
    }
    else{
        Serial.println("SDCard Initialization done.");
    }

    ////////////////////// Open a file in write mode ///////////////////
    if(!myFile1.open(filename1, O_CREAT | O_WRITE)){
        Serial.println("File 1 open failed");
    }
    else{
        Serial.println("File 1 open successfull");
    }
    ///////////////////////// Write to a file /////////////////////////
    myFile1.println("Line1 of file 1");
    ///////////////////////// Close a file ///////////////////////////
    myFile1.close();
    //////////////////////////////////////////////////////////////////

    if(!myFile2.open(filename2, O_CREAT | O_WRITE)){   //O_WRITE overwrites the file, does not delete existing contents
        Serial.println("File 2 open failed");
    }
    else{
        Serial.println("File 2 open successfull");
    }

    myFile2.println("Line 1 of file 2");
    myFile2.close();

    ///////////////////////// Open a file in read mode ////////////////
    myFile1.open(filename1, O_READ);
    ///////////////////////// Read from a file ////////////////////////
    myFile1.fgets(line, sizeof(line));
    Serial.println(line);
    myFile1.close();
    //////////////////////////////////////////////////////////////////

    ///////////////////////// Open a file in append mode ////////////////
    myFile2.open(filename2, O_APPEND | O_WRITE);
    ///////////////////////// Write to a file /////////////////////////
    myFile2.print("Line 2 of file 2");
    ///////////////////////// Close a file ///////////////////////////
    myFile2.close();
    //////////////////////////////////////////////////////////////////

    ///////////////////////// Open a file in read mode ////////////////
    myFile2.open(filename2, O_READ);
    ///////////////////////// Read lines from a file ////////////////////////
    int n;
    while ((n = myFile2.fgets(line, sizeof(line))) > 0) {
        if (line[n - 1] == '\n') {
        cout << '>' << line;
        } else {
        cout << '#' << line << endl;
        }
    }
    myFile2.close();
}

void loop(){
    
}
