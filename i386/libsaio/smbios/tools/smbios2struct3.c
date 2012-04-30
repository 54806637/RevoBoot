/***
  *
  * Name        : smbios2struct3 (pipe output to file)
  * Version     : 1.0.5
  * Type        : Command line tool
  * Description : SMBIOS extractor / converter (resulting in a smaller and more Apple like table).
  *
  * Copyright   : DutchHockeyPro (c) 2011
  *
  * Compile with: cc -I . smbios2struct3.c -o smbios2struct3 -Wall -framework IOKit -framework CoreFoundation
  *
  */

#include <stdio.h>
#include <stdlib.h>

#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>

#include "SMBIOS.h"

#define DEBUG		0	// Set to 1 for additional (debug) output
#define VERBOSE		1

#if VERBOSE
	#define VERBOSE_DUMP(x...) printf(x)
#else
	#define VERBOSE_DUMP(x...)
#endif

#define	round2(x, m)	(((x) + (m / 2)) & ~(m - 1)) // Taken from saio_types.h


//==============================================================================

char tableDescriptions[133][36] = 
{
	/*   0 */ "BIOS Information", 
	/*   1 */ "System Information", 
	/*   2 */ "Base Board or Module Information", 
	/*   3 */ "System Enclosure or Chassis", 
	/*   4 */ "Processor Information", 
	/*   5 */ "Memory Controller Information", 
	/*   6 */ "Memory Module Information", 
	/*   7 */ "Cache Information", 	
	/*   8 */ "Port Connector Information",
	/*   9 */ "System Slots",
	/*  10 */ "On Board Devices Information",
	/*  11 */ "OEM Strings",
	/*  12 */ "System Configuration Options",
	/*  13 */ "BIOS Language Information",
	/*  14 */ "Group Associations",
	/*  15 */ "System Event Log",
	/*  16 */ "Physical Memory Array",
	/*  17 */ "Memory Device",
	/*  18 */ "32-bit Memory Error Information",
	/*  19 */ "Memory Array Mapped Address",
	/*  20 */ "Memory Device Mapped Address",
	/*  21 */ "Built-in Pointing Device",
	/*  22 */ "Portable Battery",
	/*  23 */ "System Reset",
	/*  24 */ "Hardware Security",
	/*  25 */ "System Power Controls",
	/*  26 */ "Voltage Probe",
	/*  27 */ "Cooling Device",
	/*  28 */ "Temperature Probe",
	/*  29 */ "Electrical Current Probe",
	/*  30 */ "Out-of-Band Remote Access",
	/*  31 */ "Boot Integrity Services",
	/*  32 */ "System Boot Information",
	/*  33 */ "64-bit Memory Error Information",
	/*  34 */ "Management Device",
	/*  35 */ "Management Device Component",
	/*  36 */ "Management Device Threshold Data",
	/*  37 */ "Memory Channel",
	/*  38 */ "IPMI Device Information",
	/*  39 */ "System Power Supply",
	/*  40 */ "Additional Information",
	/*  41 */ "Onboard Devices Extended Information",
	/*  42 */ "", "", "", "", "", "", "", "",
	/*  50 */ "", "", "", "", "", "", "", "", "", "",
	/*  60 */ "", "", "", "", "", "", "", "", "", "",
	/*  70 */ "", "", "", "", "", "", "", "", "", "",
	/*  80 */ "", "", "", "", "", "", "", "", "", "",
	/*  90 */ "", "", "", "", "", "", "", "", "", "",
	/* 100 */ "", "", "", "", "", "", "", "", "", "",
	/* 110 */ "", "", "", "", "", "", "", "", "", "",
	/* 120 */ "", "", "", "", "", "",
	/* 126 */ "Inactive",
	/* 127 */ "End-of-Table",
	/* 128 */ "",
	/* 129 */ "",
	/* 130 */ "Memory SPD Data",
	/* 131 */ "OEM Processor Type",
	/* 132 */ "OEM Processor Bus Speed"
};


//==============================================================================

