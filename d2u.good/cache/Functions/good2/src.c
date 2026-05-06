static void good2() // Line 125
{ // Line 126
    if(staticReturnsTrue()) // Line 127
    { // Line 128
        { // Line 129
            /* Call the good helper function */ // Line 130
            char * reversedString = helperGood("GoodSink"); // Line 131
            printLine(reversedString); // Line 132
            /* free(reversedString); // Line 133
             * This call to free() was removed because we want the tool to detect the use after free, // Line 134
             * but we don't want that function to be free(). Essentially we want to avoid a double free // Line 135
             */ // Line 136
        } // Line 137
    } // Line 138
} // Line 139
