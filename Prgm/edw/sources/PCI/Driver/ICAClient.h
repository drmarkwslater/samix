#ifndef ICACLIENT_H
#define ICACLIENT_H

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include "ICADriver.h"

class ICAClient : public IOUserClient {
    OSDeclareDefaultStructors(ICAClient)
    
private:
    ICADriver *driver;
    task_t	   fTask;
    bool       fDead;
	Ulong     dim;
	IOBufferMemoryDescriptor *fifo_desc;
	Ulong    *fifo;

public:
	IOReturn XferInit(int avec_dma);
	IOReturn XferSizeSet(int taille);
	IOReturn XferExec();

	// IOService overrides
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);

    // IOUserClient overrides
    virtual IOReturn  open(void);
    virtual IOReturn  close(void);

    virtual IOReturn message(Ulong type, IOService *provider,  void *argument = 0);
    
    virtual bool initWithTask(task_t owningTask, void *security_id, Ulong type);
    virtual bool finalize(IOOptionBits options);

    virtual IOExternalMethod *getTargetAndMethodForIndex(IOService **target, Ulong index);
	virtual IOReturn clientMemoryForType(Ulong type, IOOptionBits *options, IOMemoryDescriptor **desc );
	virtual IOReturn clientClose(void);
    virtual IOReturn clientDied(void);
    virtual bool terminate(IOOptionBits options = 0);    
};

#endif
