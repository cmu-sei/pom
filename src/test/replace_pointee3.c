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

void* malloc_or_die(size_t n) {
    void* ret = malloc(n);
    if (!ret) {
        abort();
    }
    return ret;
}

int** foo(int*** p) {
    int** q = *p;
    #ifdef HAS_BUG
    **p = NULL; // This overwrites *q.
    #endif
    *p = malloc_or_die(sizeof(int*));
    **p = malloc_or_die(sizeof(int*));
    ***p = 777;
    return q;
}

int main() {
    int*** p;
    p = malloc_or_die(sizeof(int**));
    *p = malloc_or_die(sizeof(int*));
    **p = malloc_or_die(sizeof(int));
    ***p = 42;

    int** q = foo(p);

    printf("***p = %d, **q = %d\n", ***p, **q);
    free(**p);
    free(*p);
    free(p);
    free(*q);
    free(q);
    return 0;
}
