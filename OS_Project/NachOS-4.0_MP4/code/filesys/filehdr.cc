// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	singleIndirectSector = -1;
	doubleIndirectSector = -1;
	numBytes = 0;
	numSectors = 0;
	memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{ 
    int newSize = numBytes + fileSize;
 //   numBytes = fileSize;
   // numSectors  = divRoundUp(fileSize, SectorSize);
    int newSectors = divRoundUp(fileSize, SectorSize);

    if (freeMap->NumClear() < numSectors){
	printf("not enough space\n");
	return FALSE;		// not enough space
    }
    this->AllocateDirectBlocks(freeMap, fileSize);
    if(newSize <= (NumDirect * SectorSize)) return TRUE;

    this->CreateSingleIndirectBlock(freeMap, newSize);

    int singleAllocated = this->AllocateIndirectSpace(freeMap, newSize, (NumDirect * SectorSize), singleIndirectSector);
    if(singleAllocated == 0) return TRUE;
/*
    for (int i = 0; i < numSectors; i++) {
	dataSectors[i] = freeMap->FindAndSet();
	// since we checked that there was enough free space,
	// we expect this to succeed
	ASSERT(dataSectors[i] >= 0);
    }
*/ 
    this->AllocateDoubleIndirectBlock(freeMap, newSize);
    return TRUE;
}

void 
FileHeader::AllocateDirectBlocks(PersistentBitmap *freeMap, int fileSize)
{
    while(numBytes >= 0 && numBytes < (NumDirect * SectorSize))
    {
	if(fileSize <= (numSectors * SectorSize))
	{
	    numBytes = fileSize;
	    break;
	}
	else
	{
	    numBytes = (numSectors * SectorSize);
	    if(numSectors < NumDirect)
	    {
		dataSectors[numSectors++] = freeMap->FindAndSet();
	    }
	}
    }

}

void
FileHeader::CreateSingleIndirectBlock(PersistentBitmap *freeMap, int fileSize)
{
    if(singleIndirectSector <= 0)
    {
	Indirect* singleIndirect = new Indirect;
	singleIndirectSector = freeMap->FindAndSet();
	singleIndirect->numSectors = 0;
	WriteBack(singleIndirectSector, (char*) singleIndirect);
    }
}

int
FileHeader::AllocateIndirectSpace(PersistentBitmap *freeMap, int fileSize, int start, int sector)
{
    int end = start + NumIndirect * SectorSize;
    Indirect* indirect = new Indirect();
    
    FetchFrom(sector, (char*) indirect);

    while(numBytes >= start && numBytes < end)
    {
	if(fileSize <= numSectors * SectorSize)
	{
	    numBytes = fileSize;
	    break;
	}
	else
	{
	    numBytes = numSectors * SectorSize;
	    if(indirect->numSectors < NumIndirect)
	    {
		indirect->dataSectors[indirect->numSectors++] = freeMap->FindAndSet();
		numSectors++;
	    }
	}
    }
    WriteBack(sector, (char*) indirect);
    return fileSize - numBytes;
}

void
FileHeader::AllocateDoubleIndirectBlock(PersistentBitmap *freeMap, int fileSize)
{
    int allocated = -1, currentIndirect = 0;

    Indirect *doubleIndirect = new Indirect();
    if(doubleIndirectSector <= 0)
    {
	doubleIndirectSector = freeMap->FindAndSet();
	doubleIndirect->numSectors = 0;
	WriteBack(doubleIndirectSector, (char*)doubleIndirect);	
    }
    else
    {
	FetchFrom(doubleIndirectSector, (char*)doubleIndirect);
    }
    while(allocated != 0)
    {
	Indirect * singleIndirect = new Indirect();
	
	if(doubleIndirect->dataSectors[currentIndirect] <= 0)
	{
	    int indSector = freeMap->FindAndSet();
	    singleIndirect->numSectors = 0;
	    doubleIndirect->dataSectors[doubleIndirect->numSectors++] = indSector;
	    WriteBack(doubleIndirectSector, (char*) doubleIndirect);
	    WriteBack(indSector, (char*) singleIndirect);
	}
	else
	{
	    FetchFrom(doubleIndirect->dataSectors[currentIndirect], (char*)singleIndirect);
	}
	
	int start = (NumDirect * SectorSize) + (NumIndirect * SectorSize) + (currentIndirect * (NumIndirect *SectorSize));
	
	allocated = AllocateIndirectSpace(freeMap, fileSize, start, doubleIndirect->dataSectors[currentIndirect]);
    }    
}
void 
FileHeader::SetSector(int sector)
{
    headSector = sector;
}

int
FileHeader::GetSector()
{
    return headSector;
}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void
FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    for(int i = 0; i < numSectors; i++)
    {
	int s = GetSectorPhysicalAddress(i);
	freeMap->Clear(s);
    }
    if(singleIndirectSector > 0)
	if(freeMap->Test(singleIndirectSector))
	    freeMap->Clear(singleIndirectSector);

    if(doubleIndirectSector > 0)
	if(freeMap->Test(doubleIndirectSector)) freeMap->Clear(doubleIndirectSector);

}


//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

void 
FileHeader::FetchFrom(int sector, char* data)
{
    kernel->synchDisk->ReadSector(sector, data);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

void
FileHeader::WriteBack(int sector, char* data)
{
    kernel->synchDisk->WriteSector(sector, data);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    //return dataSectors[offset/SectorSize];
    int localSector = offset / SectorSize;

    return GetSectorPhysicalAddress(localSector);
}

int 
FileHeader::GetSectorPhysicalAddress(int localSector)
{
    if(localSector < NumDirect) return(dataSectors[localSector]);
    else if (localSector < (NumDirect + NumIndirect))
    {
        ASSERT(singleIndirectSector != 0);
        ASSERT(localSector < (NumDirect + NumIndirect));
        Indirect* singleIndirect = new Indirect;
        FetchFrom(singleIndirectSector, (char *)singleIndirect);
        return singleIndirect->dataSectors[localSector - NumDirect];
    }
    else {
    
        ASSERT(doubleIndirectSector != 0);
        ASSERT(localSector >= (NumDirect + NumIndirect));
    
        Indirect* doubleIndirect = new Indirect;

        FetchFrom(doubleIndirectSector, (char *)doubleIndirect);
    
        int single = (localSector - (NumDirect + NumIndirect))/NumIndirect;

        Indirect* ind = new Indirect;
        FetchFrom(doubleIndirect->dataSectors[single], (char *)ind);
        int pos = (localSector - (NumDirect + NumIndirect)) % NumIndirect;

        return ind->dataSectors[pos];
    }
}
//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}
