#include <IOKit/IOLib.h>
#include "ICADriver.h"
#include "ICAClient.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define super IOUserClient 
OSDefineMetaClassAndStructors(ICAClient, IOUserClient)
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
	Parametrage des methodes : (indexees par l'index present dans IOConnectMethodxxxxyyyy)
		NULL                  : The (IOService *) will be determined at runtime below
		(IOMethod)&<methode>  : Pointeur sur la methode a utiliser
		kOIUC<type>I<type>O   :
			<type> : Scalar ou Struct (voir IOConnectMethodxxxxyyyy)
		<type>=Scalar:
			IOItemCount scalarCount : the number of scalar values (input si I, output si O)
		<type>=Structure:
			IOByteCount  structureSize : the size of the structure (input si I, output si O)
		** les valeurs et pointeurs des parametres ne sont pas references ici

	Combinaisons permises (voir IOConnectMethodxxxxyyyy):
		ScalarIScalarO
		ScalarIStructO
		ScalarIStructI
		StructIStructO
 
 Chargement dans le systeme:
	1/ cd ~gros
	2/ su root
	3/ exec tcsh -l
	-/ source .login
    5/ cd /Users/gros/$PROJECT_DIR/IcaDriver/build
	6/ chown -R root:wheel ICADriver.kext/
	7/ kextload -t ICADriver.kext/
*/
/* ========================================================================== */
IOExternalMethod *ICAClient::getTargetAndMethodForIndex(IOService **target, Ulong index) {
    static const IOExternalMethod methode[ICA_METHODES_NB] = {
        { NULL, (IOMethod)&ICAClient::open,            kIOUCScalarIScalarO, 0, 0 },
        { NULL,	(IOMethod)&ICAClient::close,           kIOUCScalarIScalarO, 0, 0 },
        { NULL,	(IOMethod)&ICADriver::ConfigRead32,    kIOUCScalarIScalarO, 1, 1 },
        { NULL, (IOMethod)&ICADriver::ConfigWrite32,   kIOUCScalarIScalarO, 2, 0 },
        { NULL,	(IOMethod)&ICADriver::ConfigRead16,    kIOUCScalarIScalarO, 1, 1 },
        { NULL, (IOMethod)&ICADriver::ConfigWrite16,   kIOUCScalarIScalarO, 2, 0 },
        { NULL,	(IOMethod)&ICADriver::ConfigRead8,     kIOUCScalarIScalarO, 1, 1 },
        { NULL, (IOMethod)&ICADriver::ConfigWrite8,    kIOUCScalarIScalarO, 2, 0 },

        { NULL, (IOMethod)&ICADriver::Setup,           kIOUCScalarIScalarO, 0, 0 },
        { NULL, (IOMethod)&ICADriver::RegisterUse,     kIOUCScalarIScalarO, 1, 0 },
        { NULL, (IOMethod)&ICADriver::UtilWrite,       kIOUCScalarIScalarO, 1, 0 },
        { NULL, (IOMethod)&ICADriver::RegWrite,        kIOUCScalarIScalarO, 2, 0 },
        { NULL, (IOMethod)&ICADriver::RegRead,         kIOUCScalarIScalarO, 1, 1 },
        { NULL, (IOMethod)&ICADriver::CommandSend,     kIOUCScalarIScalarO, 4, 0 },
        { NULL, (IOMethod)&ICADriver::InstrWrite,      kIOUCScalarIScalarO, 2, 0 },
        { NULL, (IOMethod)&ICADriver::FifoReset,       kIOUCScalarIScalarO, 0, 0 },
        { NULL, (IOMethod)&ICADriver::FifoEmpty,       kIOUCScalarIScalarO, 1, 1 },
        { NULL, (IOMethod)&ICADriver::FifoRead,        kIOUCScalarIScalarO, 0, 1 },
        { NULL, (IOMethod)&ICAClient::XferInit,        kIOUCScalarIScalarO, 1, 0 },
        { NULL, (IOMethod)&ICAClient::XferSizeSet,     kIOUCScalarIScalarO, 1, 0 },
        { NULL, (IOMethod)&ICAClient::XferExec,        kIOUCScalarIScalarO, 0, 0 },
        { NULL, (IOMethod)&ICADriver::XferDone,        kIOUCScalarIScalarO, 0, 1 },
        { NULL, (IOMethod)&ICADriver::XferStatusGet,   kIOUCScalarIScalarO, 0, 3 },
        { NULL, (IOMethod)&ICADriver::XferStatusReset, kIOUCScalarIScalarO, 0, 0 },
        { NULL, (IOMethod)&ICADriver::DmaDispo,        kIOUCScalarIScalarO, 0, 1 },
   };

#ifdef DEBUG
    IOLog("ICAClient::getTargetAndMethodForIndex(index = %d)\n", (int)index);
#endif
    // Make sure that the index of the function we're calling actually exists in the function table.
    if(index < (Ulong)ICA_METHODES_NB) {
		// These methods exist in ICAClient, so return a pointer to ICAClient.
        if((index == ICA_OPEN) || (index == ICA_CLOSE)
		|| (index == ICA_XFER_SIZE_SET) || (index == ICA_XFER_EXEC)) *target = this;	   
		// These methods exist in ICADriver, so return a pointer to ICADriver.
        else *target = (IOService *)driver;
        return (IOExternalMethod *)&methode[index];
    }
	return(NULL);
}
/* ========================================================================== */
IOReturn ICAClient::XferInit(int avec_dma) {
	IOLog("ICAClient::XferInit(%s DMA) FIFO @%08X + %08X - sans objet sous MacOsX\n",
		avec_dma? "avec": "sans",(hexa)fifo,(hexa)dim);
//	return(driver->XferInit(avec_dma,dim,fifo));
	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICAClient::XferSizeSet(int taille) {
	IOReturn rc;

// #ifdef DEBUG
	IOLog("ICAClient::XferSizeSet(%d)\n",taille);
// #endif
	rc = kIOReturnSuccess;
	if(taille == 0) dim = CLUZEL_FIFO_INTERNE;
	else if(taille <= CLUZEL_FIFO_INTERNE) dim = taille;
	else rc = kIOReturnBadArgument;
	return(rc);
}
/* ========================================================================== */
IOReturn ICAClient::XferExec() {
#ifdef DEBUG
	IOLog("ICAClient::XferExec() FIFO @%08X + %08X\n",(hexa)fifo,(hexa)dim);
#endif
	return(driver->XferExec(dim,fifo));
//	return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICAClient::clientMemoryForType(Ulong type, IOOptionBits *options, IOMemoryDescriptor **desc ) {
	/* Appele (en 4eme) par l'application */
	IOReturn rc;

#ifdef DEBUG
    IOLog("ICAClient::clientMemoryForType(...)\n");
#endif
	switch( type ) {
	  case ICA_MEM_INTERNE:
		*desc  = fifo_desc;
		rc = kIOReturnSuccess;
		break;
 	  case ICA_ACCES_PCI:
		*desc  = driver->PCIGetDesc();
		rc = kIOReturnSuccess;
		break;
	  default:
		rc = kIOReturnBadArgument;
		break;
	}

    return(rc);
}
/* ========================================================================== */
bool ICAClient::initWithTask(task_t owningTask,void *security_id , Ulong type) {
	/* Appele en 1er (avant start) */
//#ifdef DEBUG
    IOLog("ICAClient::initWithTask()\n");
//#endif
    if(!super::initWithTask(owningTask, security_id , type)) return(false);
    if(!owningTask) return(false);
    fTask = owningTask;
    driver = NULL;
    fDead = false;
	dim = 0;
    return(true);
}
/* ========================================================================== */
bool ICAClient::start(IOService * provider) {
	/* Appele en 2eme */
//#ifdef DEBUG
    IOLog("ICAClient::start()\n");
//#endif
    if(!super::start(provider)) return(false);
    driver = (ICADriver *)OSDynamicCast(ICADriver, provider);
    if (!driver) return(false);

	dim = CLUZEL_FIFO_INTERNE;
	fifo_desc = IOBufferMemoryDescriptor::withOptions(kIOMemoryKernelUserShared,dim);
	if(!fifo_desc) return(false);
	fifo_desc->retain();
	fifo = (Ulong *)fifo_desc->getBytesNoCopy();
//#ifdef DEBUG
	IOLog("ICAClient::start,  FIFO @%08X + %08X\n",(hexa)fifo,(hexa)dim);
//#endif
    return(true);
}
/* ========================================================================== */
IOReturn ICAClient::open(void) {
	/* Appele en 3eme */
//#ifdef DEBUG
    IOLog("ICAClient::open()\n");
//#endif
    
    // If we don't have an driver, this user client isn't going to do much, so return kIOReturnNotAttached.
    if (!driver) return(kIOReturnNotAttached);
    // Call driver->open, and if it fails, it means someone else has already opened the device.
    if (!driver->open(this)) return(kIOReturnExclusiveAccess);
//#ifdef DEBUG
	IOLog("ICAClient::open,  FIFO @%08X + %08X\n",(hexa)fifo,(hexa)dim);
//#endif
	driver->XferInit(1,dim,fifo);
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICAClient::message(Ulong type, IOService * provider,  void * argument) {
#ifdef DEBUG
    IOLog("ICAClient::message()\n");
#endif
    switch(type) {
        default:
            break;
    }
    return(super::message(type, provider, argument));
}
/* ========================================================================== */
IOReturn ICAClient::close(void) {
#ifdef DEBUG
    IOLog("ICAClient::close()\n");
#endif
    // If we don't have an driver, then we can't really call the driver's close() function, so just return.
    if (!driver) return(kIOReturnNotAttached);
    // Make sure the driver is open before we tell it to close.
    if (driver->isOpen(this)) driver->close(this);
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICAClient::clientClose(void) {
#ifdef DEBUG
    IOLog("ICAClient::clientClose()\n");
#endif
    // release my hold on my parent (if I have one).
    close();
    if (fTask) fTask = NULL;
    driver = NULL;
    terminate();
    // DONT call super::clientClose, which just returns notSupported
    return(kIOReturnSuccess);
}
/* ========================================================================== */
IOReturn ICAClient::clientDied(void) {
    IOReturn ret = kIOReturnSuccess;
	
#ifdef DEBUG
    IOLog("ICAClient::clientDied()\n");
#endif
    fDead = true;
    // this just calls clientClose
    ret = super::clientDied();
    return(ret);
}
/* ========================================================================== */
bool ICAClient::terminate(IOOptionBits options) {
    bool ret;
    
#ifdef DEBUG
    IOLog("ICAClient::terminate()\n");
#endif
    ret = super::terminate(options);
    return(ret);
}
/* ========================================================================== */
bool  ICAClient::finalize(IOOptionBits options) {
    bool ret;
    
#ifdef DEBUG
    IOLog("ICAClient::finalize()\n");
#endif
    ret = super::finalize(options);
    return(ret);
}
/* ========================================================================== */
void ICAClient::stop(IOService * provider) {
#ifdef DEBUG
    IOLog("ICAClient::stop()\n");
#endif
    super::stop(provider);
}
