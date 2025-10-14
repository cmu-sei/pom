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
    #define ARR_LEN 10
    int** array = calloc(ARR_LEN, sizeof(int*));
    for (int i=0; i < ARR_LEN; i++) {
        array[i] = malloc(sizeof(int));
        if (array[i] == NULL) {abort();}
        *array[i] = 1000+i;
    }

    if (argc < 3) {abort();}
    int j = atoi(argv[1]);
    int k = atoi(argv[2]);
    #ifdef HAS_BUG
        free(array[j]);
        free(array[k]);
        array[j] = NULL;
        array[k] = NULL;
    #else
        free(array[j]);
        array[j] = NULL;
        free(array[k]);
        array[k] = NULL;
    #endif

    for (int i=0; i < ARR_LEN; i++) {
        if (array[i]) {
            printf("*arr[%2d] = %d\n", i, *array[i]);
        } else {
            printf(" arr[%2d] = NULL\n", i);
        }
    }
    for (int i=0; i < ARR_LEN; i++) {
        free(array[i]);
        array[i] = NULL;
    }
    return 0;
}
