/*
  ==============================================================================

    main.cpp
    Created: 25 Aug 2018 11:52:38am
    Author:  MELD

  ==============================================================================
*/

#include<iostream>
#include<string>
#include<stdlib.h>
#include<sstream>
#include"SerialPort.h"
using namespace std;

//TODO: make function for these ints to be read only
int encoderData;
int buttonAData;
int buttonBData;
int buttonCData;
int buttonDData;
int buttonEData;
int buttonFData;

char incoming[MAX_DATA_LENGTH];
char output[MAX_DATA_LENGTH];
char *port = "\\\\.\\COM5";
string extractReturn;

string extract(const string& str, char beg, char end) //function to extract data from string
{
	size_t begPos;
	if ((begPos = str.find(beg)) != string::npos) {
		size_t endPos;
		if ((endPos = str.find(end, begPos)) != string::npos && endPos != begPos + 1)
			extractReturn = str.substr(begPos + 1, endPos - begPos - 1);
			//return extractReturn;
	}
	return extractReturn;//std::string();
}
 
int main() {	
	SerialPort arduino(port);
	if (arduino.isConnected()) {
		cout << "Connection Established" << endl << endl;
	}
	else cout << "ERROR: Wrong Port" << endl << endl;

	while (arduino.isConnected()) {
		char charArray[MAX_DATA_LENGTH] = "";
		arduino.readSerialPort(charArray, MAX_DATA_LENGTH);
		
		string charCaster = charArray; //make string with charArray data
		char startMarker = charArray[0]; //first index of charArray (i.e Letter Index)

		if (charCaster != ""){
			switch (startMarker) {
				case 'X': {
					std::stringstream encoderCon(extract(charCaster, startMarker, ']'));
					encoderCon >> encoderData;
					printf("%d\n", encoderData);
					break;
				}
				case 'A': {
					std::stringstream buttonACon(extract(charCaster, startMarker, ']'));
					buttonACon >> buttonAData;
					printf("%d\n", buttonAData);
					break;
				}
				case 'B': {
					std::stringstream buttonBCon(extract(charCaster, startMarker, ']'));
					buttonBCon >> buttonBData;
					printf("%d\n", buttonBData);
					break;
				}
				case 'C': {
					std::stringstream buttonCCon(extract(charCaster, startMarker, ']'));
					buttonCCon >> buttonCData;
					printf("%d\n", buttonBData);
					break;
				}
				case 'D': {
					std::stringstream buttonDCon(extract(charCaster, startMarker, ']'));
					buttonDCon >> buttonDData;
					printf("%d\n", buttonDData);
					break;
				}
				case 'E': {
					std::stringstream buttonECon(extract(charCaster, startMarker, ']'));
					buttonECon >> buttonEData;
					printf("%d\n", buttonEData);
					break;
				}
				case 'F': {
					std::stringstream buttonFCon(extract(charCaster, startMarker, ']'));
					buttonFCon >> buttonFData;
					printf("%d\n", buttonFData);
					break;
				}
			}
		}

		/*if (charCaster != "") {
			switch (startMarker) {
				case 1: if (startMarker = 'X') {
					extract(charCaster, startMarker, ']');
					std::stringstream encoderCon (extractReturn); //convert string to int
					int encoderData = 0;
					encoderCon >> encoderData;
				}
			}
			cout << extractReturn << endl;
		}*/

		
				
		/*if (charCaster != "") {
			cout << charCaster << endl;
		}*/
		
		/*if (charArray != ""){
			printf(charArray);
		}*/
	}
	return 0;
}

