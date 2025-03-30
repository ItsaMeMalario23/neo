#ifndef RESPONSE_H
#define RESPONSE_H

#include <main.h>

void parseJSession(const char* rxbuf, size_t size, char* ssnbuf);
int  parseSSNTokenViewstate(const char* rxbuf, size_t size, char* ssntbuf, char* vstabuf);
void parseViewstate(const char* rxbuf, size_t size, char* vstabuf);
void parseFieldVals(const char* rxbuf, size_t size, char** fields, size_t fldsize, char** fldbuf);

void printFields(char** fields, size_t fldsize);
void freeFields(char** fields, size_t fldsize);

#endif