#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"


int file_getblock(struct unixfilesystem *fs,
    int inumber, int blockNum, void *buf){

    if (!fs || !buf || inumber < 1 || blockNum < 0) return -1;

    struct inode in;
    if (inode_iget(fs, inumber, &in) < 0) return -1;

    int sector = inode_indexlookup(fs, &in, blockNum);
    if (sector < 0)                       // bloque ausente
    return -1;

    /* leer el sector (o rellenar de ceros si sector == 0) */
    if (sector == 0) {
    memset(buf, 0, DISKIMG_SECTOR_SIZE);
    } else {
    if (diskimg_readsector(fs->dfd, sector, buf) != DISKIMG_SECTOR_SIZE)
    return -1;
    }

    /* bytes válidos dentro de este bloque */
    int filesize   = inode_getsize(&in);
    int firstByte  = blockNum * DISKIMG_SECTOR_SIZE;
    int remaining  = filesize - firstByte;

    return (remaining >= DISKIMG_SECTOR_SIZE) ?
    DISKIMG_SECTOR_SIZE : remaining;
}



