#include <iostream>

#ifndef __APPLE__
#include <malloc.h>
#endif

#include "heaplayers/wrappers/gnuwrapper.h"

#include <limits.h>

using namespace std;

int main()
{
  cout << "size = " << (size_t) -1 << endl;
  cout << "uint_max = " << UINT_MAX << endl;
  cout << "ulong_max = " << ULONG_MAX << endl;

 void * m1 = xxmalloc(1024);
  void * c1 = calloc(1024, 1024);
  int  * n1 = new int[1024-1];

  cout << "Result of malloc = " << m1 << endl
       << "Result of calloc = " << c1 << endl
       << "Result of new    = " << n1 << endl;


  void * m = malloc((size_t) -1);
  void * c = calloc(UINT_MAX, UINT_MAX);
  int  * n = new int[UINT_MAX];

  cout << "Result of malloc = " << m << endl
       << "Result of calloc = " << c << endl
       << "Result of new    = " << n << endl;

  return 0;
}
