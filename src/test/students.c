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
#include <string.h>

struct Student {
    char* name;
    char* major;
};
typedef struct Student Student;

int main(int argc, char** argv) {
    char hello[] = "hello";
    Student students[] = {
        {strdup("Alice"),   strdup("Anthropology")},
        {strdup("Bob"),     strdup("Business")},
        {strdup("Charlie"), strdup("Computer Science")}
    };
    #define NUM_STUDENTS (sizeof(students) / sizeof(students[0]))
    Student* chosen = &students[argc % NUM_STUDENTS];

    // Question: is `chosen->name` responsible or irresponsible?
    #ifdef COPY
        chosen->name = hello; // Constraint violation if chosen->name is responsible.
    #else
        free(chosen->name);   // Constraint violation if chosen->name is irresponsible.
        // chosen->name = NULL;
    #endif

    for (int i=0; i < NUM_STUDENTS; i++) {
        printf("%s: %s\n", students[i].name, students[i].major);
    }

    for (int i=0; i < NUM_STUDENTS; i++) {
        free(students[i].name);
        free(students[i].major);
    }
}
