//
// <legal>
// Pointer Ownership Model (POM) Source Code Release
// 
// Copyright 2025 Carnegie Mellon University.
// 
// NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING
// INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON
// UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR
// IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF
// FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS
// OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT
// MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,
// TRADEMARK, OR COPYRIGHT INFRINGEMENT.
// 
// Licensed under a MIT (SEI)-style license, please see license.txt or
// contact permission@sei.cmu.edu for full terms.
// 
// [DISTRIBUTION STATEMENT A] This material has been approved for public
// release and unlimited distribution.  Please see Copyright notice for
// non-US Government use and distribution.
// 
// DM25-1262
// </legal>
//


#include <stdlib.h>
#include <stdio.h>

int foo(int*** ppp, int*** qqq, int*** mystery1, int*** mystery2) {
    *mystery1 = *mystery2;
    return ***ppp;  // Line 6
}

int main_uninit() {
    int* p = malloc(sizeof(int)); // responsible ptr
    int* q; // uninitialized
    if (!p) {return 1;}
    *p = 42;
    int** pp = &p;    // irresponsible ptr
    int** qq = &q;    // irresponsible ptr
    int*** ppp = &pp; // irresponsible ptr
    int*** qqq = &qq; // irresponsible ptr
    #ifdef HAS_BUG
    foo(ppp, qqq, ppp, qqq);  // Line 19
    #else
    foo(ppp, qqq, ppp, ppp);  // Line 21
    #endif
    free(p);
    return 0;
}

int main_null() {
    int* p = malloc(sizeof(int)); // responsible ptr
    int* q = NULL;
    if (!p) {return 1;}
    *p = 42;
    int** pp = &p;    // irresponsible ptr
    int** qq = &q;    // irresponsible ptr
    int*** ppp = &pp; // irresponsible ptr
    int*** qqq = &qq; // irresponsible ptr
    #ifdef HAS_BUG
    foo(ppp, qqq, ppp, qqq);  // Line 19
    #else
    foo(ppp, qqq, ppp, ppp);  // Line 21
    #endif
    free(p);
    return 0;
}

int main_zombie() {
    int* p = malloc(sizeof(int)); // responsible ptr
    int* q = malloc(sizeof(int)); // responsible ptr
    if (!p) {return 1;}
    if (!q) {return 1;}
    *p = 42;
    int** pp = &p;    // irresponsible ptr
    int** qq = &q;    // irresponsible ptr
    int*** ppp = &pp; // irresponsible ptr
    int*** qqq = &qq; // irresponsible ptr
    free(q);
    #ifdef HAS_BUG
    foo(ppp, qqq, ppp, qqq);  // Line 19
    #else
    foo(ppp, qqq, ppp, ppp);  // Line 21
    #endif
    free(p);
    return 0;
}

int main_good() {
    int* p = malloc(sizeof(int)); // responsible ptr
    int* q = malloc(sizeof(int)); // responsible ptr
    if (!p) {return 1;}
    if (!q) {return 1;}
    *p = 42;
    int** pp = &p;    // irresponsible ptr
    int** qq = &q;    // irresponsible ptr
    int*** ppp = &pp; // irresponsible ptr
    int*** qqq = &qq; // irresponsible ptr
    #ifdef HAS_BUG
    foo(ppp, qqq, ppp, qqq);  // Line 19
    #else
    foo(ppp, qqq, ppp, ppp);  // Line 21
    #endif
    free(q);
    free(p);
    return 0;
}

int main() {
  return main_uninit();
}


/* Valgrind output for 'HAS_BUG' case, using main_uninit: as main
==80976== Use of uninitialised value of size 8
==80976==    at 0x108726: foo (deep_alias_2.c:6)
==80976==    by 0x108796: main (deep_alias_2.c:19)
*/
