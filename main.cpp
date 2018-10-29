// A kind of packer that runs a second ELF that's embedded in itself
// Copyright (C) 2018  Alexandre-Xavier Labonté-Lamoureux
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// use SYS_memfd_create, because the <sys/memfd.h> wrapper doesn't exist until Linux 4.10 or something like that
#include <sys/syscall.h>

int main(int argc, char * argv[], char **envp)
{
	int pid = getpid();
	printf("My PID is %d.\n", pid);
	printf("My filename is %s\n", argv[0]);

	// Loader le fichier en mémoire dans un buffer (utilise sys/stat.h)
	struct stat st;
	stat(argv[0], &st);
	size_t size = st.st_size;

	printf("My size is %d.\n", size);

	char procstring[32];
	sprintf(procstring, "%s%d%s", "/proc/", pid, "/exe");

	printf("Location: %s.\n", procstring);

	int filedesc = open(procstring, O_RDONLY);
	if (filedesc < 0)
	{
		printf("Invalid file descriptor for /proc: %d\n", filedesc);
		return 1;
	}

	// TODO: delete self using 'rm'

	// The real business starts here
	char *entirefile = (char*)malloc(size);
	read(filedesc, entirefile, size);

	// Yeah... use arbitrary values here.
	for (int i = size - 10; i > 1000; i--)
	{
		// The goal is to find the second ELF header
		if (entirefile[i] == 0x7F && entirefile[i+1] == 'E' && entirefile[i+2] == 'L' && entirefile[i+3] == 'F')
		{
			printf("Second ELF header is at %d.\n", i);

			// Créer un buffer pour ce deuxième ELF
			int newsize = size - i;
			char *newelf = (char *) malloc(newsize);

			int j = i;
			int k = 0;
			while (j < size)
			{
				newelf[k] = entirefile[j];

				j++;
				k++;
			}

			// It's in memory!

			int memfd = syscall(SYS_memfd_create, "hidden", 0);

/*			if (ftruncate(memfd, newsize) < 0)
			{
				return 1;
			}
*/
			if (memfd < 0)
			{
				printf("Invalid memfd %d.\n", i);
				return 1;
			}
			else
				printf("memfd Ok: %d\n", memfd);

			write(memfd, newelf, newsize);

			// Execute the in-memory ELF
			int ret = fexecve(memfd, argv, envp);
			printf("Return value: %d. Errno is: ret %d\n", ret, errno);

			free(newelf);
		}
	}

	// Free the ressources
	free(entirefile);
	close(filedesc);

	return 0;
}
