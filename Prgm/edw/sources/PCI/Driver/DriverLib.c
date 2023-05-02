#include <stdio.h>
#include "DriverLib.h"

static char DriverMessage[80];

/* ========================================================================== */
io_connect_t DriverConnect(char *nom, int open_method) {
    kern_return_t	kernResult; 
    mach_port_t		masterPort;
    io_service_t	serviceObject;
    io_connect_t	dataPort;
    io_iterator_t 	iterator;
    CFDictionaryRef	classToMatch;

	DriverMessage[0] = '\0';
    // Returns the mach port used to initiate communication with IOKit.
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);    
	if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOMasterPort returned %d", kernResult);
        return(0);
    }
    classToMatch = IOServiceMatching(nom);
	if (classToMatch == NULL) {
        sprintf(DriverMessage,"IOServiceMatching returned a NULL dictionary");
        return(0);
    }
    // This creates an io_iterator_t of all instances of our drivers class that exist in the IORegistry.
    kernResult = IOServiceGetMatchingServices(masterPort, classToMatch, &iterator);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOServiceGetMatchingServices returned %d", kernResult);
        return(0);
    }
    // Get the first item in the iterator.
    serviceObject = IOIteratorNext(iterator);
    // Release the io_iterator_t now that we're done with it.
    IOObjectRelease(iterator);
    if (!serviceObject) {
        sprintf(DriverMessage,"Couldn't find any matches");
        return(0);
    }
    // This call will cause the user client to be instantiated.
    kernResult = IOServiceOpen(serviceObject, mach_task_self(), 0, &dataPort);
    // Release the io_service_t now that we're done with it.
    IOObjectRelease(serviceObject);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOServiceOpen returned %d", kernResult);
        return(0);
    }
    // This shows an example of calling a user client's open method.
    kernResult = IOConnectMethodScalarIScalarO(dataPort, open_method, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOConnectMethodScalarIScalarO(open) returned %d", kernResult);
        // This closes the connection to our user client and destroys the connect handle.
        IOServiceClose(dataPort);
		return(0);
    }
	return(dataPort);
}
/* ========================================================================== */
int DriverScan(char *nom, int open_method, io_connect_t *carte, int *nb) {
    kern_return_t	kernResult; 
    mach_port_t		masterPort;
    io_service_t	serviceObject;
    io_connect_t	dataPort;
    io_iterator_t 	iterator;
    CFDictionaryRef	classToMatch;
	
	DriverMessage[0] = '\0';
    // Returns the mach port used to initiate communication with IOKit.
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);    
	if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOMasterPort returned %d", kernResult);
        return(0);
    }
    classToMatch = IOServiceMatching(nom);
	if (classToMatch == NULL) {
        sprintf(DriverMessage,"IOServiceMatching returned a NULL dictionary");
        return(0);
    }
    // This creates an io_iterator_t of all instances of our drivers class that exist in the IORegistry.
    kernResult = IOServiceGetMatchingServices(masterPort, classToMatch, &iterator);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOServiceGetMatchingServices returned %d", kernResult);
        return(0);
    }
	*nb = 0;
	do {
		// Get the next item in the iterator.
		serviceObject = IOIteratorNext(iterator);
		if (!serviceObject) break;
		// This call will cause the user client to be instantiated.
		kernResult = IOServiceOpen(serviceObject, mach_task_self(), 0, &dataPort);
		// Release the io_service_t now that we're done with it.
		IOObjectRelease(serviceObject);
		if (kernResult != KERN_SUCCESS) {
			printf("(DriverScan) Sur l'objet #%d, IOServiceOpen a rendu %d\n",*nb,kernResult);
			continue;
		}
		// This shows an example of calling a user client's open method.
		kernResult = IOConnectMethodScalarIScalarO(dataPort, open_method, 0, 0);
		if (kernResult != KERN_SUCCESS) {
			printf("(DriverScan) Sur l'objet #%d, IOConnectMethodScalarIScalarO(open) a rendu %d",*nb,kernResult);
			// This closes the connection to our user client and destroys the connect handle.
			IOServiceClose(dataPort);
			continue;
		}
		carte[*nb] = dataPort; *nb += 1;
	} while(1);
	// Release the io_iterator_t now that we're done with it.
	IOObjectRelease(iterator);
	return(*nb);
}
/* ========================================================================== */
char *DriverLastMessage() { return(DriverMessage); }
/* ========================================================================== */
kern_return_t DriverDisconnect(io_connect_t dataPort, int close_method) {
	kern_return_t kernResult;

    // This shows an example of calling a user client's close method.
    kernResult = IOConnectMethodScalarIScalarO(dataPort, close_method, 0, 0);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOConnectMethodScalarIScalarO(close) returned %d", kernResult);
		return(kernResult);
    }

    // This closes the connection to our user client and destroys the connect handle.
    kernResult = IOServiceClose(dataPort);
    if (kernResult != KERN_SUCCESS) {
        sprintf(DriverMessage,"IOServiceClose returned %d", kernResult);
    }
	return(kernResult);
}
/* ========================================================================== */
char *DriverError() { return(DriverMessage); }
