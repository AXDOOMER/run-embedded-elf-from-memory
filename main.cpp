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

// How to compile and run: g++ main.cpp -o emb && cat hello >> emb  && ./emb

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <unistd.h>

#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int main(int argc, char * argv[], char **envp)
{
	int pid = getpid();
	cout << "Mon PID c'est " << pid << endl;
	cout << "Mon filename c'est " << argv[0] << endl;

	// Loader le fichier en mémoire dans un buffer (utilise sys/stat.h)
	struct stat st;
	stat(argv[0], &st);
	size_t size = st.st_size;

	cout << "Ma taille c'est " << size << endl;

	//usleep(1000000000);

	char procstring[32];
	sprintf(procstring, "%s%d%s", "/proc/", pid, "/exe");

	cout << procstring << endl;

	int filedesc = open(procstring, O_RDONLY);
	if (filedesc < 0)
	{
		cout << "Pas capable de m'ouvrir!" << endl;
		return 1;
	}
	else
	{
		// TODO: delete self

		// Ici que la vrai business commence...
		char *entirefile = (char *) malloc(size);
		read(filedesc, entirefile, size);

		// Ouin... des tailles arbitraires ici.
		for (int i = size - 10; i > 1000; i--)
		{
			// Le but c'est de trouver le deuxième header
			if (entirefile[i] == 0x7F && entirefile[i+1] == 'E' && entirefile[i+2] == 'L' && entirefile[i+3] == 'F')
			{
				cout << "Trouvé! Checkez ici: " << i << endl;

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

				// Cool, c'est en mémoire!

				FILE* memfile = fmemopen(newelf, newsize, "r");

				if (memfile == NULL)
				{
					cout << "FILE error" << endl;
					return -1;
				}

				// Convert FILE* to int
//				int memfiledescriptor = fileno(memfile);

//				extern char **environ;

// shm_fd:  https://github.com/XiphosResearch/netelf/blob/master/_linux.c

				int memfiledescriptor = memfd_create("hello", MFD_ALLOW_SEALING);
				if (ftruncate(memfiledescriptor, newsize) < 0)
				{
					cout << "222" <<endl;
					return -1;
				}

				if (memfiledescriptor < 0)
				{
					cout << "memfiledescriptor invalde " << memfiledescriptor << endl;
					return 1;
				}

				int ret = fexecve(memfiledescriptor, argv, envp);
				cout << "Return value: " << ret << endl;

				//memexec(newelf, newsize, argv);

				free(newelf);
			}
		}

		// Libérer les ressources
		free(entirefile);
		close(filedesc);
	}

	return 0;
}
