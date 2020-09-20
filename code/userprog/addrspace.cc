#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

int AddrSpace::mark = 0;

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

// copy the contents of the buffer into the mainMemory
// index gives the starting point from which the buffer needs to be copied. 
void
AddrSpace::cpToMemory(char *buff, int index){
	int end = index + PageSize;
	int x = index, y = 0;
	while(x<end){
		kernel->machine->mainMemory[x] = buff[y];
		x++; 
		y++;
	}
}

// function to get the page table entry from thr vpn
TranslationEntry
AddrSpace::getVPNEntry(int vpn){
	return pageTable[vpn];
}

//function to get the page table entry from the ppn
TranslationEntry
AddrSpace::getPPNEntry(int ppn){
	TranslationEntry tEntry;
	int x = 0;
	while(x < numPages){
		if(pageTable[x].physicalPage == ppn){
			return pageTable[x];
		}
		x++;
	}
	return tEntry; 
}

//  update the page table entry of a particular page 
void 
AddrSpace::upgradeEntryOfPage(int vpn, int pageNum, bool valid, bool use){
	pageTable[vpn].physicalPage = pageNum;
	pageTable[vpn].valid = valid;
	pageTable[vpn].use = use; 
}

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
#ifdef RDATA
    noffH->readonlyData.size = WordToHost(noffH->readonlyData.size);
    noffH->readonlyData.virtualAddr = 
           WordToHost(noffH->readonlyData.virtualAddr);
    noffH->readonlyData.inFileAddr = 
           WordToHost(noffH->readonlyData.inFileAddr);
#endif 
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);

#ifdef RDATA
    DEBUG(dbgAddr, "code = " << noffH->code.size <<  
                   " readonly = " << noffH->readonlyData.size <<
                   " init = " << noffH->initData.size <<
                   " uninit = " << noffH->uninitData.size << "\n");
#endif
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

#ifdef RDATA
// how big is address space?
    size = noffH.code.size + noffH.readonlyData.size + noffH.initData.size +
           noffH.uninitData.size + UserStackSize;	
                                                // we need to increase the size
						// to leave room for the stack
#else
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
#endif
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

    pageTable = new TranslationEntry[numPages];
    for (int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;   // for now, virt page # = phys page #
        pageTable[i].swpLocation = kernel->getSwpCount(); 
	pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; 
	
	char *charBuff = new char[PageSize];
	executable->ReadAt(charBuff, PageSize, (i*PageSize)+noffH.code.inFileAddr);

	pageTable[i].physicalPage = Kernel::pageSetMap->FindAndSet();
	if(pageTable[i].physicalPage != -1){
		kernel->pMap[pageTable[i].physicalPage] = kernel->currentThread;
		kernel->phyPages.Append(pageTable[i].physicalPage);
		pageTable[i].valid = TRUE; 

		int strtIndex = PageSize * (pageTable[i].physicalPage);
		cpToMemory(charBuff,strtIndex);
	} 
	else{
		pageTable[i].valid = FALSE;
	}
	kernel->swapFile->WriteAt(charBuff, PageSize, pageTable[i].swpLocation*PageSize);
    }
    
    // zero out the entire address space
    //bzero(&kernel->machine->mainMemory[mark * PageSize], size);

// then, copy in the code and data segments into memory
// Note: this code assumes that virtual address = physical address
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        executable->ReadAt(
		&(kernel->machine->mainMemory[noffH.initData.virtualAddr + mark * PageSize]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }

#ifdef RDATA
    if (noffH.readonlyData.size > 0) {
        DEBUG(dbgAddr, "Initializing read only data segment.");
	DEBUG(dbgAddr, noffH.readonlyData.virtualAddr << ", " << noffH.readonlyData.size);
        executable->ReadAt(
		&(kernel->machine->mainMemory[noffH.readonlyData.virtualAddr + mark * PageSize]),
			noffH.readonlyData.size, noffH.readonlyData.inFileAddr);
    }
#endif

    mark += numPages;
    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void 
AddrSpace::Execute() 
{

    kernel->currentThread->space = this;

    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
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
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start", which
    //  is assumed to be virtual address zero
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    // Since instructions occupy four bytes each, the next instruction
    // after start will be at virtual address four.
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
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
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}


//----------------------------------------------------------------------
// AddrSpace::Translate
//  Translate the virtual address in _vaddr_ to a physical address
//  and store the physical address in _paddr_.
//  The flag _isReadWrite_ is false (0) for read-only access; true (1)
//  for read-write access.
//  Return any exceptions caused by the address translation.
//----------------------------------------------------------------------
ExceptionType
AddrSpace::Translate(unsigned int vaddr, unsigned int *paddr, int isReadWrite)
{
    TranslationEntry *pte;
    int               pfn;
    unsigned int      vpn    = vaddr / PageSize;
    unsigned int      offset = vaddr % PageSize;

    if(vpn >= numPages) {
        return AddressErrorException;
    }

    pte = &pageTable[vpn];

    if(isReadWrite && pte->readOnly) {
        return ReadOnlyException;
    }

    pfn = pte->physicalPage;

    // if the pageFrame is too big, there is something really wrong!
    // An invalid translation was loaded into the page table or TLB.
    if (pfn >= NumPhysPages) {
        DEBUG(dbgAddr, "Illegal physical page " << pfn);
        return BusErrorException;
    }

    pte->use = TRUE;          // set the use, dirty bits

    if(isReadWrite)
        pte->dirty = TRUE;

    *paddr = pfn*PageSize + offset;

    ASSERT((*paddr < MemorySize));

    //cerr << " -- AddrSpace::Translate(): vaddr: " << vaddr <<
    //  ", paddr: " << *paddr << "\n";

    return NoException;
}




