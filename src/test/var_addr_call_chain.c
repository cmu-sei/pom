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

void bar(int **x) {
    printf("%d\n", **x);
    #ifdef ALSO_FREE
        free(*x);
        *x = NULL;
    #endif
}

void foo(int** val) {
    bar(val);
}

int main(int argc, char** argv) {
    int *x = NULL, *y = NULL;
    x = malloc(sizeof(int));
    y = malloc(sizeof(int));
    if (!x || !y) {
        free(x);
        free(y);
        abort();
    }
    *x = 42;
    *y = 99;
    int* z = x;
    int** p = &x;
    if (argc > 1) {
        if (atoi(argv[1]) == 2) {
            p = &y;
        }
        #ifdef ALLOW_Z
        if (atoi(argv[1]) == 3) {
            p = &z;
        }
        #endif
    }
    foo(p);
    free(x);
    free(y);
    return 0;
}
