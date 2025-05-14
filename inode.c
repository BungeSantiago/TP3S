#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#define INDIR_ADDR 7

/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
    if (!fs || !inp) return -1;                         // punteros inválidos

    if (inumber < 1) return -1;

    const int INODES_PER_SECTOR = DISKIMG_SECTOR_SIZE / sizeof(struct inode);

    uint32_t max_inodes = fs->superblock.s_isize * INODES_PER_SECTOR;
    if ((uint32_t)inumber > max_inodes) {
        return -1;                         // fuera de rango
    }

    int idx        = inumber - 1;          // 0-based
    int sectorOff  = idx / INODES_PER_SECTOR;
    int entryOff   = idx % INODES_PER_SECTOR;
    int sectorNum  = INODE_START_SECTOR + sectorOff;

    struct inode sectorBuf[INODES_PER_SECTOR];
    int nread = diskimg_readsector(fs->dfd, sectorNum, sectorBuf);
    if (nread != DISKIMG_SECTOR_SIZE) {    // incluye nread < 0
        return -1;                         // I/O error
    }

    *inp = sectorBuf[entryOff];

    return 0;                              // éxito
}



int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
    if (!fs || !inp || blockNum < 0) return -1;                         // parámetros inválidos

    if ((inp->i_mode & ILARG) == 0) {      // bit ILARG apagado
        if (blockNum >= 8) {               // sólo caben 8 entradas directas
            return -1;
        }
        return inp->i_addr[blockNum];      // puede ser 0 si el bloque no existe
    }

    const int PTRS_PER_SECTOR = DISKIMG_SECTOR_SIZE / sizeof(uint16_t); 
    const int SINGLE_INDIRECT  = 7 * PTRS_PER_SECTOR;                   

    int fd = fs->dfd;

    if (blockNum < SINGLE_INDIRECT) {
        int idx1   = blockNum / PTRS_PER_SECTOR;    // qué indirecto (0-6)
        int idx2   = blockNum % PTRS_PER_SECTOR;    // entrada dentro del indirecto

        uint16_t firstLevelBuf[PTRS_PER_SECTOR];
        int n = diskimg_readsector(fd, inp->i_addr[idx1], firstLevelBuf);
        if (n != DISKIMG_SECTOR_SIZE) return -1;

        return firstLevelBuf[idx2];                 // puede ser 0 si hueco
    }

    int rel = blockNum - SINGLE_INDIRECT;           // índice relativo dentro del doble
    int idx1 = rel / PTRS_PER_SECTOR;               // cuál de los 256 indirectos
    int idx2 = rel % PTRS_PER_SECTOR;               // entrada dentro del indirecto

    uint16_t dblBuf[PTRS_PER_SECTOR];
    int n1 = diskimg_readsector(fd, inp->i_addr[7], dblBuf); // i_addr[7] = doble
    if (n1 != DISKIMG_SECTOR_SIZE) return -1;

    uint16_t indirectSector = dblBuf[idx1];
    if (indirectSector == 0) return -1;             // hueco no asignado

    uint16_t secondBuf[PTRS_PER_SECTOR];
    int n2 = diskimg_readsector(fd, indirectSector, secondBuf);
    if (n2 != DISKIMG_SECTOR_SIZE) return -1;

    return secondBuf[idx2];                         // 0 si el bloque no existe
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
