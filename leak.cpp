#include <unistd.h>

#include <iostream>

using namespace std;

unsigned long long getAvailSystemMemory()
{
	long pages = sysconf(_SC_AVPHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}

int main()
{
	while(getAvailSystemMemory() > 1024 * 1024 * 1024)
	{
		int* tableau = new int[1024];
		cout << "." << flush;
	}
}
