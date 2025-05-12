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

    // /* -------- verificar que el inodo esté asignado ---------- */
    // if ((inp->i_mode & IALLOC) == 0) {
    //     return -1;                         // inodo libre
    // }

    return 0;                              // éxito
}



int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if ((inp->i_mode & ILARG) == 0) {
        if (blockNum < 0 || blockNum >= 8) {
            return -1;
        }
        return inp->i_addr[blockNum];
    } else {
        // ILARG = archivo grande, usar bloques indirectos
        if (blockNum < 0 || blockNum >= 7 * 256 + 256 * 256) {
            return -1;
        }

        int indirectIndex = blockNum / 256;
        int indirectOffset = blockNum % 256;

        if (indirectIndex < 7) {
            uint16_t indirectBlock[256];
            int indirectBlockNum = inp->i_addr[indirectIndex];
        
            if (diskimg_readsector(fs->dfd, indirectBlockNum, indirectBlock) < 0) {
                return -1;
            }
        
            return indirectBlock[indirectOffset];
        } else {
            uint16_t doubleIndirectBlock[256];
            int doubleIndirectBlockNum = inp->i_addr[7];
        
            if (diskimg_readsector(fs->dfd, doubleIndirectBlockNum, doubleIndirectBlock) < 0) {
                return -1;
            }
        
            int first = (blockNum - 7 * 256) / 256;
            int second = (blockNum - 7 * 256) % 256;
        
            uint16_t indirectBlock[256];
            if (diskimg_readsector(fs->dfd, doubleIndirectBlock[first], indirectBlock) < 0) {
                return -1;
            }
        
            return indirectBlock[second];
        }
    }
}



int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
