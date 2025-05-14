#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int directory_findname(struct unixfilesystem *fs, const char *name,
  int dirinumber, struct direntv6 *dirEnt)
{
  if (!fs || !name || !dirEnt || dirinumber < 1) return -1;

  size_t namelen = strlen(name);
  if (namelen == 0 || namelen > 14) return -1;

  struct inode dinode;
  if (inode_iget(fs, dirinumber, &dinode) < 0) return -1;

  if ((dinode.i_mode & IFMT) != IFDIR) return -1;

  int dirSize   = inode_getsize(&dinode);
  int nBlocks   = (dirSize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
  char buf[DISKIMG_SECTOR_SIZE];

  for (int blk = 0; blk < nBlocks; blk++) {
    int nValid = file_getblock(fs, dirinumber, blk, buf);
    if (nValid < 0) return -1;                         /* error de lectura */

    int nEntries = nValid / sizeof(struct direntv6);
    for (int i = 0; i < nEntries; i++) {
      struct direntv6 *entry = (struct direntv6 *)(buf + i * sizeof(struct direntv6));

      if (entry->d_inumber == 0) continue;        /* entrada vacía */

      char entryName[15];
      memcpy(entryName, entry->d_name, 14);
      entryName[14] = '\0';              /* asegurar terminación */

      if (strcmp(name, entryName) == 0) {
      *dirEnt = *entry;              /* copiar resultado */
      return 0;                      /* éxito */
      }
    }
  }

return -1;                                 /* nombre no encontrado */
}
