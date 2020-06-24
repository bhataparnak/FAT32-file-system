# FAT32-file-system

Implement a user space shell application that is capable of interpreting a FAT32 file system image. The utility must not corrupt
the file system image and should be robust. 

Following commands are supported

1) open <filename>
2) close
3) stat <filename> or <directory name>
4) get <filename>
5) cd <directory>
6) ls
7) read <filename> <position> <number of bytes>
8) info
