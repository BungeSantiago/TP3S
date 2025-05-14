#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber,
    int blockNum, void *buf)
{
    if (!fs || !buf || inumber < 1 || blockNum < 0) return -1;

    struct inode in;
    if (inode_iget(fs, inumber, &in) < 0) return -1;

    int sector = inode_indexlookup(fs, &in, blockNum);
    if (sector <= 0) return -1;


    int nread = diskimg_readsector(fs->dfd, sector, buf);
    if (nread != DISKIMG_SECTOR_SIZE) return -1;

    int totalBytes  = inode_getsize(&in);
    int firstByte   = blockNum * DISKIMG_SECTOR_SIZE;
    if (firstByte >= totalBytes) return -1;

    int remaining   = totalBytes - firstByte;
    int nValid      = (remaining >= DISKIMG_SECTOR_SIZE) ? DISKIMG_SECTOR_SIZE : remaining;

    return nValid;                          // éxito
}



