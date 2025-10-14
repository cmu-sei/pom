I want you to tell me about the C function {function_name}.  In particular, I want to know about the behavior of each function argument that is a pointer.  You should build a JSON data structure that looks like:

{{"signature": "SIGNATURE",
  "args": {{"ptr_arg1": ..., "ptr_arg2": ..., ...}}}}

The SIGNATURE is the function's argument signature, that you would see in a header file or manpage. Each argument should have a type and a name.  This should include every argument, not just the pointers.

As for the "args", I just want to know about the pointers; you can ignore the non-pointer arguments.  For each pointer argument, I want to know its index in the argument list (where the first argument has index 0).  I also want to know each pointer's responsibility, which will be one of four terms: irresponsible, responsible, diligent, or producer. They are defined as follows:

 * A pointer is "producer" only if it obeys the following:
 
   * It references another pointer, which we'll call the inner pointer.
   * The producer pointer must be constant, that is, the function may not assign it to point elsewhere.
   * The producer pointer may not be freed or consumed
   
If a pointer is a producer, then treat its referent (that is, the pointer it refers to) as an argument, and provide the same analysis for its argument. That is, its argument could be diligent, responsible, or irresponsible, and provide the same analysis for the referent. For example, if "ptr" is the second argument, and is a producer to an responsible pointer that could start off as a ZOMBIE but ends up GOOD, then say:

{{"ptr": {{"arg_idx": 1, "resp": "producer", "referent": {{"resp": "responsible",  "start": ["ZOMBIE"], "end": ["GOOD"]}}}}}}

 * A pointer is "diligent" if it obeys the following:
 
   * They always point to valid memory or NULL and are never assigned to point somewhere else.
   * The memory they point to is never freed or consumed.
   * Their value never escapes the function...they are never assigned to another pointer.
     There is one exception: The function may return their value as an irresponsible pointer.

The diligent pointer's pointed-to value may be changed, but never freed.

Diligent pointers each have a start state, which is the state the pointer is in when the function is invoked. The state is either VALID or [NUL, VALID] depending on whether the pointer may or may not be NULL.    For example, if "ptr" is the first argument, is diligent, and may be VALID or NULL, then say:

{{"ptr": {{"arg_idx": 0, "resp": "diligent", "start": ["NUL", "VALID"]}}}}

 * A pointer is "irresponsible" if its value is never freed or consumed, but might be assigned to point to something else. If the pointer or its referenced data could be changed via the pointer, mark the pointer as mutable.  These pointers each have a start state, which is the state the pointer is in when the function is first invoked, and an end state, which is the state of the pointer when the function returns. These states can be NUL, VALID, or INVALID.  For example, if "ptr" is the 4th argument, starts off VALID and might becomes VALID or INVALID when the function is done, this means it is inherently mutable, then say:

{{"ptr": {{"arg_idx": 3, "resp": "irresponsible", "mutable": true,  "start": ["VALID"],  "end": ["INVALID", "VALID"]}}}}

 * A pointer is "responsible" if the pointer points to memory on the heap, and it is either freed or assigned to a larger object that will free the object when it is itself freed. Don't worry about whether or not the pointer is mutable.  These pointers' start states and end states can be noted. Unlike irresponsible pointer, the states of responsible pointers are NUL, GOOD, or ZOMBIE.  A responsible pointer becomes a ZOMBIE when its pointed-to object is freed.  If trhe pointer should be freed by a function other than free, indicate that as well.  For example, if the 2nd argument "ptr" is responsible, starts off GOOD and becomes a ZOMBIE when the function is done, and should be passed by fclose() instead of free(), then say:

{{"ptr": {{"arg_idx": 1, "resp": "responsible",  "start": ["GOOD"], "end": ["ZOMBIE"], "destructor": "fclose"}}}}
