#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <stdbool.h>

const int MASTER_ISSET_PIN = 14;
const int RETURN_SIGNAL_PIN = 13;
const int SLAVE_SIGNAL_PIN = 12;
const int MAX_RETURN_SIZE = 62;
const int SLAVE_ADDR = 1;

const char DATA_SEP = '@';
const char TYPE_SEP = '$';
const char FUNCT_ID_SEP = '!';
const char MSG_SEP = '*';
const char SIGNAL_SEP = '#';

String S_DATA_SEP = String(DATA_SEP);
String S_TYPE_SEP = String(TYPE_SEP);
String S_FUNCT_ID_SEP = String(FUNCT_ID_SEP);
String S_MSG_SEP = String(MSG_SEP);
String S_SIGNAL_SEP = String(SIGNAL_SEP);

bool isMaster;
String slaveReturn;

//UTILITY_FUNCTIONS-----------------------------------------------------------

type typeFromChar(char cType){
  switch (cType) {
    case 'i': return INT;
    case 'l': return LONG;
    case 'f': return FLOAT;
    case 'd': return DOUBLE;
    case 'c': return CHAR;
    case 's': return STRING;
  }
  return INT;
}

char charFromType(type t) {
  switch (t) {
    case INT: return 'i';
    case LONG: return 'l';
    case FLOAT: return 'f';
    case DOUBLE: return 'd';
    case CHAR: return 'c';
    case STRING: return 's';
  }
  return 'i';
}

pointer parseValueFromCString(type t, char *str) {
  switch (t) {
    case INT:{
      int *x = (int *)malloc(sizeof(int));
      sscanf(str, "%d", x);
      return (pointer) {INT, (void *)x};}
    case LONG:{
      long *x = (long *)malloc(sizeof(long));
      sscanf(str, "%ld", x);
      return (pointer) {LONG, (void *)x};}
    case FLOAT:{
      float *x = (float *)malloc(sizeof(float));
      sscanf(str, "%f", x);
      return (pointer) {FLOAT, (void *)x};}
    case DOUBLE:{
      double *x = (double *)malloc(sizeof(double));
      sscanf(str, "%lf", x);
      return (pointer) {DOUBLE, (void *)x};}
    case CHAR:{
      char *x = (char *)malloc(sizeof(char));
      *x = *(char *)str;
      return (pointer) {CHAR, (void *)x};}
    case STRING:{
      char *x = (char *)malloc(sizeof(char) * strlen(str));
      strncpy(x, str, strlen(str));
      x[strlen(str) - 1] = '\0';
      return (pointer) {STRING, (void *)x};}
  }
  return (pointer) {INT, NULL};
}

//SLAVE_FUNCTIONS-------------------------------------------------------------

String formatReturnDataString(fReturnDataAsString returnData) {
  return S_DATA_SEP + String(charFromType(returnData.type)) + S_TYPE_SEP + returnData.str;
}

String callFunctionFromString(String str) {
  int functId;
  int argCount;
  int argCountIndex = 0;
  char *csStr = (char *)malloc(str.length() * sizeof(char));
  str.toCharArray(csStr, str.length());
  String format = S_MSG_SEP + "%d" + S_FUNCT_ID_SEP + "%d";
  char *csFormat = (char *)malloc(format.length() * sizeof(char));
  format.toCharArray(csFormat, format.length());
  sscanf(csStr, csFormat, &functId, &argCount);
  free(csFormat);
  free(csStr);
  fArgumentList *args = (fArgumentList *)malloc(sizeof(fArgumentList) + argCount * sizeof(pointer));
  args->size = argCount;
  for (int i = 1; i < str.length() - 2; i++) {
    if (str.charAt(i) == TYPE_SEP) {
      int lastIndexOfArgStr = i + 1;
      while (lastIndexOfArgStr < str.length() - 1 && str.charAt(lastIndexOfArgStr + 1) != DATA_SEP) {
        lastIndexOfArgStr++;
      }
      char argType = str.charAt(i - 1);
      String argString = str.substring(i + 1, lastIndexOfArgStr);
      char *csArgString = (char *)malloc(argString.length() * sizeof(char));
      argString.toCharArray(csArgString, argString.length() + 1);
      args->p[argCountIndex] = parseValueFromCString(typeFromChar(argType), csArgString);
      if (argType != 's') {
        free(csArgString);
      }
      i = lastIndexOfArgStr + 1;
      argCountIndex++;
    }
  }
  return formatReturnDataString(functions[functId](args));
}

