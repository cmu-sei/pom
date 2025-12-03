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
#include <stdlib.h>

void* malloc_or_die(int n) {
    void* ret = malloc(n);
    if (!ret) {abort();}
    return ret;
}

// Simple functions that will be pointed to
int* add(int a, int b) {
    int* ret = malloc_or_die(sizeof(int));
    *ret = a + b;
    return ret;
}

int* multiply(int a, int b) {
    int* ret = malloc_or_die(sizeof(int));
    *ret = a * b;
    return ret;
}

int* subtract(int a, int b) {
    int* ret = malloc_or_die(sizeof(int));
    *ret = a - b;
    return ret;
}

int read_and_free(int* p) {
    int ret = *p;
    free(p);
    return ret;
}

// Function that returns a function pointer
// This returns a pointer to a function that takes two ints and returns an int*
int *(*get_operation(char op))(int, int) {
    switch(op) {
        case '+': return add;
        case '*': return multiply;
        case '-': return subtract;
        default: return NULL;
    }
}

int main() {
    // Simple function pointer usage
    int *(*func_ptr)(int, int);
    func_ptr = add;
    printf("Using function pointer directly: 5 + 3 = %d\n", read_and_free(func_ptr(5, 3)));

    // Now for the interesting part: a function pointer that returns a function pointer
    // Declare a pointer to a function that returns a function pointer
    int *(*(*operation_selector)(char))(int, int);  // Added * to match int* return type

    // Assign our get_operation function to it
    operation_selector = get_operation;

    // Call via the function pointer to get another function pointer, then call that
    printf("\nUsing function pointer that returns function pointer:\n");
    int result = read_and_free(operation_selector('+')(10, 5));  // Added read_and_free
    printf("10 + 5 = %d\n", result);

    result = read_and_free(operation_selector('*')(10, 5));  // Added read_and_free
    printf("10 * 5 = %d\n", result);

    result = read_and_free(operation_selector('-')(10, 5));  // Added read_and_free
    printf("10 - 5 = %d\n", result);

    // Breaking it down step by step for clarity
    printf("\nStep-by-step breakdown:\n");
    int *(*math_func)(int, int);                // Added * to match int* return type
    math_func = operation_selector('*');        // Get function pointer via another function pointer
    result = read_and_free(math_func(7, 6));    // Call the returned function
    printf("7 * 6 = %d\n", result);

    // Load func ptr from mem
    typeof(add)** p_fn_ptr = malloc_or_die(sizeof(*p_fn_ptr));
    *p_fn_ptr = &add;
    printf("1000 + 42 = %d\n", read_and_free((**p_fn_ptr)(1000, 42)));
    free(p_fn_ptr);

    return 0;
}
