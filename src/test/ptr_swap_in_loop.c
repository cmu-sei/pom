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

int main(int argc, char** argv) {
    int* p1 = malloc_or_die(sizeof(int*)); *p1 = 42;
    int* p2 = malloc_or_die(sizeof(int*)); *p2 = 777;
    for (int i=0; i < argc; i++) {
        int* t = p1;
        p1 = p2;
        p2 = t;
    }
    printf("*p1 = %d, *p2 = %d\n", *p1, *p2);
    free(p1);
    free(p2);
    return 0;
}
