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

void __dump_borrows(){};
#define check(x) if (!(x)) {printf("Fail: %s\n", #x);}

// Command to run: ./do_tests.py '["GOOD", "borrow_chain02.c", {"LLVM_OPTS":"-no-check-null -no-mem-leak"}]' | grep -v call | sed 's/_[0-9]\+//g'

int main() {
    int* p1 = malloc(sizeof(int));
    int** p2 = malloc(sizeof(int*));
    int*** p3 = malloc(sizeof(int*));
    int** p4 = malloc(sizeof(int*));
    int** p5 = malloc(sizeof(int*));

    *p3 = p2;
    **p3 = p1; //__dump_borrows();
    p4 = *p3;  //__dump_borrows();
    *p3 = p5;  __dump_borrows();
    // Expected results:
    // borrow_to[*p2] = {p1}
    // borrow_to[*p3] = {p5}
    // borrow_to[p4] = {p2}
    // borrow_to[*p4] = {p1}
    check(*p2 == p1);
    check(*p3 == p5);
    check(p4 == p2);
    check(*p4 == p1);
    printf("p1=%p, p2=%p, p3=%p, p4=%p, p5=%p\n", p1, p2, p3, p4, p5); // Keep vars alive.
    return 0;
}
