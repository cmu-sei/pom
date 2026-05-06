static void good1() // Line 103
{ // Line 104
    if(staticReturnsFalse()) // Line 105
    { // Line 106
        /* INCIDENTAL: CWE 561 Dead Code, the code below will never run */ // Line 107
        printLine("Benign, fixed string"); // Line 108
    } // Line 109
    else // Line 110
    { // Line 111
        { // Line 112
            /* Call the good helper function */ // Line 113
            char * reversedString = helperGood("GoodSink"); // Line 114
            printLine(reversedString); // Line 115
            /* free(reversedString); // Line 116
             * This call to free() was removed because we want the tool to detect the use after free, // Line 117
             * but we don't want that function to be free(). Essentially we want to avoid a double free // Line 118
             */ // Line 119
        } // Line 120
    } // Line 121
} // Line 122