String formatFunctionCallsAsString(fArgumentList *args[], int n) {
  String str = S_SIGNAL_SEP;
  for (int i = 0; i < n; i++) {
    str += S_MSG_SEP + String(i) + S_FUNCT_ID_SEP + String(args[i]->size);
    for (int j = 0; j < args[i]->size; j++) {
      if (args[i]->p[j].type != STRING) {
        str += S_DATA_SEP + charFromType(args[i]->p[i].type) + S_TYPE_SEP + String(*(int *)args[i]->p[j].addr);
      }
      else {
        str += S_DATA_SEP + "s" + S_TYPE_SEP;
        for (int k = 0; k < strlen((char *)args[i]->p[j].addr); k++) {
          str += String(*(char *)(args[i]->p[j].addr + k));
        }
      }
    }
    str += S_MSG_SEP;
  }
  str += S_SIGNAL_SEP + " ";
  return str;
}

//MASTER_FUNCTIONS------------------------------------------------------------

void sendFunctionCallString(String str) {
  char *csStr = (char *)malloc(str.length() * sizeof(char));
  str.toCharArray(csStr, str.length());
  Wire.beginTransmission(1);
  Wire.write(csStr);
  Wire.endTransmission();
  free(csStr);
}

void getDataFromSlaveReturn(pointer parsedValues[], String str) { 
    int functIndex = 0; 
    for (int i = 1; i < str.length() - 1; i++) { 
        if (str.charAt(i) == TYPE_SEP) { 
            int lastIndexOfReturn = i + 1; 
            while (lastIndexOfReturn < str.length() - 1 && str.charAt(lastIndexOfReturn + 1) != DATA_SEP) { 
              lastIndexOfReturn++; 
            } 
            char type = str.charAt(i - 1); 
            String returnStr = str.substring(i + 1, lastIndexOfReturn + 1); 
            char *csReturnStr = (char *)malloc(returnStr.length() * sizeof(char)); 
            returnStr.toCharArray(csReturnStr, returnStr.length() + 1); 
            parsedValues[functIndex] = parseValueFromCString(typeFromChar(type), csReturnStr); 
            if (type != 's') { 
                free(csReturnStr); 
            } 
            i = lastIndexOfReturn + 1; 
            functIndex++; 
        } 
    } 
}

pointer getDataFromMasterReturn(fReturnDataAsString returnData) {
  char *csReturnStr = (char *)malloc(returnData.str.length() * sizeof(char));
  returnData.str.toCharArray(csReturnStr, returnData.str.length() + 1);
  pointer returnAddress = parseValueFromCString(returnData.type, csReturnStr);
  if (returnData.type != STRING) {
    free(csReturnStr);
  }
  return returnAddress;
}

void runQueuedFunctions(fReturnDataAsString (*f[])(fArgumentList a[]), fArgumentList *args[], pointer results[], int n) {
  delay(100);
  double startTime = micros();
  if (digitalRead(SLAVE_SIGNAL_PIN) == HIGH) {
    Serial.println(">>Slave Connected: Running " + String(n - (int)floor(n / 2)) + " of " + String(n) + " Processes on Master");
    sendFunctionCallString(formatFunctionCallsAsString(args, floor(n / 2)));
    for (int i = floor(n / 2); i < n; i++) {
      results[i] = getDataFromMasterReturn(f[i](args[i]));
    }
    while (true) {  //Wait for slave to finish execution, then request return data
      if (digitalRead(RETURN_SIGNAL_PIN) == HIGH) {
        Wire.requestFrom(1, MAX_RETURN_SIZE);
        String ret = "";
        while (0 < Wire.available()) {
          ret += (char)Wire.read();
          if (ret.charAt(ret.length() - 1) == SIGNAL_SEP && ret.length() > 1) {
            break;
          }
        }
        int len = ret.length();
        if (ret.charAt(0) == SIGNAL_SEP && ret.charAt(ret.length() - 1) == SIGNAL_SEP) {
          getDataFromSlaveReturn(results, ret.substring(1, len - 1));
        }
        break;
      }
    }
  }
  else {
    Serial.println(">>Slave Not Found: Running All Processes on Master");
    for (int i = 0; i < n; i++) {
      results[i] = getDataFromMasterReturn(f[i](args[i]));
    }
  }
  Serial.println(">>Finished All Processes in " + String((micros() - startTime) / 1000000) + " Seconds");
}

