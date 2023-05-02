#include <IOKit/IOKitLib.h>
#include <ApplicationServices/ApplicationServices.h>
#include "IcaDefine.h"
#include "DriverLib.h"
#include "ICALib.h"

/* ========================================================================== */
void AfficheSystemLog() {
    // This will launch the Console.app so you can see the IOLogs from the KEXT.
    CFURLRef pathRef;

    pathRef = CFURLCreateWithString(NULL, CFSTR("/var/log/system.log"), NULL);
    if (pathRef) {
        LSOpenCFURLRef(pathRef, NULL);
        CFRelease(pathRef);
    }
}
/* ========================================================================== */
int main(int argc, char* argv[]) {
    io_connect_t dataPort; kern_return_t rc;
	int adrs,val;

    AfficheSystemLog();
	dataPort = DriverConnect("ICADriver",ICA_OPEN);
	if((int)dataPort == 0) {
		printf("Erreur a la connection du driver (%s)\n",DriverError());
		return(0);
	}

    // This shows an example using IOConnectMethodScalarIScalarO().
	for(adrs = 0; adrs <= 0x24; adrs += 4) {
		if(!IcaConfigReadLong(&dataPort,adrs,&val))
			printf("IOConnectMethodScalarIScalarO(config_read) a retourne %d\n",IcaDerniereErreur());
		else printf("Configuration Register #%02X : %08X\n",adrs,val);
	}

	DriverDisconnect(dataPort,ICA_CLOSE);

	return(0);
}