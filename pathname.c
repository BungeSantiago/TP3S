
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
    if (!fs || !pathname || pathname[0] != '/')
        return -1;

    if (pathname[1] == '\0')
        return ROOT_INUMBER;                       /* / */

    int curr_inum = ROOT_INUMBER;                  /* empezamos en raíz */
    const char *p = pathname + 1;                  /* saltar '/' inicial */
    char name[15];                                 /* 14 + '\0' */
    int len = 0;

    while (1) {
        if (*p != '/' && *p != '\0') {
            if (len >= 14) return -1;              /* nombre muy largo */
            name[len++] = *p++;
            continue;
        }

        if (len == 0) return -1;                   /* shouldn't happen (no //) */
        name[len] = '\0';

        struct direntv6 entry;
        if (directory_findname(fs, name, curr_inum, &entry) < 0)
            return -1;                             /* componente no hallado */

        curr_inum = entry.d_inumber;

        if (*p == '/') {
            /* Debe ser directorio */
            struct inode node;
            if (inode_iget(fs, curr_inum, &node) < 0)
                return -1;
            if ((node.i_mode & IFMT) != IFDIR)
                return -1;                         /* intermedio no-directorio */

            ++p;                                   /* saltar '/' */
            len = 0;                               /* reiniciar buffer nombre */
            continue;
        }
        break;
    }

    return curr_inum;                              /* éxito */
}
