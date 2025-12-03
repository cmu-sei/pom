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
    if (!ret) {abort();}
    return ret;
}

int main(int argc, char** argv) {
    int *x = NULL, *y = NULL;
    x = malloc_or_die(sizeof(int));
    y = malloc_or_die(sizeof(int));
    *x = 42;
    *y = 99;
    int z = 0;
    int* pz = &z;
    int** p1 = &x;
    int** p3 = &pz;
    if (argc > 1) {
        if (atoi(argv[1]) == 2) {
            p1 = &y;
        }
        p3 = p1;
    }
    **p3 = 7;
    printf("**p1 = %d\n", **p1);
    printf("**p3 = %d\n", **p3);
    free(x);
    free(y);
    return 0;
}
