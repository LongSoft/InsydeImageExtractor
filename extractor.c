/*  extractor.c - InsydeFlash BIOS image extractor
    Author: Nikolaj Schlej 
    License: WTFPL
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define ERR_SUCCESS             0
#define ERR_NOT_FOUND           1
#define ERR_FILE_OPEN           2
#define ERR_FILE_READ           3
#define ERR_FILE_WRITE          4
#define ERR_INVALID_PARAMETER   5
#define ERR_OUT_OF_MEMORY       6

const uint8_t IFLASH_BIOSIMG_SIGNATURE[] = { 
    0x24, 0x5F, 0x49, 0x46, 0x4C, 0x41, 0x53, 0x48, 0x5F, 0x42, 0x49, 0x4F,
    0x53, 0x49, 0x4D, 0x47
}; // "$_IFLASH_BIOSIMG"
#define IFLASH_BIOSIMG_SIGNATURE_LENGTH 16 

typedef struct _IFLASH_BIOSIMG_HEADER {
    uint8_t  Signature[16];
    uint32_t FullSize;
    uint32_t UsedSize;
} IFLASH_BIOSIMG_HEADER;

/* Implementation of GNU memmem function using Boyer-Moore-Horspool algorithm
*  Returns pointer to the beginning of found pattern or NULL if not found */
uint8_t* find_pattern(uint8_t* begin, uint8_t* end, const uint8_t* pattern, size_t plen)
{
    size_t scan = 0;
    size_t bad_char_skip[256];
    size_t last;
    size_t slen;

    if (plen == 0 || !begin || !pattern || !end || end <= begin)
        return NULL;

    slen = end - begin;

    for (scan = 0; scan <= 255; scan++)
        bad_char_skip[scan] = plen;

    last = plen - 1;

    for (scan = 0; scan < last; scan++)
        bad_char_skip[pattern[scan]] = last - scan;

    while (slen >= plen)
    {
        for (scan = last; begin[scan] == pattern[scan]; scan--)
            if (scan == 0)
                return begin;

        slen     -= bad_char_skip[begin[last]];
        begin   += bad_char_skip[begin[last]];
    }

    return NULL;
}

/* Entry point */
int main(int argc, char* argv[])
{
    FILE*    in_file;
    FILE*    out_file;
    uint8_t* in_buffer;
    uint8_t* end;
    uint8_t* found;
    long filesize;
    long read;
    IFLASH_BIOSIMG_HEADER* header;

    /* Check for arguments count */
    if (argc < 3)
    {
        printf("InsydeFlashExtractor v0.1\n\nUsage: extractor INFILE OUTFILE");
        return ERR_INVALID_PARAMETER;
    }

    /* Open input file */
    in_file = fopen(argv[1], "r+b");
    if(!in_file)
    {
        perror("Input file can't be opened");
        return ERR_FILE_OPEN;
    }

    /* Get input file size */
    fseek(in_file, 0, SEEK_END);
    filesize = ftell(in_file);
    fseek(in_file, 0, SEEK_SET);

    /* Allocate memory for input buffer */
    in_buffer = (uint8_t*)malloc(filesize);
    if (!in_buffer)
    {
        perror("Can't allocate memory for input file");
        return ERR_OUT_OF_MEMORY;
    }
    
    /* Read whole input file to input buffer */
    read = fread((void*)in_buffer, sizeof(char), filesize, in_file);
    if (read != filesize)
    {
        perror("Can't read input file");
        return ERR_FILE_READ;
    }
    end = in_buffer + filesize - 1;

    /* Search for signature in file */
    found = find_pattern(in_buffer, end, IFLASH_BIOSIMG_SIGNATURE, IFLASH_BIOSIMG_SIGNATURE_LENGTH);
    if (!found)
    {
        printf("Insyde BIOS image signature not found in input file");
        return ERR_NOT_FOUND;
    }

    /* Populate header and read used size */
    header = (IFLASH_BIOSIMG_HEADER*) found;
    found += sizeof(IFLASH_BIOSIMG_HEADER);
    filesize = header->UsedSize;

    /* Open output file */
    out_file = fopen(argv[2], "wb");
    if (!out_file)
    {
        perror("Output file can't be opened");
        return ERR_FILE_OPEN;
    }

    /* Write BIOS image to output file */
    read = fwrite(found, sizeof(char), filesize, out_file);
    if (read != filesize)
    {
        perror("Can't write output file");
        return ERR_FILE_WRITE;
    }
    
    /* All done */
    printf("File %s successfully extracted", argv[2]);
    return ERR_SUCCESS;
}