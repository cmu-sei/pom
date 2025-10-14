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

void __dump_borrows() {}

void borrow(int n, int** p1, int** p2, int** p3, int** p4) {
    if (n == 0) {
        return;
    }
    *p4 = *p3;
    *p3 = *p2;
    *p2 = *p1;
    //__dump_borrows();
    borrow(n-1, p1, p2, p3, p4);
    //__dump_borrows();
}

int main() {
    int x1 = 1; int* p1 = &x1;
    int x2 = 2; int* p2 = &x2;
    int x3 = 3; int* p3 = &x3;
    int x4 = 4; int* p4 = &x4;
    borrow(2, &p1, &p2, &p3, &p4);
    printf("*p1=%d, *p2=%d, *p3=%d, *p4=%d\n", *p1, *p2, *p3, *p4);
    return 0;
}
