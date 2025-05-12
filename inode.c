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
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1) {
        return -1;
    }

    int inodeBlock = ((inumber - 1) / (DISKIMG_SECTOR_SIZE / sizeof(struct inode))) + INODE_START_SECTOR;
    struct inode buffer[DISKIMG_SECTOR_SIZE / sizeof(struct inode)];

    int bytes = diskimg_readsector(fs->dfd, inodeBlock, buffer);
    if (bytes == -1) {
        return -1;
    }

    int inodeIndex = (inumber - 1) % (DISKIMG_SECTOR_SIZE / sizeof(struct inode));
    *inp = buffer[inodeIndex];

    // if ((inp->i_mode & IALLOC) == 0) {
    //     return -1;
    // }

    return 0;
}



int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
    if (!fs || !inp || blockNum < 0) {
        return -1;                         // parámetros inválidos
    }

    /* --------------- Caso 1: archivo pequeño (direcciones directas) --------------- */
    if ((inp->i_mode & ILARG) == 0) {      // bit ILARG apagado
        if (blockNum >= 8) {               // sólo caben 8 entradas directas
            return -1;
        }
        return inp->i_addr[blockNum];      // puede ser 0 si el bloque no existe
    }

    /* --------------- Caso 2: archivo grande -------------------------------------- */
    const int PTRS_PER_SECTOR = DISKIMG_SECTOR_SIZE / sizeof(uint16_t); // 256
    const int SINGLE_INDIRECT  = 7 * PTRS_PER_SECTOR;                   // 1792

    int fd = fs->dfd;

    /* ---- 2a. Bloques cubiertos por los 7 indirectos simples ---- */
    if (blockNum < SINGLE_INDIRECT) {
        int idx1   = blockNum / PTRS_PER_SECTOR;    // qué indirecto (0-6)
        int idx2   = blockNum % PTRS_PER_SECTOR;    // entrada dentro del indirecto

        uint16_t firstLevelBuf[PTRS_PER_SECTOR];
        int n = diskimg_readsector(fd, inp->i_addr[idx1], firstLevelBuf);
        if (n != DISKIMG_SECTOR_SIZE) return -1;

        return firstLevelBuf[idx2];                 // puede ser 0 si hueco
    }

    /* ---- 2b. Bloques cubiertos por el doblemente indirecto ---- */
    int rel = blockNum - SINGLE_INDIRECT;           // índice relativo dentro del doble
    int idx1 = rel / PTRS_PER_SECTOR;               // cuál de los 256 indirectos
    int idx2 = rel % PTRS_PER_SECTOR;               // entrada dentro del indirecto

    /* Primer nivel: leer el bloque doble indirecto */
    uint16_t dblBuf[PTRS_PER_SECTOR];
    int n1 = diskimg_readsector(fd, inp->i_addr[7], dblBuf); // i_addr[7] = doble
    if (n1 != DISKIMG_SECTOR_SIZE) return -1;

    uint16_t indirectSector = dblBuf[idx1];
    if (indirectSector == 0) return -1;             // hueco no asignado

    /* Segundo nivel: leer el bloque indirecto al que apunta la entrada anterior */
    uint16_t secondBuf[PTRS_PER_SECTOR];
    int n2 = diskimg_readsector(fd, indirectSector, secondBuf);
    if (n2 != DISKIMG_SECTOR_SIZE) return -1;

    return secondBuf[idx2];                         // 0 si el bloque no existe
}



int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
