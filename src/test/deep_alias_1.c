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
#include <string.h>
#include <stdint.h>
#include <stdio.h>

void* malloc_or_die(size_t sz) {
    void* ret = malloc(sz);
    if (!ret) {
        abort();
    }
    return ret;
}

uintptr_t min_ptr = (uintptr_t)(ssize_t)(-1);

void update_min_ptr(void* new_ptr) {
    if ((uintptr_t)new_ptr < min_ptr) {
        min_ptr = (uintptr_t)new_ptr;
    }
}

size_t sub_min_ptr(void* ptr) {
    return (size_t)((uintptr_t)ptr - min_ptr);
}

int*** triple_malloc(int base_val, int**** r1, int*** r2, int** r3) {
    int*** ret;
    *r1 = malloc_or_die(sizeof(*ret));   ret = *r1;   update_min_ptr(ret);
    *r2 = malloc_or_die(sizeof(**ret));  *ret = *r2;  update_min_ptr(*ret);
    *r3 = malloc_or_die(sizeof(***ret)); **ret = *r3; update_min_ptr(**ret);
    ***ret = base_val;
    return ret;
}

void foo(int*** x, int*** y, int*** z, int*** w) {
    **y = **z;
    *x = *z;
    ***w += 1;
}

void print_ptr_chain(int*** var, char* name) {
    printf("%s: 0x%03jx -> 0x%03jx -> 0x%03jx -> %d\n",
        name, sub_min_ptr(var), sub_min_ptr(*var), sub_min_ptr(**var), ***var);
}

int main() {
    // The below 'r' variables are responsible pointers that 'own' the memory
    // allocated by triple_malloc.  Invariants: The value of each 'r' variable
    // doesn't change until the memory is freed, whereupon the 'r' variable may
    // be set to NULL.
    int ***rx1, **rx2, *rx3;
    int ***ry1, **ry2, *ry3;
    int ***rz1, **rz2, *rz3;
    int ***rw1, **rw2, *rw3;

    int*** x = triple_malloc(10, &rx1, &rx2, &rx3);
    int*** y = triple_malloc(20, &ry1, &ry2, &ry3);
    int*** z = triple_malloc(30, &rz1, &rz2, &rz3);
    int*** w = triple_malloc(40, &rw1, &rw2, &rw3);

    printf("Before:\n");
    print_ptr_chain(x, "x");
    print_ptr_chain(y, "y");
    print_ptr_chain(z, "z");
    print_ptr_chain(w, "w");

    *w = *y;
    foo(x,y,z,w);

    free(rx2); rx2=NULL; free(rx3); rx3=NULL;
    free(ry3); ry3=NULL;
    free(rw2); rw2=NULL; free(rw3); rw3=NULL;

    printf("After:\n");
    print_ptr_chain(x, "x");
    print_ptr_chain(y, "y");
    print_ptr_chain(z, "z");
    print_ptr_chain(w, "w");

    free(rx1); free(rx2); free(rx3);
    free(ry1); free(ry2); free(ry3);
    free(rz1); free(rz2); free(rz3);
    free(rw1); free(rw2); free(rw3);

    return 0;
}
