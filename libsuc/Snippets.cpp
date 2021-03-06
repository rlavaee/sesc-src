/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Milos Prvulovic

This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "Snippets.h"
#include "nanassert.h"

#include <typeinfo>

ushort log2i(ulong n)
{
  ulong m = 1;
  ulong i = 0;

  //assume integer power of 2
  GI(n!=1,(n & (n - 1)) == 0);

  while(m<n) { 
    i++; 
    m <<=1; 
  }

  return i;
}

short log2i(long n)
{
  return (long)log2i((ulong)n);
}

short log2i(int n)
{
  return (int)log2i((ulong)n);
}

// this routine computes the smallest power of 2 greater than the
// parameter
unsigned int roundUpPower2(unsigned int x)
{  
  // efficient branchless code extracted from "Hacker's Delight" by
  // Henry S. Warren, Jr.

  x = x - 1;
  x = x | (x >>  1);
  x = x | (x >>  2);
  x = x | (x >>  4);
  x = x | (x >>  8);
  x = x | (x >> 16);
  return x + 1;
}

bool debacc  = false;
void debugAccess()
{
  debacc = true;
  int j = rand();
}
