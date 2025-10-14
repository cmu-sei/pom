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
    int* p1 = malloc(sizeof(int)); *p1 = 1;
    int* p2 = malloc(sizeof(int)); *p2 = 2;
    int* q;
    if (argc % 2) {
        q = p1;
        /* Here, p1 is zombie and p2 is good. */
    } else {
        q = p2;
        /* Here, p2 is zombie and p1 is good. */
    }
    printf("*p1 = %d, *p2 = %d\n", *p1, *p2);  // p1 or p2 is ZOMBIE, violates POM
    free(q);
    // We've now leaked either p1 or p2.
    return 0;
}
