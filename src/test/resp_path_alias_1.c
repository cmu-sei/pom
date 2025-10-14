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

void foo_0(int** q) {
    printf("%d\n", **q);
    (*q)++;
    printf("%d\n", **q);
    free(q);
}
void foo_1(int** q) {
    printf("%d\n", **q);
    (*q)++;
    printf("%d\n", **q);
}

void cleanup(int** ppX) {
    free(*ppX);
    free(ppX);
}

int main() {
    int** ppX = malloc(sizeof(int*));
    *ppX = malloc(sizeof(int)*2);
    **ppX = 42;
    *(1+*ppX) = 43;
    int** p2 = malloc(sizeof(int*));
    *p2 = *ppX;
    int** p3 = ppX;
    #if HAS_BUG == 1
        foo_1(p3);
    #elif HAS_BUG == 2
        cleanup(p2);
    #elif HAS_BUG == 3
        free(*p3);
    #else
        foo_0(p2);
    #endif
    cleanup(ppX);
    return 0;
}
