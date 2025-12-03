/*
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
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Example run:
// $ ./a.out food
// mood

int main(int argc, char** argv) {
    if (argc < 2) {
        abort();
    }
    char* data = strdup(argv[1]);
    if (!data) {abort();}

    // Two immutable pointers returned by two calls to strchr
    char* loc_o = strchr(data, 'o');
    char* loc_d = strchr(data, 'd');
    if (loc_o) {
        printf("Character after first 'o' is 0x%02x.\n", *(loc_o + 1));
    }
    if (loc_d) {
        printf("Character after first 'd' is 0x%02x.\n", *(loc_d + 1));
    }

    // One mutable pointer returned by a call to strchr
    char* replace = strchr(data, 'f');
    if (replace) {
        *replace = 'm';
    }
    printf("%s\n", data);
    free(data);
    return 0;
}
