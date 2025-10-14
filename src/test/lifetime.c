/*
 * Simple example showing lifetimes
 *
 * Rough C translation of Rust linked list from:
 * https://doc.rust-lang.org/book/ch10-03-lifetime-syntax.html
 *
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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

char* longest(char* x, char* y) {
  return (strlen(x) > strlen(y)) ? x : y;
}

int main(void) {
  char string1[] = "long string is long";
  char *result;
  {
    char string2[] = "xyz";
    result = longest( string1, string2);
  }
  puts("The longest string is:");
  puts(result);  // POM lifetime violation, string2 out of scope!
  // However, this is satisfiable b/c the inner block is eliminated at
  // the LLVM-IR level, so the IR doesn't violate POM. See lifetime2.c
  // for an example of lifetime violation.
  return 0;
}
