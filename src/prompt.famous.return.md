I want you to tell me about the C function {function_name}.  In particular, does this function return a pointer?  If not, then say:

{{"return": {{}}}}

If it returns a pointer, I'd like to know if the it returns a responsible or irresponsible pointer.  These are defined as follows:

 * A return value is "irresponsible" if its value is never freed or consumed, usually because some other pointer is responsible for freeing it, or the return value does not point to the heap.  Also indicate the possible states of the return pointer, which can be any of NUL, VALID, or INVALID.  Also indicate how long the object it references will live.  If the object is permanent, then it has a "static" lifetime.  For example, if the return pointer could be VALID or NUL, and its pointed-to object must not outlive the "foo" or "bar" function arguments, but may outlive the "baz" function arguments, then say:
 
{{"return": {{"resp": "irresponsible",  "start": ["NUL", "VALID"], "lifetime": ["foo", "bar"]}}}}

 * A return value is "responsible" if the it points to memory on the heap, and it must eventually be freed or assigned to a larger object that will free the object when it is itself freed.  The pointer's state should also be noted.  Unlike irresponsible pointer, the states of responsible pointers are NUL, GOOD, or ZOMBIE.  A responsible pointer becomes a ZOMBIE when its pointed-to object is freed.  If the pointer should be freed by a function other than free, indicate that as well.  For example, if the return pointer is responsible, will always be GOOD, and should be passed by fclose() instead of free(), then say:

{{"return": {{"resp": "responsible",  "start": ["GOOD"], "destructor": "fclose"}}}}
