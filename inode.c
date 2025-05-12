#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"

#define INODE_START_SECTOR  2                           /* sector donde arrancan los i-nodos */
#define SECTOR_SIZE          DISKIMG_SECTOR_SIZE     /* 512 bytes por sector */
#define INODES_PER_SECTOR    (SECTOR_SIZE / sizeof(struct inode))
#define INDIR_ADDR  7


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
    	int fd = fs->dfd;
	int is_small_file = ((inp->i_mode & ILARG) == 0);

	// if it is a small file
	if(is_small_file) {
		return inp->i_addr[blockNum];
	}	

	// if it is a large file
	int addr_num = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);
	int indir_addr_num = addr_num * INDIR_ADDR;
	if(blockNum < indir_addr_num) {		// if it only uses INDIR_ADDR
		int sector_offset = blockNum / addr_num;
		int addr_offset = blockNum % addr_num;
		uint16_t addrs[addr_num];
		int err = diskimg_readsector(fd, inp->i_addr[sector_offset], addrs);
		if(err < 0) return -1;	
		return addrs[addr_offset];
	} else {							// if it also uses the DOUBLE_INDIR_ADDR
		// the first layer
		int blockNum_in_double = blockNum - indir_addr_num;
		int sector_offset_1 = INDIR_ADDR;
		int addr_offset_1 = blockNum_in_double / addr_num;
		uint16_t addrs_1[addr_num];
		int err_1 = diskimg_readsector(fd, inp->i_addr[sector_offset_1], addrs_1);
		if(err_1 < 0) return -1;

		// the second layer
		int sector_2 = addrs_1[addr_offset_1];
		int addr_offset_2 = blockNum_in_double % addr_num;
		uint16_t addrs_2[addr_num];
		int err_2 = diskimg_readsector(fd, sector_2, addrs_2);
		if(err_2 < 0) return -1;
		return addrs_2[addr_offset_2];
	}	
}

int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