void dumpStaticTableData(const UInt8 * tableBuffer, int maxStructureSize, int structureCount, int tableLength)
{
	int index = 0;

	printf("\n\t#define	STATIC_SMBIOS_SM_MAX_STRUCTURE_SIZE\t%d\n", maxStructureSize);
	printf("\n\t#define	STATIC_SMBIOS_DMI_STRUCTURE_COUNT\t%d\n", structureCount);
	printf("\n");
	printf("\t#define STATIC_SMBIOS_DATA \\\n");
	printf("\t/* SMBIOS data (0x%04x / %d bytes) converted with smbios2struct2 into little endian format. */ \\\n", tableLength, tableLength);
	printf("\t/* 0x0000 */\t");
	
	UInt16 length = round2(tableLength, 4);
	UInt32 * newTableData = malloc(tableLength);
	bcopy((UInt32 *)tableBuffer, newTableData, tableLength);
	
	do
	{
		printf("0x%08x", (unsigned int)newTableData[index++]);
		
		if ((index * 4) <= length)
		{
			printf(", ");
			
			if ((index % 8) == 0)
			{
				printf("\\\n\t/* 0x%04x */\t", (index * 4));
			}
		}
	} while((index * 4) <= length);
	
	printf("\n");
	
	free(newTableData);
}


//==============================================================================

