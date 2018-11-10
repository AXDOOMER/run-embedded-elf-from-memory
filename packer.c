// A packer that executes a second (embedded) ELF
//
// Copyright (c) 2018  Alexandre-Xavier Labonté-Lamoureux
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// memcpy
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Use "SYS_memfd_create", because the <sys/memfd.h> wrapper doesn't exist until Linux 4.10
// The "memfd_create" system call was added to Linux 3.17 so "syscall" can be used to call it
#include <sys/syscall.h>

int main(int argc, char* argv[], char** envp)
{
	int pid = getpid();
	printf("My PID is: %d\n", pid);
	printf("My filename is: %s\n", argv[0]);

	// Find the size of the file using <sys/stat.h>
	struct stat st;
	stat(argv[0], &st);
	size_t size = st.st_size;

	printf("My size is: %d\n", size);

	char procstring[32];
	sprintf(procstring, "%s%d%s", "/proc/", pid, "/exe");

	printf("Location (copy): %s\n", procstring);

	int filedesc = open(procstring, O_RDONLY);
	if (filedesc < 0)
	{
		printf("Invalid file descriptor for /proc: %d\n", filedesc);
		return 1;
	}

	// Delete self
	unlink(argv[0]);
	if (access(argv[0], F_OK) < 0)
		printf("I successfully deleted myself.\n");
	else
		printf("Couldn't erase myself from the filesystem.\n");

	// The real business starts here
	char* entirefile = (char*)malloc(size);
	read(filedesc, entirefile, size);

	// Yeah... use arbitrary values here.
	for (int i = size - 10; i > 1000; i--)
	{
		// The goal is to find the second ELF header
		if (entirefile[i] == 0x7F && entirefile[i + 1] == 'E' && entirefile[i + 2] == 'L' && entirefile[i + 3] == 'F')
		{
			printf("Second ELF header found at: 0x%x\n", i);

			// Create a buffer for this second ELF
			int newsize = size - i;
			char* newelf = (char*)malloc(newsize);

			// Copy ELF to buffer
			memcpy(newelf, entirefile + i, newsize);

			// It's in memory! Create a file descriptor.
			int memfd = syscall(SYS_memfd_create, "hidden", 0);

			if (memfd < 0)
			{
				printf("Invalid memfd %d.\n", i);
				return 1;
			}
			else
				printf("memfd is: %d\n", memfd);

			// Write ELF to temporary memory file
			write(memfd, newelf, newsize);

			// Deploy the payload as a different process
			fork();
			int newpid = getpid();
			if (newpid != pid)
			{
				// Execute the in-memory ELF
				int ret = fexecve(memfd, argv, envp);
				// The above function will only return if there's an error
				printf("ERROR EXECUTING PAYLOAD: Return value: %d. Errno is: ret %d\n", ret, errno);
			}
			else
				printf("The packer is done deploying the payload.\n");

			free(newelf);
		}
	}

	// Free the resources
	free(entirefile);
	close(filedesc);

	return 0;
}
