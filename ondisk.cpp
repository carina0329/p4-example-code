#include "fs_server.h"

#include <cassert>
#include <cstring>

int main()
{
    fs_inode   root_inode;

    // Read the root inode of the (assumed to be) empty file system
    disk_readblock(0, &root_inode);

    // Make sure it is the empty root directory
    assert(root_inode.type == 'd');
    assert(root_inode.owner[0] == '\0');
    assert(root_inode.size == 0);

    // Create an empty file
    fs_inode file_inode;

    file_inode.type = 'f';
    strcpy(file_inode.owner, "bnoble");
    file_inode.size = 0;

    // Write that inode to block 1
    disk_writeblock(1, &file_inode);

    // Create a directory entry block with this file
    fs_direntry   root_dirblock[FS_DIRENTRIES];

    // Initialize that directory entry block to empty
    for (unsigned int i = 0; i<FS_DIRENTRIES; i++) {
	root_dirblock[i].inode_block = 0; // unused
    }

    // Add the new file to the directory entry block
    strcpy(root_dirblock[0].name, "aFile");
    root_dirblock[0].inode_block = 1; // The block of the file inode

    // Write this directory entry block to block #2
    disk_writeblock(2, root_dirblock);

    // Now update the root inode to point to this new block
    root_inode.size = 1; // There is one block
    root_inode.blocks[0] = 2; // It lives at block #2

    // Write the new root inode
    disk_writeblock(0, &root_inode);

    // We are now done.
    return 0;
}