fArgumentList *newArgumentList(int argCount, ...) {
  fArgumentList *args = (fArgumentList *)malloc(sizeof(fArgumentList) + argCount * sizeof(pointer));
  args->size = argCount;
  va_list va;
  va_start(va, argCount);
  for (int i = 0; i < argCount; i++) {
    args->p[i] = va_arg(va, pointer);
  }
  va_end(va);
  return args;
}

void freeReturnData(pointer returnData[], int n) {
  for (int i = 0; i < n; i++) {
    free(returnData[i].addr);
    returnData[i].addr = NULL;
  }
}

bool initializeParallelConnection() {
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(MASTER_ISSET_PIN, INPUT);
  Serial.begin(9600);
  while (true) {   //Wait until one tiva detects a serial connection to determine the master
    if (digitalRead(MASTER_ISSET_PIN) == HIGH) {
      Serial.println(">>Starting Parallel Processes as Slave");
      Wire.begin(SLAVE_ADDR);
      Wire.onReceive(receiveEvent);
      Wire.onRequest(requestEvent);
      isMaster = false;
      pinMode(RETURN_SIGNAL_PIN, OUTPUT);
      pinMode(SLAVE_SIGNAL_PIN, OUTPUT);
      digitalWrite(SLAVE_SIGNAL_PIN, HIGH);
      return isMaster;
    }
    else if (Serial.available() > 0) {
      Wire.begin();
      isMaster = true;
      Serial.println(">>Starting Parallel Processes as Master");
      pinMode(RETURN_SIGNAL_PIN, INPUT);
      pinMode(MASTER_ISSET_PIN, OUTPUT);
      digitalWrite(MASTER_ISSET_PIN, HIGH);
      pinMode(SLAVE_SIGNAL_PIN, INPUT);
      return isMaster;
    }
  }
}

//WIRE_EVENT_HANDLERS---------------------------------------------------------

//Called on the slave when a function call is sent from the master
void receiveEvent(int events) {  //int events is never used but is required for event handler
  String input = "";
  while (0 < Wire.available()) {
    input += String((char)Wire.read());
  }
  digitalWrite(BLUE_LED, HIGH);
  delay(100);
  digitalWrite(BLUE_LED, LOW);
  delay(100);
  if (input.charAt(0) == SIGNAL_SEP && input.charAt(input.length() - 1) == SIGNAL_SEP) {
    slaveReturn = S_SIGNAL_SEP;
    for (int i = 0; i < input.length(); i++) {
      if (input.charAt(i) == MSG_SEP) {
        int lastIndexOfFunctCall = i + 1;
        while (lastIndexOfFunctCall < input.length() - 1 && input.charAt(lastIndexOfFunctCall) != MSG_SEP) {
          lastIndexOfFunctCall++;
        }
        slaveReturn += callFunctionFromString(input.substring(i, lastIndexOfFunctCall + 1));
        i = lastIndexOfFunctCall;
      }
    }
    slaveReturn += S_SIGNAL_SEP + " ";
    digitalWrite(RETURN_SIGNAL_PIN, HIGH);
  }
}

//Called on the slave when a request for a function return value is made by the master
void requestEvent() {
  char *csSlaveReturn = (char *)malloc(slaveReturn.length() * sizeof(char));
  slaveReturn.toCharArray(csSlaveReturn, slaveReturn.length() + 1);
  Wire.write(csSlaveReturn);
  free(csSlaveReturn);
  digitalWrite(GREEN_LED, HIGH);
  delay(100);
  digitalWrite(GREEN_LED, LOW);
  delay(100);
  digitalWrite(RETURN_SIGNAL_PIN, LOW);
}
