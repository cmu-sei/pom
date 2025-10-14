I want you to examine the following C function, and then tell me about function's return value, but only if it returns a pointer.  If the function does not return a pointer, then say:

    {{"return": {{}}}}

In particular, I want to know if the function returns a responsible or irresponsible pointer. These are defined as follows:

 * A return value is "responsible" iff it points to memory on the heap, and it must eventually be freed or assigned to a larger object that will free the object when it is itself freed.  The pointer's state should also be noted.  Unlike irresponsible pointers, the states of responsible pointers are NUL, GOOD, or ZOMBIE.  A responsible pointer becomes a ZOMBIE when its pointed-to object is freed.  If the pointer should be freed by a function other than free, indicate that as well.  For example, if the return pointer is responsible, will always be GOOD, and should be passed by fclose() instead of free(), then say:

{{"return": {{"resp": "responsible",  "start": ["GOOD"], "destructor": "fclose"}}}}

 * A return value is *irresponsible* iff its value is never freed or consumed, usually because some other pointer is responsible for freeing it, or the return value does not point to the heap.  Also indicate the possible states of the return pointer, which can be any of NUL, VALID, or INVALID.  Also indicate how long the object it references will live.  If the object is permanent, then it has a "static" lifetime.  For example, if the return pointer could be VALID or NUL, and its pointed-to object must not outlive the "foo" or "bar" function arguments, but may outlive the "baz" function arguments, then say:

    {{"return": {{"resp": "irresponsible",  "start": ["NUL", "VALID"], "lifetime": ["foo", "bar"]}}}}

Responsibility is a type-qualifier property; it doesn't change over time.

 
Below is the code.
File {file_name}:
```c
{source_code}
```

Think hard before giving your final answer.  If possible, your answer should comply with all the rules about `responsible` and `lifetime` given above.  If the program is not compliant with the pointer-ownership model specified above, then give your best guess at what the programmer likely intended.
