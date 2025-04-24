/*
 * maptest.c
 * 
 *   $ gcc -o maptest -lm maptest.c   # link to math library
 *   $ ./maptest poem 5 25            # write 25 chars from offset 5 
 * 
 * Program description...
 * Prints part of the file specified in its first command-line argument to 
 * standard output. The range of bytes to be printed is specified via offset 
 * and length values in the second and third command-line arguments. The 
 * program creates a memory mapping of the required pages of the file and then 
 * uses write(2) to output the desired bytes.
 *
 * Notes: mmap allows multiple processes to share virtual memory; unlike SysV 
 * IPC, mmap structures are not persistent; when attached to a file there is 
 * no inherent synchronization so processes will not always see the same data;
 * the segment is copied into RAM and periodically flushed to disk; 
 * synchronization can be forced with the msync system call.
 * 
 * A file is mapped in multiples of the page size so the offset into the file
 * must be page aligned.
 */

#include <sys/mman.h>   /* mmap() function is defined in this header */
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define perror(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
    int fd, fd2;
    struct stat sb;  /* see below for struct stat definition */
    off_t offset, pa_offset;
    size_t length;

    if (argc < 3) {
        fprintf(stdout,"Usage: %s <infile> <offset> <length>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* open the input file for reading */
    fd = open(argv[1], O_RDONLY);
    if (fd == -1)
        perror("open");

    /* returns file size in struct stat sb */
    if (fstat(fd, &sb) == -1)
        perror("fstat");
    printf("filesize: %ld bytes\n", sb.st_size); 
    offset = atoi(argv[2]);
    /* determine page offset in case you need to re-align things;
       this is just finding your starting page */
    pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1); /* ~ is bitwise not */ 
    printf("pa_offset %ld\n", pa_offset); 

    if (offset >= sb.st_size) {
        fprintf(stderr, "offset is past end of file\n");
        exit(EXIT_FAILURE);
    }

    if (argc >= 4) {
        length = atoi(argv[3]);
        if (offset + length > sb.st_size)
            length = sb.st_size - offset;
        /* Can't display bytes past end of file */
    } else {  
        /* if you made it here there is no length arg so just display to EOF */
        length = sb.st_size - offset;
    }

    /* This maps your input file to virtual memory address: addr */
    char *addr = mmap(NULL, length + offset - pa_offset, PROT_READ,
            MAP_PRIVATE, fd, pa_offset);
    if (addr == MAP_FAILED)
        perror("mmap");

    int pagesz = getpagesize(); 
    pid_t mypid = getpid();
    printf("pid: %d pagesize: %d \n", mypid, pagesz);

    fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);

    char *dest = mmap(NULL, sb.st_size, PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (dest == MAP_FAILED)
        perror("mmap:");
    memcpy(dest, addr + offset - pa_offset, length);
    /* ssize_t s;
       write(STDOUT_FILENO,"\n",2); 
       write(STDOUT_FILENO, "\n", 1);
       s = write(STDOUT_FILENO, addr + offset - pa_offset, length); 
       write(STDOUT_FILENO, "\n", 1);
       if (s != length) {
       if (s == -1)
       perror("write");
       fprintf(stderr, "partial write");
       exit(EXIT_FAILURE);
       } */
    munmap(addr, length + offset - pa_offset);
    munmap(dest, length + offset - pa_offset);
    close(fd);
    close(fd2);
    exit(EXIT_SUCCESS);
}

