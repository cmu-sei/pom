static char * helperBad(char * aString) // Line 18
{ // Line 19
    size_t i = 0; // Line 20
    size_t j; // Line 21
    char * reversedString = NULL; // Line 22
    if (aString != NULL) // Line 23
    { // Line 24
        i = strlen(aString); // Line 25
        reversedString = (char *) malloc(i+1); // Line 26
        if (reversedString == NULL) {exit(-1);} // Line 27
        for (j = 0; j < i; j++) // Line 28
        { // Line 29
            reversedString[j] = aString[i-j-1]; // Line 30
        } // Line 31
        reversedString[i] = '\0'; // Line 32
        /* FLAW: Freeing a memory block and then returning a pointer to the freed memory */ // Line 33
        free(reversedString); // Line 34
        return reversedString; // Line 35
    } // Line 36
    else // Line 37
    { // Line 38
        return NULL; // Line 39
    } // Line 40
} // Line 41
