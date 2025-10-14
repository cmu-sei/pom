// Test file to demonstrate VarStorePass functionality
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

int main(int argc, char** argv) {
    int *q1, *q2;
    int* foo = malloc(sizeof(int)); if (!foo) {abort();}
    int* bar = malloc(sizeof(int)); if (!bar) {free(foo); abort();}
    int* baz = malloc(sizeof(int)); if (!baz) {free(foo); free(bar); abort();}

    *foo = 1;
    *bar = 10;
    *baz = 100;

    if (argc % 2) {
        q1 = foo;  // This store should be transformed
        q2 = foo;  // This store should be transformed
    } else {
        q1 = bar;  // This store should be transformed
        q2 = baz;  // This store should be transformed
    }

    printf("%d\n", *q1 + *q2);
    free(foo);
    free(bar);
    free(baz);
    return 0;
}

int test_multiple(int cond1, int cond2) {
    int* foo = malloc(sizeof(int)); if (!foo) {abort();}
    int* bar = malloc(sizeof(int)); if (!bar) {free(foo); abort();}
    int* baz = malloc(sizeof(int)); if (!baz) {free(foo); free(bar); abort();}

    int *ptr1, *ptr2;
    *foo = 1;
    *bar = 10;
    *baz = 100;

    if (cond1) {
        ptr1 = foo;
        ptr2 = foo;
    } else {
        ptr1 = bar;
        ptr2 = baz;
    }

    if (cond2) {
        ptr2 = bar;
    }

    int ret = *ptr1 + *ptr2;
    free(foo);
    free(bar);
    free(baz);
    return ret;
}
