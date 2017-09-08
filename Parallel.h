#ifndef PARALLEL_H
#define PARALLEL_H

enum type {
  INT,
  LONG,
  FLOAT,
  DOUBLE,
  CHAR,
  STRING
};

typedef struct {
  enum type type;
  String str;
} fReturnDataAsString;

typedef struct {
  enum type type;
  void *addr;
} pointer;

typedef struct {
  int size;
  pointer p[];
} fArgumentList;

void runQueuedFunctions(fReturnDataAsString (*f[])(fArgumentList *), fArgumentList *args[], pointer returnData[], int n);
fArgumentList *makeArgList(int n, ...);
void freeReturnData(pointer returnData[], int n);
bool initializeParallelConnection();

#endif