int main(int argc, char * argv[])
{
	mach_port_t		masterPort;
    io_service_t	service = MACH_PORT_NULL;
	CFDataRef		dataRef;

	UInt16 maxStructureSize, structureCount = 0;
	UInt16 tableLength = 0, droppedTables = 0;
	UInt16 newTableLength = 0;

	// UInt32 * shadowTableData = NULL;

	IOMasterPort(MACH_PORT_NULL, &masterPort);
	service = IOServiceGetMatchingService(masterPort, IOServiceMatching("AppleSMBIOS"));

    if (service)
	{
#if DEBUG
		printf("\nHave AppleSMBIOS\n");
#endif
		dataRef = (CFDataRef) IORegistryEntryCreateCFProperty(service, 
															  CFSTR("SMBIOS-EPS"), 
															  kCFAllocatorDefault, 
															  kNilOptions);

		if (dataRef)
		{
			// We could use isA_CFData(dataRef) {} here, but that would force us to include 
			// another file, which in my opinion would be overkill (dataRef should always be there).
#if DEBUG
			printf("Have SMBIOS-EPS dataRef\n\n");
#endif
			struct SMBEntryPoint * eps = (struct SMBEntryPoint *) CFDataGetBytePtr(dataRef);
			tableLength = eps->dmi.tableLength;

			/* shadowTableData = malloc(tableLength);
			bzero(shadowTableData, tableLength);
			bcopy((void *)(unsigned int)eps->dmi.tableAddress, shadowTableData, tableLength); */
#if DEBUG
			// _SM_ 
			eps->anchor[4] = 0; // Prevent garbage output.
			printf("eps.anchor            : %s\n", eps->anchor);
			printf("eps.checksum          : 0x02%x\n", eps->checksum);
			printf("eps.entryPointLength  : 0x%x\n", eps->entryPointLength);
			printf("eps.majorVersion      : 0x%02x\n", eps->majorVersion);
			printf("eps.minorVersion      : 0x%02x\n", eps->minorVersion);
#endif
			maxStructureSize = eps->maxStructureSize;
#if DEBUG
			printf("eps.maxStructureSize  : 0x%04x\n", maxStructureSize);
			printf("eps.entryPointRevision: 0x%02x\n", eps->entryPointRevision);
			printf("eps.formattedArea     : %s\n\n", eps->formattedArea);

			// _DMI_
			eps->dmi.anchor[5] = 0; // Prevent garbage output.
			printf("eps.dmi.anchor        : %s\n", eps->dmi.anchor);
			printf("eps.dmi.checksum      : 0x%02x\n", eps->dmi.checksum);
			printf("eps.dmi.tableLength   : 0x%04x\n", tableLength); // eps->dmi.tableLength);
			printf("eps.dmi.tableAddress  : 0x%08x\n", (unsigned int)eps->dmi.tableAddress);
#endif
			structureCount = eps->dmi.structureCount;
#if DEBUG
			printf("eps.dmi.structureCount: 0x%04x\n", structureCount);
			printf("eps.dmi.bcdRevision   : 0x%x\n", eps->dmi.bcdRevision);
#endif
			// Done with dataRef / release it.
			CFRelease(dataRef);
		}
#if DEBUG			
		printf("\n");
#endif

		dataRef = (CFDataRef) IORegistryEntryCreateCFProperty(service, 
															  CFSTR("SMBIOS"), 
															  kCFAllocatorDefault, 
															  kNilOptions);

		if (dataRef)
		{
			// We could use isA_CFData(dataRef) {} here, but that would force us to include another
			// header file, which in my opinion would be overkill as dataRef should always be there.
#if DEBUG
			printf("Have SMBIOS dataRef\n");
#endif
			UInt32 * tableData = (UInt32 *) CFDataGetBytePtr(dataRef);

#if VERBOSE
			UInt16 numBytes = (int) CFDataGetLength(dataRef);
			printf("\nNumber of bytes: %d (Original SMBIOS table)\n", numBytes);
#endif

			SMBStructHeader * header;
			const UInt8 * tablePtr = (const UInt8 *) tableData;
			const UInt8 * tableEndPtr  = tablePtr + tableLength;
			const UInt8 * tableStructureStart = 0;
			const UInt8 * droppedTableStructureStart = 0;
			const UInt8 * tableBuffer = malloc(tableLength);

			// int index				= 0;
			int maxStructureSize	= 0;
			int newStructureCount	= 0;
			int structureLength		= 0;

			SMBWord newHandle = 0;

			bzero((void *)tableBuffer, sizeof(tableBuffer));

			while (structureCount-- && (tableEndPtr > tablePtr + sizeof(SMBStructHeader)))
			{
				droppedTableStructureStart = tableStructureStart = 0;

				header = (SMBStructHeader *) tablePtr;

				if (header->length > tableEndPtr - tablePtr)
				{
					break;
				}
				
				switch (header->type)
				{
					case kSMBTypeBIOSInformation:		// Type 0
					case kSMBTypeSystemInformation:		// Type 1
					case kSMBTypeBaseBoard:				// Type 2
					case kSMBTypeProcessorInformation:	// Type 4
					// case kSMBTypeMemoryModule:		// Type 6
					// case kSMBTypeSystemSlot:			// Type 9
					case kSMBTypePhysicalMemoryArray:	// Type 16
					case kSMBTypeMemoryDevice:			// Type 17
					case kSMBTypeEndOfTable:			// Type 127
					case kSMBTypeFirmwareVolume:		// Type 128
					case kSMBTypeMemorySPD:				// Type 130
					case kSMBTypeOemProcessorType:		// Type 131
					case kSMBTypeOemProcessorBusSpeed:	// Type 132

						newStructureCount++;
						tableStructureStart = tablePtr;
						header->handle = newHandle++;
						// printf("type: %d, handle: %d\n", header->type, header->handle);
						break;

					default:
						droppedTables++;
						droppedTableStructureStart = tablePtr;
				}
				
				VERBOSE_DUMP("Table: %3d %37s  @%p  Formatted area: %2d bytes  ", header->type, tableDescriptions[header->type], tablePtr, header->length);

				// Skip the formatted area of the structure.
				tablePtr += header->length;
				
				// Skip the unformatted structure area at the end (strings).
				for (; tableEndPtr > tablePtr + sizeof(SMBStructHeader); tablePtr++)
				{
					// Look for a terminating double NULL.
					if (tablePtr[0] == 0 && tablePtr[1] == 0)
					{
						tablePtr += 2;
						break;
					}
				}
				
				if (tableStructureStart)
				{
					structureLength = tablePtr - tableStructureStart;

					VERBOSE_DUMP("Structure length: %3d bytes.\n", structureLength);

					bcopy((void *)tableStructureStart, (void *)tableBuffer + newTableLength, structureLength);
					newTableLength += structureLength;
					
					// Taking care of eps->maxStructureSize
					if (maxStructureSize < structureLength)
					{
						maxStructureSize = structureLength;
					}
				}
				else if (droppedTableStructureStart)
				{
					structureLength = tablePtr - droppedTableStructureStart;
					VERBOSE_DUMP("Structure length: %3d bytes (dropped).\n", structureLength);
				}
			}
			
			VERBOSE_DUMP("\nDropped tables: %2d.\n", droppedTables);

			dumpStaticTableData(tableBuffer, maxStructureSize, newStructureCount, newTableLength);

			free((void *)tableBuffer);

			// Done with dataRef / release it.
			CFRelease(dataRef);
		}

		// Done with the service / release it.
		IOObjectRelease(service);
    }

	exit(0);
}
