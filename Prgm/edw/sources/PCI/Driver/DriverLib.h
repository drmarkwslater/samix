#ifndef DRIVERLIB_H
#define DRIVERLIB_H

#include <IOKit/IOKitLib.h>

io_connect_t DriverConnect(char *nom, int open_method);
int DriverScan(char *nom, int open_method, io_connect_t *carte, int *nb);
char *DriverLastMessage();
kern_return_t DriverDisconnect(io_connect_t dataPort, int close_method);
char *DriverError();

#endif
