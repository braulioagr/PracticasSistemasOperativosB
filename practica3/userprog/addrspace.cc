// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "iostream"

//********* se agregaron los siguientes headers
//********* para los archivos y concatenar el nombre con .swp

#include "string.h"
#include "fstream"

using namespace std;

//***********************************************************

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	//tree types of segments
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

// se le agrego el parametro @filename para poder acceder al nombre del archivo
AddrSpace::AddrSpace(OpenFile *executable, char *filename)
{
    NoffHeader noffH;
    unsigned int i, size;
    //variable para manejar el archivo swp
    ofstream swp;
    //agregandole al nombre del archivo la extension ".swp"
    strcat(filename, ".swp");
    //abriendo el archivo binario para escritura
    swp.open(filename, ofstream::binary);
    //leyendo el encabezado  
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);//asisgandole los valores correspondientes a noffH
	//SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
    						// to leave room for the stack
    if(swp.is_open())
    {
		int _length = size - UserStackSize - noffH.uninitData.size;
		char *buffer = new char[_length];
		executable->ReadAt(buffer, _length, 40);
		swp.write(buffer, _length);
		swp.close();
    }
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
    printf("--------------------------------------------------\n");
    printf("Algoritmo de Reemplazon FIFO\n");
    printf("Tamaño de proceso: %d\n",size);
    printf("Numero de paginas: %d\n",numPages);
    printf("--------------------------------------------------\n");
    if(!strcmp(comando,"-C"))
	{
		printf("Cadena de referencia\n");
	}
	else if(!strcmp(comando,"-M"))
	{
		printf("Dir.Virt\t vpn\t offset\t Dir.Fis\t\n");
	}
    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, size);
//  printf("Tabla: \nIndice	No.Marco	BitValidez\n");	

// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) { // i es el indice
//        printf("%d	", i);
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;	//marco
//	printf("	%d", pageTable[i].physicalPage);
	pageTable[i].valid = FALSE;	//se modifico el valor de bit de validez: de true a false
//	if(pageTable[i].valid)
//	    printf("		1");
//	else
//	    printf("    	0");
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
// 	printf("\n");
	}
    //printf("\nDir log.	No.Pagina	Desp.       Dir fis.\n");
    return;
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
	bzero(machine->mainMemory, size);
// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]), noffH.code.size, noffH.code.inFileAddr);
		//printf("memmoriaaa:%hhx\n", machine->mainMemory[0]);
		printf("code_addr: %d, size: %d\n", noffH.code.virtualAddr, noffH.code.size);
		//printf("Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
	}
		
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]), noffH.initData.size, noffH.initData.inFileAddr);
		printf("initdata_addr: %d, size: %d\n", noffH.initData.virtualAddr, noffH.initData.size);
		//printf("Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
	}
}

//metodo para cargar una pagina del archivo swp a memoria
//----------------------------------------------------------------------
bool 
AddrSpace::swapIn(int vpn)
{
	bool band;
	band = true;
	//file_name se agrego a machine
	OpenFile *swp = fileSystem->Open(machine->file_name);	//abrimos el archivo
	if(swp != NULL)
	{
		for(int i = 0 ; i < numPages ; i++)
		{
			if(pageTable[i].valid)
			{
				if(pageTable[i].physicalPage == indexFrame)
				{
					swapOut(i);
					break;
				}
			}
		}	
		pageTable[vpn].valid = TRUE;
		pageTable[vpn].physicalPage = indexFrame;
		swp->ReadAt(&(machine->mainMemory[indexFrame * PageSize]), PageSize, vpn * PageSize);
		indexFrame++;
		if (indexFrame >= NumPhysPages)
		{
			indexFrame = 0;
		}
		if(!strcmp(comando,"-F"))
		{
			printf("Numero de fallo: %d\n",stats->numPageFaults);
			printf("Pagina de fallo: %d\n", vpn);
			printf("-------------------------\n");
		}
		stats->numPageFaults++;
		stats->numDiskReads++;
		delete swp;	//cerramos el archivo
		
	}
	else
	{
		printf("swb no open\n");
		band = false;
	}
	
	return band;
}
//metodo para cargar una pagina de memoria al archivo swp 
//----------------------------------------------------------------------
bool AddrSpace::swapOut(int vpn)
{
	bool band;
	band = true;
	//file_name se agrego a machine
	OpenFile *swp = fileSystem->Open(machine->file_name);	//abrimos el archivo
	if(swp != NULL)
	{
		pageTable[vpn].valid = FALSE;
		if(pageTable[vpn].dirty)
		{
			swp->WriteAt(&(machine->mainMemory[pageTable[vpn].physicalPage * PageSize]), PageSize, vpn * PageSize);
			stats->numDiskWrites++;
		}
		pageTable[vpn].physicalPage = -1;
		//printf("%d, ", vpn);
		delete swp;	//cerramos el archivo
	}
	else
	{
		printf("swp no open\n");
		band = false;
	}
	return band;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
