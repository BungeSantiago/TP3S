#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#define INODE_START_SECTOR  2                           /* sector donde arrancan los i-nodos */
#define SECTOR_SIZE          DISKIMG_SECTOR_SIZE     /* 512 bytes por sector */
#define INODES_PER_SECTOR    (SECTOR_SIZE / sizeof(struct inode))
#define SECTOR_SIZE        DISKIMG_SECTOR_SIZE
#define PTRS_PER_BLOCK ((int)(SECTOR_SIZE / sizeof(uint16_t)))
#define INODE_DIRECT       8    /* número total de punteros en i_addr[] */
#define INODE_SMALL_DIRECT 8    /* para archivos no ILARG usan los 8 directos */
#define INODE_LARGE_DMIN   6    /* directos antes de indirecto simple */
#define INODE_LARGE_SIND   6    /* índice de indirecto simple */
#define INODE_LARGE_DIND   7    /* índice de indirecto doble */


/**
 * TODO
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    /* 1) Total de i-nodos disponibles según el superbloque */
    int total_inodes = fs->superblock.s_isize * INODES_PER_SECTOR;

    /* 2) Validación de rango [1 .. total_inodes] */
    if (inumber < 1 || inumber > total_inodes) {
        return -1;
    }

    /* 3) Ajuste a índice base-0 */
    int idx = inumber - 1;

    /* 4) Cálculo de sector y desplazamiento dentro */
    int sector = INODE_START_SECTOR + (idx / INODES_PER_SECTOR);
    int offset = idx % INODES_PER_SECTOR;

    /* 5) Buffer local de lectura */
    struct inode buf[INODES_PER_SECTOR];

    /* 6) Leer todo el sector de i-nodos */
    if (diskimg_readsector(fs->dfd, sector, buf) < 0) {
        return -1;
    }

    /* 7) Copiar el i-nodo solicitado */
    *inp = buf[offset];

    return 0;
}

/**
 * TODO
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp,
    int blockNum) {  
    /* 1) Calcular cuántos bloques tiene el fichero */
    uint32_t filesize = ((uint32_t)inp->i_size0 << 16)
                      | (uint32_t)inp->i_size1;
    int total_blocks = (filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (blockNum < 0 || blockNum >= total_blocks)
        return -1;

    /* 2) Casos de fichero pequeño (sin ILARG) */
    if (!(inp->i_mode & ILARG)) {
        if (blockNum < INODE_SMALL_DIRECT)
            return inp->i_addr[blockNum];
        else
            return -1;
    }

    /* 3) Fichero grande: directos, indirecto simple, indirecto doble */
    /* 3a) Punteros directos (0..5) */
    if (blockNum < INODE_LARGE_DMIN) {
        return inp->i_addr[blockNum];
    }

    /* 3b) Indirecto simple (bloques 6 .. 6+PTRS_PER_BLOCK-1) */
    if (blockNum < INODE_LARGE_DMIN + PTRS_PER_BLOCK) {
        uint16_t buf[PTRS_PER_BLOCK];
        int err = diskimg_readsector(fs->dfd,
                                     inp->i_addr[INODE_LARGE_SIND],
                                     buf);
        if (err < 0) return -1;
        return buf[blockNum - INODE_LARGE_DMIN];
    }

    /* 3c) Indirecto doble */
    {
        int idx = blockNum - (INODE_LARGE_DMIN + PTRS_PER_BLOCK);
        int first_idx = idx / PTRS_PER_BLOCK;
        int second_idx = idx % PTRS_PER_BLOCK;

        /* Leer bloque de punteros a bloques indirectos simples */
        uint16_t buf1[PTRS_PER_BLOCK];
        if (diskimg_readsector(fs->dfd,
                               inp->i_addr[INODE_LARGE_DIND],
                               buf1) < 0)
            return -1;

        uint16_t indirect_blk = buf1[first_idx];

        /* Leer bloque indirecto simple concreto */
        uint16_t buf2[PTRS_PER_BLOCK];
        if (diskimg_readsector(fs->dfd,
                               indirect_blk,
                               buf2) < 0)
            return -1;

        return buf2[second_idx];
    }
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
