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

int* malloc_and_init(int init_val) {
    int* ret = malloc(sizeof(int));
    if (!ret) {abort();}
    *ret = init_val;
    return ret;
}

struct PointT {
    int* x;
    int* y;
};

struct BigStructT {
    int inline_array[100];
};
typedef struct BigStructT BigStructT;

struct PointT foo(struct PointT in_pt) {
    struct PointT out_pt;
    out_pt.x = in_pt.y;
    out_pt.y = in_pt.x;
    return out_pt;
}

BigStructT bar(BigStructT in) {
    BigStructT out;
    for (int i=0; i < sizeof(in.inline_array)/sizeof(in.inline_array[0]); i++) {
        out.inline_array[i] = in.inline_array[i] + 1;
    }
    return out;
}

int main() {
    struct PointT pt1a = {malloc_and_init(1), malloc_and_init(2)};
    struct PointT pt1b = foo(pt1a);
    printf("*pt1b.x = %d, *pt1b.y = %d\n", *pt1b.x, *pt1b.y);
    free(pt1a.x);
    free(pt1a.y);

    BigStructT s1 = {0};
    BigStructT s2 = bar(s1);
}
