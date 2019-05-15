// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void Copy(char *from, char *to) {
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
        printf("Copy: couldn't open input file %s\n", from);
        return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
    	printf("Copy: couldn't create output file %s\n", to);
    	fclose(fp);
    	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);	
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void Print(char *name) {
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
        printf("Print: unable to open file %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0){
        for (i = 0; i < amountRead; i++){
            printf("%c", buffer[i]);
        }
        delete [] buffer;
    }
    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 5000))

static void FileWrite() {
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
        FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0)) {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10) {
            printf("Perf test: unable to write %s\n", FileName);
            delete openFile;
            return;
        }
    }
delete openFile;	// close file
}

static void FileRead() {
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
        FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
        printf("Perf test: unable to open file %s\n", FileName);
        delete [] buffer;
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
        if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
            printf("Perf test: unable to read %s\n", FileName);
            delete openFile;
            delete [] buffer;
            return;
        }
    }
    delete [] buffer;
    delete openFile;	// close file
}

void PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FileName))
    {
     printf("Perf test: unable to remove %s\n", FileName);
     return;
    }
    stats->Print();
}

void Rename(char* fileName, char* newFileName)
{
    printf("Renombrar archivo de %s a %s\n", fileName, newFileName);
    fileSystem->Rename(fileName,newFileName);
}

void PrintHDSectors()
{
    printf("Imprimir sectores del disco duro libres\n");
    fileSystem->FreeSectorsDisk();
}

void PrintFileSectors(char *fileName)
{
    printf("Imprime los sectores del archivo %s\n", fileName);
    if (!fileSystem->FileSectors(fileName)) {
        printf("Archivo no encontrado\n");
    }
}

void Help()
{
    printf("Comandos\n");
    printf("\t-f\n");
    printf("\t Descripción:\n");
    printf("\t\t Hace que el disco físico sea formateado\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -f\n");

    printf("\t-cp\n");
    printf("\t Descripción:\n");
    printf("\t\t Copia un archivo de UNIX a Nachos\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -cp <Nombree del archivo en UNIX> <Nombree del archivo en NachOs>\n");

    printf("\t-p\n");
    printf("\t Descripción:\n");
    printf("\t\t Imprime un archivo Nachos a la salida estándar\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -p\n");

    printf("\t-r\n");
    printf("\t Descripción:\n");
    printf("\t\t Elimina un archivo Nachos del sistema de archivos\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -r <Nombre del Archivo>\n");

    printf("\t-l\n");
    printf("\t Descripción:\n");
    printf("\t\t Enumera los contenidos del directorio de Nachos\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -l\n");

    printf("\t-D\n");
    printf("\t Descripción:\n");
    printf("\t\t Imprime el contenido de todo el sistema de archivos\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -D\n");

    printf("\t-t\n");
    printf("\t Descripción:\n");
    printf("\t\t Prueba el rendimiento del sistema de archivos Nachos\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -t\n");

    printf("\t-sd\n");
    printf("\t Descripción:\n");
    printf("\t\t Despliega el número de los sectores de disco duro que están libres\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -sd\n");

    printf("\t-sa\n");
    printf("\t Descripción:\n");
    printf("\t\t Despliega los sectores que tiene asignado un archivo\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -sa <Nombre del Archivo>\n");

    printf("\t-rn\n");
    printf("\t Descripción:\n");
    printf("\t\t Permite renombrar un archivo");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -rn <Nombre del Archivo><Nuevo nombre>\n");

    printf("\t-h\n");
    printf("\t Descripción:\n");
    printf("\t\t Visualiza el manual del sistema de archivos de NachOs\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -h\n");

    printf("\t-inf\n");
    printf("\t Descripción:\n");
    printf("\t\t Muestra la información de los desarrolladores\n");
    printf("\t Sintaxis:\n");
    printf("\t\t $./nachos -inf\n");
}

void Info()
{
    printf("Información\n");
    printf("\tIntegrantes:\n");
    printf("\t\t Oscar Armando Gonzalez Patiño\n");
    printf("\t\t Alan Alejandro Ortiz Diaz\n");
    printf("\t\t Braulio Alejandro García Rivera\n");
    printf("\tMateria:\n");
    printf("\t\t Sistemas Operativos B\n");
    printf("\tSemestre:\n");
    printf("\t\t 2018-2019-II\n");
    printf("\tMaestra:\n");
    printf("\t\t Marcela Ortiz Hernández\n");
}
