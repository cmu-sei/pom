static char * helperGood(char * aString) // Line 43
{ // Line 44
    size_t i = 0; // Line 45
    size_t j; // Line 46
    char * reversedString = NULL; // Line 47
    if (aString != NULL) // Line 48
    { // Line 49
        i = strlen(aString); // Line 50
        reversedString = (char *) malloc(i+1); // Line 51
        if (reversedString == NULL) {exit(-1);} // Line 52
        for (j = 0; j < i; j++) // Line 53
        { // Line 54
            reversedString[j] = aString[i-j-1]; // Line 55
        } // Line 56
        reversedString[i] = '\0'; // Line 57
        /* FIX: Do not free the memory before returning */ // Line 58
        return reversedString; // Line 59
    } // Line 60
    else // Line 61
    { // Line 62
        return NULL; // Line 63
    } // Line 64
} // Line 65
