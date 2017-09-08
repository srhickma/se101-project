#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <Wire.h>
#include <stdbool.h>
#include "Parallel.h"

fReturnDataAsString approximatePI1(fArgumentList *);
fReturnDataAsString approximatePI2(fArgumentList *);
fReturnDataAsString celebrate(fArgumentList *);

fReturnDataAsString (*functions[3])(fArgumentList *) = {approximatePI1, approximatePI2, celebrate};

void setup() {
  if (initializeParallelConnection()) {
    pinMode(RED_LED, OUTPUT);
    randomSeed(analogRead(0));
    long iters = 999999;
    int celebrateTimeInterval = 9;
    
    fArgumentList *approxPI1Args = newArgumentList(1, (pointer){LONG, (void *) &iters});
    fArgumentList *approxPI2Args = newArgumentList(1, (pointer){LONG, (void *) &iters});
    fArgumentList *celebrateArgs = newArgumentList(2, (pointer){INT, (void *) &celebrateTimeInterval}, 
                                             (pointer){STRING, (void *) "Done Calculating and Celebrating"});
    fArgumentList *args[] = {approxPI1Args, approxPI2Args, celebrateArgs};
    pointer returnData[3];
    runQueuedFunctions(functions, args, returnData, 3);
    
    Serial.print("PI from approximatePI1: ");
    Serial.println(*(double *)returnData[0].addr / 10000000 , 9);
    Serial.print("PI from approximatePI2: ");
    Serial.println((double)(*(long *)returnData[1].addr) / (double)iters * 4.0, 9);
    Serial.println((char *)returnData[2].addr);
    
    freeReturnData(returnData, 3);
    free(approxPI1Args);
    free(approxPI2Args);
    free(celebrateArgs);
  }
}

void loop() {}

fReturnDataAsString approximatePI1(fArgumentList *args) {
  long n = *(long *)(args->p[0].addr);
  double sum = 1.0;
  double y = 3.0;
  double z;

  for (long i = 0; i < n; i++) {
    z = 1.0 / y;
    if ((i & 1) == 0) {
      sum -= z;
    }
    else {
      sum += z;
    }
    y += 2.0;
  }
  double pi = 4.0 * sum;
  
  //The String() object truncates values past the 2nd decimal, so
  return (fReturnDataAsString){DOUBLE, String(pi * 10000000)}; 
}

fReturnDataAsString approximatePI2(fArgumentList *args) {
  long n = *(long *)(args->p[0].addr);
  long inside = 0;
  for (long i = 0; i < n; i++) {
    long x = random(2000) - 1000;
    long y = random(2000) - 1000;
    if (x * x + y * y <= 1000000) {
      inside++;
    }
  }
  return (fReturnDataAsString){LONG, String(inside)};
}

fReturnDataAsString celebrate(fArgumentList *args) {
  int celebrateTime = *(int *)(args->p[0].addr);
  char *msg = (char *)(args->p[1].addr);
  for (int i = 0; i < celebrateTime * 10; i++) {
    int x = random(3);
    if (x == 0) {
      digitalWrite(RED_LED, HIGH);
    }
    else if (x == 1) {
      digitalWrite(GREEN_LED, HIGH);
    }
    else {
      digitalWrite(BLUE_LED, HIGH);
    }
    delay(100);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
  }
  return (fReturnDataAsString){STRING, String(msg) + " For " + String(celebrateTime) + " Seconds!"};
}
