#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#define INDIR_ADDR  7


/**
 * TODO
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (!fs || !inp) {
        return -1;                         // punteros inválidos
    }

    /* -------- rango válido del número de inodo ---------- */
    if (inumber < 1) {
        return -1;                         // inode 0 no existe
    }

    const int INODES_PER_SECTOR = DISKIMG_SECTOR_SIZE / sizeof(struct inode);

    /* Máximo número de inodo presente en el disco */
    uint32_t max_inodes = fs->superblock.s_isize * INODES_PER_SECTOR;
    if ((uint32_t)inumber > max_inodes) {
        return -1;                         // fuera de rango
    }

    /* -------- localizar el sector y la entrada dentro del sector ---------- */
    int idx        = inumber - 1;          // 0-based
    int sectorOff  = idx / INODES_PER_SECTOR;
    int entryOff   = idx % INODES_PER_SECTOR;
    int sectorNum  = INODE_START_SECTOR + sectorOff;

    struct inode sectorBuf[INODES_PER_SECTOR];
    int nread = diskimg_readsector(fs->dfd, sectorNum, sectorBuf);
    if (nread != DISKIMG_SECTOR_SIZE) {    // incluye nread < 0
        return -1;                         // I/O error
    }

    /* -------- copiar la entrada solicitada ---------- */
    *inp = sectorBuf[entryOff];

    /* -------- verificar que el inodo esté asignado ---------- */
    // if ((inp->i_mode & IALLOC) == 0) {
    //     return -1;                         // inodo libre
    // }

    return 0;                              // éxito	
}


int inode_indexlookup(struct unixfilesystem *fs,
    struct inode *inp, int blockNum)
{
if (!fs || !inp || blockNum < 0) return -1;

/* --- nº de bloques que realmente existen en este archivo --- */
int filesize = inode_getsize(inp);                    // bytes
int fileblocks = (filesize + DISKIMG_SECTOR_SIZE-1) /
   DISKIMG_SECTOR_SIZE;
if (blockNum >= fileblocks)
return -1;                                        // fuera de rango

const int PTRS_PER_SECTOR = DISKIMG_SECTOR_SIZE / sizeof(uint16_t); // 256

/* ------------- archivos “chicos” — 8 direcciones directas ------------- */
if ((inp->i_mode & ILARG) == 0) {
uint16_t phys = inp->i_addr[blockNum];
return phys ? phys : -1;                          // 0 ⇒ hueco sin asignar
}

/* ------------- archivos “grandes” — 7 indirectos + 1 doble ------------- */
const int SINGLE_MAX = 7 * PTRS_PER_SECTOR;           // 1792 bloques
int fd = fs->dfd;

if (blockNum < SINGLE_MAX) {          /* ➊ — indirectos simples */
int l1 = blockNum / PTRS_PER_SECTOR;  // 0-6
int l2 = blockNum % PTRS_PER_SECTOR;

uint16_t table[PTRS_PER_SECTOR];
if (inp->i_addr[l1] == 0) return -1;
if (diskimg_readsector(fd, inp->i_addr[l1], table) != DISKIMG_SECTOR_SIZE)
return -1;

return table[l2] ? table[l2] : -1;
}

/* ➋ — doble indirecto */
int rel   = blockNum - SINGLE_MAX;
int l1    = rel / PTRS_PER_SECTOR;
int l2    = rel % PTRS_PER_SECTOR;

uint16_t dbl[PTRS_PER_SECTOR];
if (inp->i_addr[7] == 0) return -1;
if (diskimg_readsector(fd, inp->i_addr[7], dbl) != DISKIMG_SECTOR_SIZE)
return -1;

if (dbl[l1] == 0) return -1;          // agujero no asignado

uint16_t ind[PTRS_PER_SECTOR];
if (diskimg_readsector(fd, dbl[l1], ind) != DISKIMG_SECTOR_SIZE)
return -1;

return ind[l2] ? ind[l2] : -1;
}



int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
