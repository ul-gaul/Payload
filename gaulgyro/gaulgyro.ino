/*
SD card module:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
*/ 

#include <SPI.h>
#include <SD.h>

unsigned long time;
char charFileName[16];

void setup() {
  Serial.begin(9600);
  if (!SD.begin(4)) {
    Serial.println("Card failed, or not present");
    return;
  }
 
 String fileName = String();
 unsigned int fn = 1;
 
 while(!fn==0) {
   fileName = "file_";
   fileName += fn;
   fileName += ".txt";
   fileName.toCharArray(charFileName, sizeof(charFileName));
   
   if (SD.exists(charFileName)) { 
     fn++;
   }
   else {
     File dataFile = SD.open(charFileName, FILE_WRITE);
     fn = 0;
     dataFile.close();
   }
 }
}

void loop() {  
  File myFile = SD.open(charFileName, FILE_WRITE);
  if (myFile) {
    int x = analogRead(A0);
    int y = analogRead(A1);
    int z = analogRead(A2);
    time = millis();
    String tst = String(time) + '#' + String(x) + '#' + String(y) + '#' + String(z);
    Serial.println(tst);
    // myFile.println(tst); // ajouter cette ligne pour enregistrer sur la carte SD (FPS Drop (4 fps)... ne pas utiliser en même temps que le programme Live idéalement)
    myFile.close();
    delay(20);
  }
  else {
    Serial.println("error opening file");
    delay(20);
  }

}
