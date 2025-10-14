# Design of POM  (Pointer Ownership Model)
## Copyright

<legal>  
Pointer Ownership Model (POM) Source Code Release  
  
Copyright 2025 Carnegie Mellon University.  
  
NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING  
INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON  
UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR  
IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF  
FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS  
OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT  
MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,  
TRADEMARK, OR COPYRIGHT INFRINGEMENT.  
  
Licensed under a MIT (SEI)-style license, please see license.txt or  
contact permission@sei.cmu.edu for full terms.  
  
[DISTRIBUTION STATEMENT A] This material has been approved for public  
release and unlimited distribution.  Please see Copyright notice for  
non-US Government use and distribution.  
  
DM25-1262  
</legal>  

## What is POM?

POM stands for the Pointer Ownership Model. This is a model that can be used to prove temporal memory safety for all pointers in a C or C++ program that comply with it.  It is currently informal, we hope to formalize it soon.

If every pointer in a program conforms to POM, then the program is temporally memory-safe. It will have no use-after-frees and no dereferences of null pointers.

Memory safety is partitioned into temporal memory safety vs spatial memory safety. Temporal memory safety means that no pointer is kept after it is no longer safe to be dereferenced. Dereferencing a pointer after it has been freed is a time-based violation.  Spatial memory safety means that a pointer should not be incremented or decremented beyond the range over which it may vary. Doing an increment or decrement outside the intended range is a space-based violation.  Buffer overflows are spatial memory safety violations.

The POM should be intuitive...any C programmer who works with dynamic memory soon learns a discipline very similar to the POM.  It also manifests in C++ as RAII (Resource Acquisition Is Initialization).  A model very similar to POM is used in Rust.

POM will model pointers that are responsible or irresponsible. A /responsible/ pointer is a pointer that stewards an object on the heap, and makes sure the object's memory is freed (or delegated to another responsible pointer). An /irresponsible/ pointer is a pointer that is never involved with allocating or freeing memory; it merely points to global, local, or dynamic memory that is already managed by other pointers.

POM will not model any other type of pointer. This includes:
  * Reference-counted smart pointers, such as C++'s std::shared_ptr<>
  * Cyclic pointers, such as in doubly-linked lists
  * Windows handles
  * Pointers managed by some other mechanism, such as a garbage collector

The POM is not designed to verify that every valid C program is temporally memory-safe.  Some programs, even well-defined ones, violate POM. This simply means that you can't use POM to completely confirm their memory safety, and you must use some other mechanism (perhaps in addition to POM).

The POM builder and verifier are designed to assume that every pointer fits in POM, as either responsible or irresponsible. A pointer that does not comply with POM will cause the builder or verifier to report a POM violation.  For each error, the user should investigate the error. If they decide that the pointer is out-of-scope (i.e., it is managed by some other mechanism), then they should add this information to the model.  Once added, the POM builder and verifier will ignore the pointer.

In this document, a "heap object" is an object whose memory was allocated with malloc, calloc, aligned_alloc, or realloc(). Objects not allocated using one of these functions are not "heap objects" even some platforms cause them to live on the heap.

In POM, C pointers are partitioned into responsible pointers, irresponsible pointers, and out-of-scope pointers.  Out-of-scope pointers are just that, out of scope, and POM enforces nothing upon them. This means their safety needs to be checked by some other mechanism.  Responsible pointers steward the objects they point to; they always point to something on the heap that must be freed. Irresponsible pointers have no involvement with allocating or freeing memory, but they may point to anything (including on the stack or data segment, or into objects), and they may undergo pointer arithmetic.

The terms 'responsible', 'irresponsible' and 'out-of-scope' can be treated like type qualifiers in C (i.e., const or restrict). They sub-type the pointer variables, irrespective of what the variables' values are.  As with types, these qualifiers apply to a variable throughout its lifetime. For example, if `p` is considered to be a responsible pointer, it remains responsible throughout its lifetime, and cannot cease to be responsible.  These terms can apply to local pointers, pointers defined in structs or unions, and pointers defined as function arguments. They can also apply to the return value of a function if it is a pointer type.

For this project, we are not going to address C++. That will be future work.

### Conditional Compilation

Many C/C++ programs employ conditional compilation. That is, their code employs `#ifdef` or `#if` preprocessor commands. When such a program is compiled, some lines may be removed by the preprocessor...we often speak of these lines as "#ifdef'd out".  While lines that are ifdef'd out must still be tokenize-able, they may have other syntactic or semantic errors that might cause the program to fail to compile if the lines became live. Analyzing every line of a program would require compiling the code enough times to capture every ifdef'd-out line, which can be a huge task.  For this current project, we will table this question, by ignoring any lines that are ifdef'd out.  We will concern ourselves only with lines that appear when code is compiled in POM's container.

## ISO C pointers

Let us consider C pointers outside POM

The heap can be considered to consist of chunks of allocated memory, as well as regions of un-allocated (or freed) memory. A chunk of memory is any region of memory returned by a memory allocation function, which includes malloc() and its sister functions, realloc(), calloc(), and aligned_alloc(). Each chunk of memory has a lifetime that begins when it is allocated, and ends when it is freed.

A pointer always has a value, and this value is typically the numeric address of the memory that the pointer points to. This is distinct from the pointer's pointed-to value, which is the contents of the memory referenced by the pointer. Furthermore, every pointer (along with every other variable) has an address, which can be obtained with the & operator, this is the address of memory where the pointer's value lives.  Thus for pointers, it has an "address", a "value", and a "pointed-to value" (also known as a "referenced value"), which are three separate pieces of data.

The value of a C pointer can be in one of these three states: INVALID, NULL, VALID.  A pointer is INVALID when it is uninitialized, or its value is changed to something that is not part of a live chunk.  A pointer is VALID when it points to memory that is part of a live chunk. A pointer is NULL when it has the value 0 (often known as NULL).

In C, a pointer can be const or non-const, and its pointed-to value can also be const or non-const. Ideally, any function that does not change a pointer's value would mark it as const, but many functions fail to do so.  Likewise, any function that does not change the pointer's pointed-to value would mark it as const. When we speak of a "const pointer", we refer to the first type of constness, where the pointer's value does not change, but its pointed-to value may or may not change.

A const pointer can be passed to free() or fclose() without casting away its constness. So C constness does not provide that much use for POM.

Often a pointer's state can be unknown, so we consider combinations of these states. Combining INVALID with NULL or VALID is useless however, because it is undefined behavior to refer to the address value to a pointer that might be INVALID. (It is OK to refer to the address value of NULL, which is 0.)  Thus the only combination of states that is useful is VALID|NULL. For example, the malloc() function returns a pointer value that is VALID|NULL, and you can determine which by comparing the pointer with NULL.

When a pointer's value undergoes a state change, we may say that the pointer becomes the new state (e.g. a pointer becomes INVALID). A pointer becomes INVALID when it is declared without being initialized. It can also become INVALID if it was VALID but passed to free().  We sometimes say that a freed pointer value is a "zombie" because it is officially INVALID, but may look VALID...often the memory referenced by a freed pointer retains its original data and is not overwritten until some time later. A pointer becomes NULL by being assigned 0, which might happen if a call to malloc() fails. A call to malloc() that succeeds can make a pointer VALID. Any pointer assigned the value of another pointer also receives that pointer's state.

Like any other function argument, passing a pointer to a function copies the pointer, just like assignment does. So this code:

``` c
static char* p;
// ...
foo(p);

void foo(char* q) {
  // ...
}
```

is equivalent to:

``` c
static char* p;
// ...

foo();

void foo() {
  char* q = p;
  // ...
}
```

Many functions take pointers as arguments, and expect these pointers to have particular states, typically VALID. For example, the standard strlen() function expects its sole argument to be VALID (and also point to a null-terminated byte string).  Some functions also gracefully handle NULL pointers. For example, the free() function, when given a VALID pointer, converts it to INVALID. free() also accepts NULL pointers, and will do nothing when passed one.  Functions that return pointers also constrain the states of these pointers. For example, malloc() returns either a VALID pointer, or NULL.

While many pointers point to memory on the heap and must be eventually freed, there are other types of pointers that follow a similar model. For example FILE pointers are returned by fopen(), and eventually 'freed' by fclose(). Thus, we would say that file pointers can also be INVALID, VALID, or NULL, and they differ from heap pointers only in that they must be passed to fclose() rather than free().  Consequently, POM can be applied to any pointers that must eventually be passed to some function that deallocates it, whether that function be free(), fclose(), or some more idiosyncratic function.

## Responsible pointers

Responsible pointers are the subset of pointers that shepherd heap memory. Each chunk of heap memory may be accessed directly at most by one good responsible pointer. That is, the pointer references the initial byte of that chunk. This pointer is responsible for ensuring that the memory gets freed.

Responsible pointers may never point inside a heap object; they may only point to its beginning. Also, they never point to within the stack or data segment.

Like ISO C pointers, responsible pointers have states that can change over their lifetimes. The states for responsible pointers are ZOMBIE, NULL, and GOOD. A GOOD pointer in POM is similar to a VALID pointer in ISO C, but a bit more limited:  A pointer that is GOOD is always VALID, but the reverse is not always true.  Likewise, a ZOMBIE pointer in POM is analogous to an INVALID pointer in ISO C. An INVALID pointer is always a ZOMBIE, but the reverse is not always true.  A NULL pointer has the exact same meaning in POM as it does in ISO C. 

There are two common constructs in C that present problems for responsible pointers: assignment and function arguments.  Both involve copying the pointer's address from one pointer to another.  How can POM handle assignment expressions and function arguments that involve responsible pointers?  Any assignment expression where the right-hand-side indicates a responsible pointer can have two interpretations: The left-hand-side could indicate an irresponsible pointer, which is allowed...see the [Irresponsible Pointers](#irresponsible-pointers) section for details. Or the left-hand-side could indicate a responsible pointer, in which case the right-hand-side pointer is no longer considered GOOD. While it is still a responsible pointer, it has "delegated ownership" of the pointed-to object to the assigned-to pointer.

As such, the states of responsible pointers vary slightly from ISO C pointers.   In particular, the states of responsible pointers employ move semantics. If a responsible pointer is assigned the value of a second responsible pointer, that second responsible pointer changes its state to ZOMBIE, just as if it had been freed. Referencing the freed value of a responsible pointer is undefined behavior, and violates POM.  Dereferencing a responsible pointer whose value has been assigned but not freed is valid C, but it violates POM.  Thus, a responsible pointer whose value was assigned away becomes a zombie or ZOMBIE in the POM, although it still points to GOOD memory in ISO C.

One consequence of these semantics is that a function argument pointer should be declared 'responsible' only if the function might 'consume' the argument. By 'consume', the function might free the pointer, or assign it to another responsible pointer.  Many functions that take pointers do nothing to modify the pointer's state or lifetime. By convention, these functions will declare their pointer argument as diligent (see the [Diligent Arguments](#diligent-arguments) section for details), but they can take responsible or irresponsible pointers. (Also, but see the [Pointer to Pointer](#pointer-to-pointer) section about pointer-to-pointer arguments).  For more information about function pointer arguments, see the [Functions and Pointers](#runctions-and-pointers) section for details.

Responsible pointers are similar to Rust's Box<> type.

Responsible pointers avoid the aliasing problem by enforcing (at compile time) that for each chunk on the heap there is exactly one good responsible pointer that points to it.

Note that some non-dynamic C pointers can be responsible. Responsible pointers are technically pointers that should eventually be passed to a de-allocation function. That function is usually free()...but could be something else. For example, C++ pointers allocated by the new operator should not be passed to free(), but instead should be passed to the delete operator. Likewise, C FILE pointers are generally returned by the fopen() function, and should be passed to the fclose() function.  Responsible pointers is passed to the wrong de-allocation function. For example, it is undefined behavior to pass a pointer allocated with the new operator to free(), or to pass a pointer allocated with malloc() to the delete operator.  To handle such pointers, any responsible pointer should indicate which de-allocation function should handle it, and if left unspecified, this defaults to free().  For the remainder of this document, we will assume each responsible pointer should be passed to free() unless specified otherwise.

### Properties

Throughout this document, many code comments indicate types and states of pointers, e.g., responsible, irresponsible, zombie, etc. In some cases, the code snippet will provide sufficient context for that type and/or state. In other cases, the code context is insufficient, but we explicitly describe that type and/or state, and you should use that to understand the code and explanation in that section.

#### A responsible pointer should never be assigned the value of an irresponsible pointer
#### A responsible pointer should never be assigned a value from pointer arithmetic
``` c
char *rp1 = ...; // responsible
char *rp2; // responsible
rp1 = rp2 + 1; // Violates POM
```
#### A responsible pointer should never be assigned a value created by &
``` c
char *rp1 = ...; // responsible
char *rp2; // responsible
rp1 = &rp2; // Violates POM
```
But see the [Producer Arguments](#function-producer-argument) section for an exception.
#### A responsible pointer can be assigned a return value or argument from a function.
``` c
char *rp1; // responsible
rp1 = malloc(5);  // ok, rp1 is GOOD|NULL
char *rp2; // responsible
getline( &rp2, 80, stdin); // ok, rp2 is GOOD|NULL
```

See [here](https://www.gnu.org/software/libc/manual/html_node/Line-Input.html) for more information about GNU getline().

#### A responsible pointer can be assigned the value of another responsible pointer
``` c
char *rp1 = ...; // responsible
char *rp2; // responsible
char *rp3; // responsible
rp1 = rp2; // ok, rp1 gets rp2's state, rp2 becomes ZOMBIE
memcpy( &rp3, &rp1, sizeof( rp3)); // ok, rp3 gets rp1's state, rp1 becomes ZOMBIE
```

Note that double-assignment can produce surprising results. See [here](src/test/assignment_chain.pom.yml) for more information.

#### A responsible pointer can be set to NULL
``` c
char *rp; // responsible
rp = NULL; // ok
```
#### A responsible pointer becomes ZOMBIE when freed
``` c
//  Suppose nums is an array of responsible pointers

// new_num must be a good responsible pointer, but becomes ZOMBIE
void free_num(int *(nums[]), int size, int* new_num) {
  free(new_num);
}

void call_free_num(int *(nums[]), int size, int* new_num) {
  free_num(nums, size, new_num);
  printf("new_num is %d\n", *new_numn); // use-after-free, violates POM
}
```
#### A responsible pointer also becomes ZOMBIE by being assigned to another responsible pointer
``` c
// new_num must be a GOOD responsible pointer, becomes ZOMBIE
void push_num(int *(nums[]), int size, int* new_num) {
  nums[size++] = new_num;  // new_num becomes ZOMBIE
}

// new_num must be a GOOD responsible pointer, becomes ZOMBIE
void call_push_num(int *(nums[]), int size, int* new_num) {
  push_num(nums, size, new_num);
  printf("new_num is %d\n", *new_numn); // well-defined C, but violates POM!
}
```

``` c
int *(nums[5]);  // array of responsible pointers to int
int size = 0;

int *new_num = malloc(sizeof(int));  // responsible
if (new_num == NULL) goto error;
*new_num = 123;
push_num(nums, size, new_num);  // nums[0] becomes GOOD, new_num becomes ZOMBIE
nums[0]++; // 124. Complies with POM
new_num++; // 125, Well-defined, but new_num is ZOMBIE, so violates POM
int *old_num = &nums[0]; // irresponsible
free(nums[0]); // nums[0] becomes ZOMBIE, and old_num should never be used again.
```

#### A responsible pointer should be de-allocated by its matching de-allocation function.

``` c
char *rp = malloc(size); // responsible, GOOD|NULL
if (rp == NULL) abort(); // GOOD
fclose(rp);              // Oops, wrong deallocation function!
```

``` c
FILE *f = fopen("foo.txt"); // responsible, GOOD|NULL
if (f == NULL) abort();     // GOOD
free(f);                    // Oops, wrong deallocation function!
```
<a name="irresponsible-pointers"></a>
## Irresponsible pointers

Irresponsible pointers are not responsible for cleaning up the memory they point to.  Since they do not participate in memory allocation or freeing, the main concern is that they respect temporal memory safety.  Thus, they must not outlast the object that they point to.

Irresponsible pointers are similar to Rust's reference type

Irresponsible pointers do not prevent the aliasing problem. POM must simply guarantee that an irresponsible pointer cannot outlast the lifetime of whatever it points to, thus preventing temporal memory errors.  This can be achieved by a technique similar to Rust's borrow checker.

### Properties
#### An irresponsible pointer can be assigned a value from pointer arithmetic
``` c
int *p = ... // responsible
int *q = p + 1; // q must be irresponsible
p++; // Only irresponsible pointers can be incremented, so violates POM
```
#### An irresponsible pointer can be assigned a value created by &
``` c
int i = 123;
int r = &i; // irresponsible
```
#### An irresponsible pointer should never be assigned the return value of a function that returns a responsible pointer (such as malloc())
``` c
char *ip1; // irresponsible
ip1 = malloc(5);  // Violates POM
```

#### An irresponsible pointer should never be passed to a function that expects a responsible pointer, such as free()
``` c
char *ip1; // irresponsible
getline( &ip1, 80, stdin); // Violates POM, 1st arg of getline() must be responsible
char *ip2; // irresponsible
free( ip2); // Violates POM
```

See [here](https://www.gnu.org/software/libc/manual/html_node/Line-Input.html) for more information about GNU getline().
#### An irresponsible pointer should never be copied over to a responsible pointer
``` c
char *ip = ...; // irresponsible
char *rp;       // responsible
rp = ip;        // Violates POM
```

#### A pointer can never be both responsible and irresponsible
``` c
int* p;
  {
  p = malloc(...);  // p must be responsible
  ...
  free(p);
  }
...
  {
  p++; // Violates POM, p must be irresponsible
  ...
  }
```

### Lifetimes

Since irresponsible pointers do not participate in memory management, the only liability for irresponsible pointers is that an irresponsible pointer might live longer than the object it points to. Due to the aliasing problem, we do not expect to track the value of every irresponsible pointer. Consequently, we track their lifetimes, that is, the lifetime of every object that an irresponsible pointer can reference.  When we speak of an irresponsible pointer's lifetime, we mean the lifetime of the object the pointer may reference.  An irresponsible pointer may be assigned to a different object, but that object must live at least as long as the object formerly referenced by the pointer.  In fact, every object has a lifetime; it is only irresponsible pointers whose objects' lifetimes must be explicitly tracked.  An irresponsible p pointer can have several lifetimes, depending on how many responsible pointers it references. 

By default, irresponsible pointers that are function arguments are assumed not to outlast the function call; their lifetimes need not be specified unless their referenced object outlasts the function call.  The same applies for local irresponsible pointers...as long as their referenced objects do not outlast the function call, their lifetimes can be left unspecified.  However, the verifier should confirm that local irresponsible pointers do not outlast the objects they point to.

Irresponsible pointers that serve as function return values are different; they are clearly intended to outlast the function call. They must provide an explicit lifetime, which will typically match the lifetime of one of the function arguments.  Alternately, they could specify the lifetime "static", which means their object lives until the program terminates.

Irresponsible pointers affect other types that contain them, such as arrays, structs, and unions. Any composite type that can contain an irresponsible pointer should have a lifetime specified; this can be used to indicate the lifetime of the irresponsible pointers contained in the composite type.

The question arises of when irresponsible pointer function arguments need lifetimes. The answer is that if a irresponsible pointer's address might outlive the function, then its lifetime should be specified. This will typically occur only in complex types such as pointer-to-pointer or array-of-pointer arguments.

There should be no pointers that point to an INVALID irresponsible pointer. But see the [Producer Arguments](#function-producer-argument) section for an exception.

### Mutability

Rust has a `mut` keyword which means the opposite of C's `const` keyword...In Rust, a variable that is not declared `mut` may not be modified once it has been initialized.  Rust also has the rule that any object may be either 'borrowed mutably' exactly once, or 'borrowed immutably' multiple times. For POM, we would enforce the constraint that any object may be referenced by any number of immutable irresponsible pointers, or by no more than one mutable irresponsible pointer.  Thus an irresponsible pointer may be declared 'mutable', meaning that you can use that pointer to modify the object it references. (This 'mutable' keyword does not affect your ability to assign the irresponsible pointer to point elsewhere...that is always permitted.)

To be precise, an irresponsible pointer variable is mutable if the memory referenced by the variable could be changed via the variable.  In this sense, mutable corresponds to a const* rather than a *const.

To understand the definition of mutable, it helps to understand C's concept of const.  Consider this Rust code example:

``` rust
let mut i32 x = 42;
let r2 = &mut &mut x;
**r2 = 43;
x = 43;
```

This corresponds to this code in C:

``` c
int x = 42;
int **const r1 = &&x;
// r1 is a mutable irresponsible pointer to another mutable irresponsible pointer
**r1 = 43;
x = 43;
```

Rust forbids trying to modify a non-mutable value:

``` rust
let mut r1 = &x;
*r1 = 43;  // error, behind a non-mutable ref
```

``` c
int const* r1 = &x;  // r1 is non-mutable irresp ptr
*r1 = 43; // error, behind non-mutable ptr
```

Finally, Rust's concept of 'mutable' applies to all objects. But POM is more limited, we are only applying the "mutable" term to irresponsible pointers. (To be fair, all responsible and producer pointers are mutable and all diligent pointers are immutable).

For example, incrementing an irresponsible pointer variable is permitted whether or not the pointer is mutable.

As a final example, the 'dest' argument to the strcpy() function is a mutable irresponsible pointer, because it receives the character data referenced by the `src` argument.  While POM does not care about the non-pointer contents of the dest or src arrays, a system that also enforced concurrency safety would care about this...and a concurrency-safety module could be built atop POM, as one has been integrated into Rust atop its borrow checker.

Any pointer to const that is irresponsible is considered not to be mutable, but a pointer to non-const may or may not be mutable.  (One could resolve such ambiguities by sprinkling 'const' throughout the code, but we are assuming that the code itself cannot be changed.)

Furthermore, any heap object may not be freed if it has any irresponsible pointers pointing to it.

### Properties
#### There may not be both mutable and immutable irresponsible pointers to the same object.
``` c
int *p = ... // responsible
int *ir1 = p; // irresponsible, immutable
int *ir2 = p; // irresponsible, mutable, violates POM!
```
#### There may not be more than one mutable irresponsible pointers to the same object.
``` c
int *p = ... // responsible
int *ir1 = p; // irresponsible, mutable
int *ir2 = p; // irresponsible, mutable, violates POM!
```
#### You may not cannot pass a responsible pointer to a function expecting a responsible pointer if there is one or more irresponsible pointers pointing to the same object as the responsible pointer.
``` c
int *p = malloc(...); // responsible
int *ir1 = p; // irresponsible
free(p); // violates POM!
```
## Out of Scope Pointers

POM doesn't apply to this pointer; the pointer is handled some other way.

As far as POM is concerned, the program can co anything with out-of-scope pointers (in contrast, POM imposes limits on irresponsible pointers).

### Properties
#### An out-of-scope pointer shall not be assigned the value of a responsible pointer

## States of Responsible Pointers

Responsible pointers can be in one of the following states. The state may not always be known, in which case the pointer can be tracked as occupying multiple states. These states can be resolved later in the code.

### NULL
#### A responsible NULL pointer shall never be dereferenced

``` c
char *rp = NULL; // responsible
*rp = '\0'; // Violates POM
```
### GOOD
Pointer points to memory that must be freed, and no other good responsible pointer points to this memory

``` c
char *ptr = new char[5]; // good
char *ptr2 = malloc(5);  // Consistent (good or null)
if (ptr2 == NULL) abort();
// if we get here, ptr2 is good
```
#### A GOOD pointer shall never be overwritten

``` c
char *rp = new char[5]; // responsible
rp = new char[7]; // Violates POM
```
#### A responsible pointer shall never go out of scope while GOOD

``` c
{
  char *rp = malloc(5); // responsible, consistent (good or null)
}  // Violates POM, memory might be leaked

struct foo_s {
  char *c; // responsible
} *s;
s = new struct foo_s;
s->c = new char[3];
// ...
delete s; // Violates POM, s->c's memory lost
```
### ZOMBIE
The pointer's value has either been assigned to another responsible pointer, or freed or uninitialized.

``` c
free( ptr); // ZOMBIE
ptr = ptr2; // ptr2 now ZOMBIE, ptr now GOOD
```

``` c
char *ptr; // uninitialized, ZOMBIE
char *ptr2 = ptr;  // also uninitialized, ZOMBIE
memcpy( &ptr2, &ptr, sizeof( ptr)); // still uninitialized, ZOMBIE
```
#### A ZOMBIE pointer's value shall never be read
``` c
char *rp = malloc( 5); // responsible, consistent (good or null)
free( rp); // zombie or null
char *rp2 = rp; // Violates POM
```

``` c
char *rp; // responsible
char *irp; // iresponsible
irp = rp; // Violates POM
```

#### A ZOMBIE pointer shall never be freed
``` c
char *rp = malloc(5); // responsible, consistent
free(rp); // zombie
free(rp); // Violates POM (double free)
```

``` c
char *rp; // responsible
free( rp); // Violates POM
```
#### A ZOMBIE pointer shall never be dereferenced
``` c
char *rp = malloc(5); // responsible, consistent
free(rp); // zombie
rp[0] = '\0'; // Violates POM
```

``` c
char *rp; // responsible
*rp = '\0'; // Violates POM
```
#### There should be no pointers that point to the same value as a ZOMBIE pointer

But see the [Producer Arguments](#function-producer-argument) section for an exception.

``` c
char *rp = malloc(5); // responsible, GOOD|NULL
free(rp); // zombie
char *irp = &rp; // Violates POM
```

``` c
char *rp; // responsible, ZOMBIE
char *irp = &rp; // Violates POM
```
### Consistent
A responsible pointer that is either GOOD or NULL
## Responsible Pointer State Changes
The sub-types of pointer variables never changes...once a responsible pointer, always a responsible pointer.
But the states of responsible pointers can change in the following ways:

The [Function Return Values](#function-return-values) section has good examples of responsible pointers being passed in to and out of functions.

### Null check
A pointer that may be GOOD or NULL can be disambiguated by a null check.
A responsible pointer that is a ZOMBIE should not be checked for NULL, even inspecting a ZOMBIE pointer's value is undefined behavior.

``` c
char *rp = malloc(5); // responsible, consistent (GOOD or NULL)
if (rp == NULL) {
  // rp is now NULL
  abort(); // or any noreturn function
} else {
  // rp is now GOOD
}
// rp is now GOOD
```
### Assignment
This includes any read of one responsible pointer and write of the value into another responsible pointer.  The pointer on the left-hand-side (LHS) of the assignment receives the state of the pointer on the right-hand-side (RHS). The RHS pointer's state becomes ZOMBIE.

``` c
char *rp1 = ...; // responsible
char *rp2;
rp2 = rp1; // rp2 gets rp1's state. If rp1 was GOOD, it is now ZOMBIE
memcpy( &rp1, &rp2, sizeof( rp1)); // rp1 gets rp2's state, if rp2 was GOOD, it is now ZOMBIE
```

Note that assigning a responsible pointer value to an irresponsible pointer does not change the responsible pointer's state.
### Function Argument
Like assignment, passing a responsible pointer to a function that accepts a responsible pointer as argument copies its state, and the original pointer becomes a ZOMBIE.
``` c
void foo(char* ptr); // responsible

char *rp1 = ...; // responsible
foo(rp1);  // foo's ptr gets rp1's state. If rp1 was GOOD, it is now ZOMBIE
```

### Function Return Value
Some functions return GOOD or NULL pointers, and some only return GOOD pointers.

<a name="function-producer-argument"></a>
### Function Producer Argument
Some functions take pointer-to-pointer arguments, and modify the inner pointer, or change its state via malloc() or some other memory-management function.

Many C functions can indicate errors preventing them from completing normal execution. There are many ways in C to indicate errors, such as returning error values such as NULL, or using the global `errno` variable.

## Functions and Pointers

In this section we consider function arguments that are pointers.

A pointer that is into a function could be responsible, irresponsible, or out-of-scope. It will also be an a subset of possible states. While the pointer's responsibility never changes during the function's execution, the pointer's state may change.  Thus, every pointer argument has an initial set of states, and a final set of states. These can be identical but need not be. For example, many functions will take an irresponsible pointer that is expected to be VALID (that is, not null, and points to allocated memory).

We can classify function pointer argument responsibility into five states: out-of-scope, diligent, irresponsible, responsible, or producer. 

#### Out-of-scope Arguments

These are pointers outside our model.

<a name="diligent-arguments"></a>
#### Diligent Arguments

These are pointers that could be either irresponsible or responsible. In particular, during the function execution:

 * They always point to valid memory or NULL and are never assigned to point somewhere else in the function
 * The memory they point to is never freed or consumed.
 * Their value never escapes the function; their value is never assigned to a pointer that outlives the function
   There is one exception: The function may return a diligent argument's value as an irresponsible pointer.
 
Any pointer may be passed to a function as a diligent argument.

A function's pointer argument can be diligent. It can (and probably should) be diligent if the function neither frees the pointed-to memory, nor assigns it to a responsible pointer. It is permissible to pass any pointer (responsible, irresponsible, or out-of-scope) to such a function.

Diligent arguments may or may not be NULL. They may be required to VALID or they may be NULL|VALID.

##### Example: strcpy

For example, strcpy() takes two irresponsible pointers, and returns its destination string. You can pass it any pointers, as long as they point to VALID memory.

``` c
char *ptr1, *ptr2 = ..., *ptr3 = ...;
ptr1 = strcpy(ptr2, ptr3);  // ptr1 == ptr2. No state changes if any ptrs were responsible
free(ptr1); // Only valid iff ptr1 was responsible. ptr1 now ZOMBIE
```

Note that the return value's lifetime matches ptr2's lifetime (since it is ptr2).

##### Example: main

The argv pointer of main() is a pointer to pointer to char. Both pointers are diligent, as they must not be freed or reassigned.

``` c
int main(int argc, char** argv) {
  // ...
  return 0;
```

#### Irresponsible Arguments

An argument whose value is never freed or consumed, but might be assigned to point elsewhere or have its pointer reassigned should be irresponsible.

These pointers' start states and end states can be noted.

##### Do not pass a responsible pointer to a function expecting an irresponsible pointer argument
##### Example: Irresponsible Swap

This function takes two pointers to mutable irresponsible pointers, and swaps their values.

While both pointers may be VALID or NULL when being invoked, and they remain VALID or NULL afterwards, this function should explicitly list their end states.  Also, this function ties its arguments' lifetimes together, since POM is not sophisticated enough to recognize that they are swapped.

``` c
void swap(char** a, char** b) {  // both pointers irresponsible, mutable, and [VALID, NULL]
  char** tmp = *a;                // and have the same lifetimes
  *a = *b;
  *b = *tmp;
}
```

The swap() function is a good illustration of why you cannot pass a responsible pointer to a function that expects an irresponsible pointer. If you could, then you could give swap a responsible pointer, and an irresponsible pointer, and it would happily swap them, breaking memory safety!

#### Responsible Arguments

A pointer that is consumed by the function should be declared as responsible.

For example, the free() function takes a responsible pointer, which is either GOOD or NULL, and converts it to a ZOMBIE.

A function with one or more consumer arguments may change the arguments' state distinctly based on whether it produced an error or not. For example, the realloc() function takes a responsible pointer as its first argument. If the pointer was NULL, it remains NULL. If it was GOOD, and realloc() returns a non-NULL pointer, it becomes a ZOMBIE. If realloc() returns NULL, the pointer's state remains unchanged.  So this argument pointer is similar to free()'s argument pointer, but only when realloc() succeeds.  But realloc() also returns a responsible pointer, which is either GOOD or NULL like malloc().

These pointers' start states and end states can be noted.

##### Do not pass an irresponsible pointer to a function via a responsible pointer argument.
##### Example: free() and fclose()

For example, the free() function and the fclose() function both convert GOOD responsible pointers to ZOMBIE.  But free(NULL) is explicitly defined to do nothing, while fclose(NULL) is undefined behavior. So fclose() requires its argument to be GOOD, but free() does not.

``` c
char *rp = ...; // responsible, GOOD
free(rp); // rp now ZOMBIE
FILE* fp = ...; // responsible, GOOD, (refers to open file)
fclose(fp); // fp now ZOMBIE
```

##### Example: realloc()

For example, the realloc() function's first argument is a void*, and second argument is a size to allocate. According to ISO C, the size must not be 0.  If the first argument is NULL, realloc() passes it to malloc() and returns the result.  Otherwise, realloc() returns a pointer to the new chunk of size bytes (which may or may not match the first argument. The first argument becomes a ZOMBIE and realloc() returns a responsible pointer which may be NULL or GOOD.

``` c
char *rp = ...; // responsible, NULL|GOOD
char *new_rp = realloc( &rp, 6); // (new_rp GOOD, rp now ZOMBIE) | (new_rp NULL, rp GOOD)
if (new_rp == NULL) { 
  // realloc() failed, rp still GOOD
} else {
  // srealloc() succeeded, new_rp GOOD, rp ZOMBIE
}
```

##### A consumer argument must always be provided by a responsible pointer

``` c
char *ptr = ...; // responsible, GOOD
free(ptr+3) // Violates POM
```
##### Example function that consumes a responsible pointer without calling free()

The following function consumes its second argument, because its second argument no longer needs to be explicitly freed; it is incorporated into a responsible object.

``` c
typedef struct foo_s {
  char *a;  // responsible
  char *b;  // irresponsible
  // The lifetime of foo_s should match the lifetime of b
};

// foo and a are both responsible, GOOD
void function2(struct foo_t* foo, char *a) {
  assert(foo->a == NULL);
  foo->a = a;  // foo->a GOOD again
}
```

#### Producer Arguments

This is a pointer-to-pointer argument with the following special properties:

A 'function producer argument' has some special properties:
 * The outer pointer must be constant, that is, the function may not assign it to point elsewhere.
 * The outer pointer may not be freed or consumed
 * Any call to the function must either provide a responsible outer pointer
   or must provide the address (&) of a pointer.

In these cases, the outer pointer is treated as responsible, except that it may not be consumed or freed.  The inner pointer may be responsible, irresponsible, or out-of-scope

A function producer argument that points to responsible pointers may free or otherwise consume them.

Taking the a address of an INVALID irresponsible pointer or a ZOMBIE responsible pointer is permitted if and only if it is immediately passed to a function expecting a producer argument, and the function expects the inner pointer to be INVALID or ZOMBIE.

If the outer pointer was responsible, you could free it:
``` c
int *ptr = malloc(sizeof(int)); // responsible
if (ptr == NULL) {abort();}
foo(&ptr);

// ...

bool foo(int **ptr) { // responsible ptr of responsible ptr
  free(ptr);  // oops, tried to free a stack variable
}
```

If the outer pointer was irresponsible, you could use it to circumvent the responsible pointer aliasing:

``` c
int *ptr = malloc(sizeof(int)); // responsible
if (ptr == NULL) {abort();}
*ptr = 123;
int **iptr = &ptr; // irresponsible
foo(&iptr);
printf("The number is %d\n", *ptr);  // oops, use-after-free

// ...

bool foo(int **ptr) { // irresponsible ptr of responsible ptr
  free(*ptr);         // ok
}
```

As another example:

``` c
int *ptr1 = malloc(sizeof(int)); // responsible
int *ptr2 = malloc(sizeof(int)); // responsible
if (ptr1 == NULL || ptr2 == NULL) {abort();}
*ptr1 = 123;
*ptr2 = 456;
int **iptr = rand(2) > 1 ? &ptr1 : &ptr2; // irresponsible
foo(&iptr); // ptr1 or ptr2 is freed, but which one?
printf("The first number is %d\n", *ptr1);  // oops?

// ...

bool foo(int **ptr) { // irresponsible? ptr of responsible ptr
  free(*ptr);         // OK
}
```

A function with one or more producer arguments may change the arguments' state distinctly based on whether it produced an error or not. 

##### Do not pass an irresponsible pointer to a function that expects a producer argument
##### Do not create an address to a responsible pointer that is not GOOD, unless it is then passed to a function that expects a producer argument.
##### Example: Responsible Swap

This function takes two pointers to responsible pointers, and swaps their values.

``` c
void swap(char** a, char** b) {  // both pointers producers and GOOD|NULL
  char** tmp = *a;
  *a = *b;
  *b = *tmp;
}
```

The code is the same as for the 'irresponsible swap' function listed above...both pointers can be responsible or irresponsible.  However, passing the wrong kind of pointers to the function would break POM.

##### Example: GNU getline()

The GNU getline() function's first argument is a **char, and second argument is a *size. If the *char argument is NULL and the *size is 0, the first argument might be converted to GOOD (via malloc()).  While the getline() function returns -1 on error, this is not sufficient to know the pointer's state...one must compare *lineptr to NULL, according to the [getline() source code](https://github.com/coreutils/gnulib/blob/master/lib/getdelim.c).  (This is not precisely specified by the [GNU info docs](https://www.gnu.org/software/libc/manual/html_node/Line-Input.html).)

``` c
char *rp = NULL; // responsible, NULL
int size = 0;
int result = getline( &rp, &size, stdin); // rp now GOOD or NULL
if (result != -1) { 
  // getline() succeeded, rp now GOOD
} else if (rp != NULL) {
  // rp GOOD, at EOF, so no bytes to read
}
```

## Function Return Values

Function return values that are pointers can only be out-of-scope, irresponsible, or responsible. Irresponsible pointers must have a lifetime specified, usually tied to one of the function arguments. 

For example, the strcpy() function returns an irresponsible pointer that does not outlive its first argument, because it actually returns its first argument.

#### Example: malloc() and operator new

For example, both the malloc() and strdup() functions produce responsible pointers, that may be GOOD or NULL.  The C++ new operator always produces a GOOD value (it never returns NULL).

``` c++
char *rp1; // responsible
rp1 = malloc(5); // rp1 is now consistent (GOOD or NULL)
char *rp2; // responsible
rp2 = new char[5]; // rp2 is now GOOD (can't be NULL)
```

#### Any function that returns a responsible pointer must have its return value assigned to a responsible pointer

``` c
malloc(5); // Violates POM, value not assigned to responsible pointer
```

#### A function can return a responsible pointer without calling malloc().

A function that split a responsible pointer from a responsible object and returned the pointer should return a responsible pointer. The following is one such function because it returns a pointer that must be subsequently freed.

``` c
typedef struct foo_s {
  char *a;  // responsible
  char *b;  // irresponsible, mutable, lifetime bound to lifetime of foo_s
};

char *function(struct foo_s* foo) {
  char *tmp = foo->a;
  foo->a = NULL;
  return tmp;
}
```

## Types That Contain Pointers

In this section, we define a *composite type* is any type that can contain a pointer. Composite types consist of structs, unions, arrays, and pointers that contain a composite type, and pointer types. C often allows such objects to be created, and they are even more popular in C++.  We distinguish these from *non-composite types, which include structs, unions, and arrays that do not contain any pointers.  A composite object is an object of a composite type.  A responsible composite object is a composite object with at least one responsible pointer, and an irresponsible composite object is a composite object with at least one irresponsible pointer.

A responsible composite object with exactly one responsible pointer has the same responsible states as the pointer...that is if the responsible pointer is GOOD, the composite object's responsible state can be inferred as GOOD.  A composite object with more than one responsible pointer will also have a responsible state derived from the responsible pointers' states. Likewise, a composite object with exactly one irresponsible pointer itself can have the same states as the pointer...that is it the irresponsible pointer is VALID, the composite object's irresponsible state can be inferred as VALID.  A composite object with more than one irresponsible pointer will also have an irresponsible state derived from the responsible pointers' states.

In C many heap objects are only indirectly accessible. For example, the third element in a linked list.

Definition: A "C-path" is a way to access any object in memory in C. It starts off with a global or local variable, and then consists of a (possibly-empty) sequence of:
 * array accesses:        `a[i]`
 * pointer dereferences:  `*p`
 * struct membership:     `s.a`
 * union membership:      `u.a`
As you might guess, C-paths are a lot like file paths. Composite types are what one uses to build networks of heap objects in memory. The pointers must be VALID or GOOD.
 
A heap object that can't be referenced by any C-paths indicates a memory-leak. If there are no memory leaks in a program at a particular point in time, then every heap object has >=1 C-paths to reference it.
 
In a memory-safe program with no out-of-scope pointers, at any point during program execution, every heap object has exactly one C-path where every pointer in the path is responsible. More importantly, in any such program, every heap object has one "responsible C-path". A "responsible C-path" is a C-path where each pointer is the sole pointer for stewarding its pointed-to object. This means that either the pointer is irresponsible and mutable, or the pointer is responsible and there is no mutable irresponsible pointer pointing to the same object. (Note that this requires assuming that alloca() can only return irresponsible pointers, but this is not a problem because supporting alloca() is future work.)  Note that in a responsible C-path, variables before the first pointer (if any) live on the stack or global segment, and everything past the first pointer must live on the heap.

This means that it is a POM violation to free a pointer via a C-path that has at least one immutable irresponsible pointer in it.  It is also a POM violation to free a pointer via a C-path that has a separate mutable irresponsible pointer pointing to one of the elements in the C-path.  It is also a POM violation to send a responsible pointer to a function expecting a responsible pointer or a producer of a responsible pointer without using a responsible C=path. And assigning one responsible pointer to another is forbidden unless both are specified via responsible C-paths. 

``` c
free(x->next->next->data[1].ptr);   // Invalid if any pointers are irresponsible
```

Function producer arguments add a minor wrinkle. As these are pointer-to-pointers, the inner pointer is responsible, and the outer pointer is a 'producer'. In such cases, the inner pointer may be freed or treated like any responsible pointer, but the outer pointer may not be freed or modified (it must be const).

Any responsible pointer that is accessed via a non-responsible C-path should be treated as diligent. That is, it cannot be passed to a function that expects a responsible pointer, because the function will consume the pointer, and thus cause a dangling responsible C-path.

### Array
All the pointers in an array must have the same responsibility. 

For POM, we will associate each array with a 'start' element and an 'end' element, which together indicate the range of elements in the array that have been initialized. This is in contrast to the 'capacity', the maximum number of elements that could be in the array without resizing it. (The start and end could be equivalent to 0 and the capacity, indicating the entire array, but the end element cannot be greater than the capacity.  Any array elements between the end element and capacity are assumed to be unused and INVALID or ZOMBIE, and any array elements between element 0 and the start element are also o assumed to be unused and INVALID or ZOMBIE.  The start and end indices can vary (typically they begin at 0 and end increases as array elements are initialized).  Cleanup can begin with the start element or the last element, this allows cleanup to go from start to end or from end to start.

All the composite objects in an array must have the same responsibility. They must all be responsible, or they must all be irresponsible, or they must all be out-of-scope. Their states can vary, and the array's state is the union of all the states of the individual composite objects.

For arrays of irresponsible pointers, all of the pointers with valid values must have the same lifetime and mutability.  That is, they must all be mutable or they must all be immutable.

``` c
const int size1 = 10;
const int size2 = 12;

// Return and *return are both responsible and GOOD.
// Return's size is size1, *return's size is size2.
int **create_matrix() {
  // array, *array will both be responsible, just like return value
  int **array = calloc(size1, sizeof(int *));
  if (array == NULL) {abort();}
  for (int i = 0; i < size1; i++) {
    array[i] = calloc(size2, sizeof( int));
    if (array[i] == NULL) {abort();}
  }
  return array;
}
```

 <a name="pointer-to-pointer"></a>
### Pointer to Pointer
A pointer to another pointer is equivalent to a pointer to an object. It can be considered as a struct with one pointer, and that pointer may be responsible, irresponsible, or out-of-scope.

Like arrays, pointer-to-pointers must be declared with the variable declaration itself.

``` c
char **cpp1
struct char_ptr {
  char *data;
};
struct char_ptr *cpp2;  // cpp1 is functionally equivalent to cpp2
```
#### pointer-to-pointer responsibility

The outer pointer may be responsible, irresponsible, or out-of-scope, and the inner pointer may also be responsible, irresponsible, or out-of-scope.

``` c
char **cpp1, **cpp2; // must declare resposnibility of cpp1, *cpp1, cpp2, *cpp2
// cpp1 and cpp2 are same type only if resp(cp1) == resp(cp2) && resp(*cp1) == resp(*cpp2)
int **ipp1; // distinct type from cpp1 regardless of responsiiblity

char **i_r_p = ... // irresp ptr to resp pointer
*i_r_p++; // Violates POM, resp pointer cannot be changed
char **r_i_p = ... // resp ptr to irresp pointer
*r_i_p++; // ok
```

#### A responsible inner pointer may only be allocated or freed when the outer pointer is responsible or a producer.

``` c
char **rrp1; // responsible of responsible pointer
int **irp1; // irresponsible of responsible pointer

rrp1 = malloc(sizeof (char*));
if (rrp1 == NULL) {abort();}
*rrp1 = malloc(sizeof (char));
if (*rrp1 == NULL) {abort();}
irp1 = rrp1;
free(rrp1); // OK (if prev free doesn't happen)
free(*irp1); // Violates POM, irp is irresponsible. Also *irp1 is already freed
```

As noted above, a function that takes a pointer-to-a-pointer argument can have the outer pointer be a 'producer' provided it obey the conditions outlined above.

#### Do not allow a pointer to a responsible pointer to substitute for a pointer to an irresponsible pointer

``` c
int *rp = malloc(sizeof(int)); // resp
int *irp = rp; // irresp
int *irp2 = ...; // irresp
swap(&irp, &irp2); // OK (for irresponsible swap)
swap(&irp, &rp);  // Violates POM!
```

It is OK if an address (returned by malloc()) becomes referenced by an irresponsible pointer. But it is not OK if a responsible pointer is mistaken for an irresponsible pointer, perhaps by taking the responsible pointer's address, and passing it to a function that expects the pointer to be irresponsible.

### Structs and Classes
A struct can contain pointers, and these pointers will be responsible, irresponsible, or out-of-scope. Consider this struct:

``` c
struct foo_s {
  char *rptr;  // responsible
  char *iptr;  // irresponsible, mutable, lifetime matches foo_s
};
```

So in every instance of this struct rptr will be responsible and iptr will be irresponsible. The iptr will have a lifetime that is bound to foo_s. That is, the irresponsible pointer's referenced object must outlast not only iptr, but also its containing struct.

During POM verification, any struct with one or more responsible pointers can have its own "responsibility state", based on the states of the responsible pointers. The struct's state is the union of the states of the responsible pointers....if one pointer is GOOD and one pointer is NULL, the struct's state is {GOOD|NULL}.  Likewise during POM verification, any struct with one or more irresponsible pointers can have its own "irresponsibility state", based on the states of the irresponsible pointers. This irresponsibility state includes the pointer's lifetime and mutability.

A struct's pointer responsibilities should be declared when the struct is declared, and may not be modified for different variables of the struct. For example, a linked-list struct such as the following:

This also means that arrays or pointer-to-pointer objects should also be declared in the struct

``` c
struct node_s {
  void** data;           // irresponsible-mutable to irresponsible-mutable, lifetimes bound to node_s,
  struct node_t *next;   // responsible
};
```

means that every variable of type `struct node_s` has an irresponsible `data` pointer and responsible `next` pointer. Thus, you cannot use this struct to store data that will be freed later.  If you want a linked list with responsible `data` pointers, you will need to construct a separate struct type.  For now, this shall be known as the struct constraint.

#### Any function that modifies the responsibility state of a struct must not exit with the struct with a responsibility state that includes both GOOD and ZOMBIE. Likewise, the irresponsibility state may not include both VALID and INVALID.

``` c
typedef struct foo_s {
  char *rptr1;  // responsible
  char *rptr2;  // responsible
} foo_t;

void f1(foo_t* foo) { // foo and *foo are responsible and GOOD
  free( foo->rptr1);
} // Violates POM, Inconsistent: foo->rptr1 is ZOMBIE, but foo->rptr2 still GOOD

void f2(foo_t* foo) { // foo and *foo are responsible and GOOD
  free( foo->rptr1);
  free( foo->rptr2);
} // OK, FOO good, *foo ZOMBIE
```

#### Example: Single-Linked List

Consider a responsible linked list (e.g., that owns its data):

``` c
typedef struct node_s {
  void* data;            // responsible
  struct node_t *next;   // responsible
} node_t;

// *list is responsible GOOD
// *list is responsible GOOD|NULL->ZOMBIE|NULL
// data is responsible GOOD|NULL->ZOMBIE|NULL
// Return value is GOOD, *return is GOOD|NULL.
node_t *add_to_list(node_t *list, void* data) {
  struct node_t* node = malloc( sizeof( struct node_t));
  if (node == NULL) {
    /* handle error */
  }
  node->data = data;
  node->next = list;  /* list now ZOMBIE, but dies at end of fn. next changes from null to list's state */
  return node;
}

// list is responsible GOOD|NULL -> ZOMBIE
// *list is responsible GOOD|NULL -> ZOMBIE
void free_list(node_t *list) {
  struct node_t *tmp; // responsible, ZOMBIE
  while (list != NULL) {
    free( list->data); // list->data now ZOMBIE
    tmp = list->next; // list->next now ZOMBIE, *list is now ZOMBIE, tmp gets old state
    free( list); // list now ZOMBIE, about to be reassigned
    list = tmp; // tmp now ZOMBIE, dies or is reassigned, list gets old state
  }
}
```

In this case, the node_t->data pointers are responsible;  they own their data. However, the case where the data pointers are irresponsible is different, as it must not free any data pointers.

All other pointers are also responsible. Each one points to a node_s object that must be subsequently freed (or it is NULL). Likewise, every node_s is a responsible struct, because it contains responsible data and next pointers.

In add_to_list(), node is responsible by being initialized to a malloc'd chunk of memory. The list pointer is also responsible, as it points to another node created from add_to_list().  The responsibility is transferred from list to node->next in the assignment, because the list pointer will go out of scope shortly. The add_to_list() function returns an object that must be subsequently freed. But it is also a consumer of both arguments, as both arguments must no longer be subsequently freed!

``` c
node_t* list1 = malloc(sizeof(node_t)
/* initialize list1 data */
/* at this point responsible list1 is GOOD, it must be freed */
node_t* list2 = add_to_list( list1, data)
/* at this point responsible list1 is ZOMBIE, its data is part of list2 */
/* but list2 is responsible & GOOD */
```

The free_list() function frees the linked list, and is meant to be a consumer for list. Since freeing list is illegal as long as it has a responsible pointer, the program transfers the responsible pointer to tmp, then it frees list. It transfers ownership back from tmp to list because tmp will next go out of scope or get reassigned (depending on if the loop terminates).

#### Example: Circular Linked List:

Consider this function, which would free a circular singly-linked list:

``` c
typedef struct {
  void* data; // irresponsible, mutable, lifetime tied to node_t
  struct node_t *next;  // responsible
} node_s node_t;

void free_list(node_t *head) {
  struct node_t *tmp, *node = head;
  while (node->next != head) {
    tmp = node->next;
    free( node);
    node = tmp;
  }
  free(node); /* last one */
}
```

This code is problematic because head is freed on the first iteration of the loop, and then compared at the start of each subsequent iteration.  This is easy to fix by freeing the head last:

``` c
void free_list(node_t *head) {
  // Handle singleton case
  if (head == head->next) {
    free(head);
    return;
  }

  // Free head after everything else.
  struct node_t *tmp, *node;
  node = head->next;
  do {
    tmp = node->next;
    free( node);
    node = tmp;
  } while (node->next != head);
  free(head);
}
```

This code still violates POM, as before free_list() is invoked, the head element is owned both by the 'head' argument, and by the final list element.

However, POM will not flag this code because the only clue that this code references a circular linked list is the final while statement that compares node->next with head.

Instead POM would flag the code that created the circular list, and tried to assign the head element to the next struct member pointer.

#### Example: Double-Linked List

Consider a double-linked list:

``` c
struct node_s {
  char* data;
  struct node_s* next; // responsible
  struct node_s* prev; // Must be out-of-scope!
}
```

First of all, double-linked lists can not be represented in safe Rust.This is because the ownership of each node in the list is jointly shared between the node's previous 'next' pointer and the node's next 'prev' pointer.  (The Rust double-linked list uses unsafe code under the hood). 

One way to resolve this is to make the 'next' pointer responsible and the 'prev' pointer irresponsible.  But this will violate irresponsible pointer lifetimes, since the lifetimes of each list node have no minimum guarantees.

But if the prev pointers are out-of-scope, then this setup complies with POM for responsible pointers; it behaves like a single-linked list.

#### Historical Example: Double-Linked List

The following code example came up during our LENS project, and we did not have a good way to resolve it. This is resolved today by the fact that POM disallows double-linked lists, (unless one of the pointers is out-of-scope). Thus, this appears to POM like a single-linked list, and POM ignores the prev pointers.

POM correctly reports some violations, but also ignores many, because out-of-scope pointers alias with responsible pointers. Perhaps in a future POM, we could forbid aliasing with out-of-scope pointers.

``` c 
struct node_s {
  char* data;          // responsible, GOOD|NULL
  struct node_s* next; // responsible, GOOD|NULL
  struct node_s* prev; // Must be out-of-scope!
}

void function(char* data); // diligent (does not consume data)

// Imagine that L is a struct node_s with three elements
// and we are trying to remove the middle element.
struct node_s* tmp = L->next->next; // responsible
free( L->next->data);        // one data now ZOMBIE
free( tmp->prev->data);      // Ignored by POM b/c prev out-of-scope
function( tmp->prev->data);  // Ignored by POM, use-after-free of out-of-scope ptr
free( L->data);              // Safe. POM knows that L->data != L->next->data, since both are responsible
tmp->prev-data = NULL;       // zeroes out freed pointer. Ignored by POM
free( L->next->data);        // Memory-safe b/c NULL, but violates POM, 
                             // b/c POM doesn't realize L->next->data is NULL
L->next->data = 0;           // POM now knows zombie is dead
free( tmp->prev->data);      // OK, free(NULL), noop. Ignored by POM
```

### Union
A union can be considered a struct, with the difference that writing to any member of the union overwrites all members. Any pointers in a union can be responsible, irresponsible, or out-of-scope.  The rules that apply to pointers in a struct also apply to pointers in a union.

#### A union may have at most one responsible pointer.
This is because assigning to one responsible pointer overwrites any other responsible pointers. So making one responsible pointer GOOD would turn other responsible pointers into ZOMBIE, making the union inconsistent.

``` c
union foo_u {
  char *ptr1;  // responsible
  char *ptr2;  // cannot be responsible
};
```

#### Writing any element besides the responsible pointer in a union with a responsible pointer sets the pointer state to ZOMBIE (its pointer value becomes INVALID)

``` c
#define SIZE (siezof(char*))
typedef union foo_u {
  char *rptr;         // responsible
  char other[SIZE];   // any other data
} foo_t;

foo_t foo;
foo.rptr = ...; // GOOD
foo.other[0] = 'a'; // foo.rptr becomes ZOMBIE
```
## Function Annotations

The POM verifier inspects each function independently, although to verify that a function complies with POM, all functions it invokes must also comply with POM.

To analyze a function body properly, we need to inspect the definition. We need to view all the variables available to the function. That is, its arguments, local variables, and static variables. Does the function modify any of the responsible pointers' states?  Does it return a responsible pointer that must be later freed?

The following text will talk about pointers, but can also be applied to composite objects. 

To do this the verifier will need to recognize the following attributes about any function:

### Error indication
Many C functions can indicate errors preventing them from completing normal execution. There are many ways in C to indicate errors, such as returning error values such as NULL, or using the global `errno` variable.

The strategy a function uses to indicate errors can affect the state of responsible pointer arguments  See blow for some examples.

### Noreturn functions
While _Noreturn is a standard function qualifier, it is not always applied where it could be. We may have to annotate some functions as _Noreturn when they are not already annotated.

These are functions that never return to their caller. They always exit via abort(), exit(), _Exit(), longjmp(), goto, or by invoking another noreturn function. The C11 _Noreturn keyword denotes noreturn functions.

Example noreturn functions: abort(), exit()

``` c
void panic(const char *s) { // this is a noreturn function
  printf("Error: %s\n", s);
  abort();
}
```
## Discussion
### Type Casts

There is no problem with typecasting a pointer (responsible or irresponsible or out-of-scope) to a pointer of different type.  However, the pointer retains its responsibility, and if responsible, its state does not change from a simple typecast.

There is no issue with converting an out-of-scope pointer to an integer or some other representation (e.g. char array), or vice versa.

#### For now, POM does not support tracing responsible or irresponsible pointers through integer casts
Or casts to any non-pointer types

``` c
int *rp = ... // responsible
uintptr_t up = (uintptr_t) rp; // Violates POM
int *ip = ... // responsible
up = (uintptr_t) ip; // Violates POM
```
### Control Flow and Responsible Pointer States

A pointer can be in multiple states at once. We always assume that the states a pointer is in can be determined statically. For any two states, it is easy to use branching to create a pointer that could be in both states.

``` c++
char *rptr; // ZOMBIE
bool flag = // ...
if (flag) {
  rptr = new char[...]; // GOOD
}
// rptr now GOOD or ZOMBIE, based on flag
```

This can cause trouble later on. Traditionally there is no way to distinguish good responsible pointers from uninitialized pointers, however in this code the flag indicates whether rptr is GOOD or ZOMBIE. To be safe, this code must use flag to determine if the pointer is good. And determining flag's value statically might be impossible.

For now, we'll table this possibility, and assume that POM may not rely on flag. This means that this code will always yield an error: either when rptr is freed (because it may have been uninitialized) or when rptr leaks (it may have been GOOD). The code can always be brought to compliance with POM by initializing rptr to NULL:

``` c++
char *rptr = NULL; //  NULL
bool flag = // ...
if (flag) {
  rptr = new char[...]; // GOOD
}
// rptr now GOOD or NULL, based on flag */
free(rptr); // was GOOD or NULL, now ZOMBIE or NULL
```

Besides using control flow to create multiple states, malloc() & other functions returning responsible pointers) can make GOOD|NULL pointers. No other combinations of states are typically possible without control flow. But because of control flow, we must assume any combination of states are possible. So a responsible pointer's state should be expressed as a list of booleans (or a bit-vector), one for each possible state.

This does mean that this code snippet violates POM despite being memory-safe:

``` c++
char *rptr; // ZOMBIE
bool flag = // ...
if (flag) {
  rptr = new char[...]; // GOOD
}
// rptr now GOOD or ZOMBIE, based on flag
if (flag) {
  free( rptr);
}
```

### C++ templates of C pointers
Such templates should have 'responsible' or 'irresponsible' as part of their type arguments.

This is based on a recent [ChatGPT discussion](https://chatgpt.com/share/36b75aaa-d52a-480c-8694-f8f62e64fdcc).  The code in question lives [here](src/test/template_lifetimes.cpp).

Our determination is that full_list is a vector<responsible int*>. However, sub_list violates POM.  It can't be a vector<responsible int*> because it receives shallow copies of the ints from full_list. It also cannot be a vector<irresponsible int*> because its values could become stale whenever full_list changes. Adding just one element to full_list could potentially end the lifetimes of the irresponsible pointers in sub_list.

There are two ways to make the code memory-safe:
 * sub_list becomes a vector of responsible int pointers, and uses operator new() to clone the ints when they are added to sub_list.
 * In the 'R' switch case, when you change full_list, you cascade these changes to sub_list, which can then be a vector of irresponsible pointers).  While this is possible and can achieve memory safety, you probably can't could make any lifetime guarantees.  Such a solution would probably not be verifiable under POM.

This code also has the problem that no memory is freed. full_lists's ptrs should be freed and sub_list's ptrs should not be.

## Algorithms
### Expression Pointer Types

In C every expression has a type associated with it, which are derived from its sub-expressions.  In POM every pointer expression also has a responsibility and a state associated with it and these are likewise deried from its sub-expressions.

Irresponsible pointers and diligent pointers will be on one of these states: {INVALID, NULL, VALID} and either mutable or immutable. 
Each local irresponsible pointer also has a set of lifetimes associated with it. The set is empty until the pointer is assigned to point to a responsible object
Responsible pointers will be on one of these states: {ZOMBIE, NULL, GOOD}

To determine the state, responsibility, and lifetime of a pointer expression, first consider what the expression is:
 * An identifier (variable) has the state/responsibility/lifetime of the identifier.
 * 0 aka NULL is the NULL state, no lifetime, and any responsibility.
 * A string literal is irresponsible and immutable, and VALID, with STATIC lifetime
 * A function or function pointer has VALID state and is irresponsible and immutable with STATIC lifetime
 * A component of an array, struct, or union has the state/responsibility/lifetime of its component type(s).
 a[5]. (note that not all struct fields have identical responsibility or state)
 * A function call has the state/responsibility/lifetime of the function's return value.
 * As a special case, a call to memcpy() or memmove(), in addition to other function calls provides the same semantics as assignment (from the src argument to the dest argument)
 * Pointer arithmetic has the state, mutability, and lifetime of the pointer expression inside the arithmetic, and is irresponsible.
 * An address-of operator (&) to a responsible pointer that is used as a parameter to a function that expects a producer argument becomes the producer argument.
 * An address-of operator (&) in any other circumstance is irresponsible and VALID with the same mutability and lifetime as the value it references
 * A dereference of an irresponsible pointer to a responsible pointer has 'diligent' responsibility and the same state, mutability, and lifetime specified in POM of the pointed-to value.
 * A dereference of any other pointer-to-pointer has the state/responsibility/lifetime specified in POM of the pointed-to value.
 * A typecast from one pointer to another has the same state/responsibility/lifetime of the typecast pointer
 * A typecast from a non-pointer type (typically an integer) to a pointer is out-of-scope.
 * An assignment operation from a pointer takes the state/responsibility/lifetime of the RHS pointer in the assignment. (But note that a responsible pointer when assigned to another responsible pointer becomes a ZOMBIE.)

### Null-Check Expressions

A "null-check expression" is any of the following:
 * \<ptr-expression\> == NULL or NULL == <ptr-expression\>
 * \<ptr-expression\> != NULL or NULL != <ptr-expression\>
 * ptr (when evaluated in a boolean context)
 * !\<null-check-expression\>
 * \<null-check-expression\> && \<expr\>

These expressions can be used to partition a pointer's state between NULL and non-NULL.

### Pointer State Map

This algorithm must make use of a pointer state map. This is a mapping of pointer variables currently in scope. Each pointer is associated both with a responsibility (responsible, irresponsible, or out-of-scope) and a subset of permissible states. Irresponsible pointers also have a mutability state (mutable or immutable) and a set of lifetimes.

Whenever a pointer is used, its responsibility, mutability, state, and lifetime must be confirmed on the map, and the system should report a POM violation if they are incompatible. For example, if an irresponsible pointer gets assigned the result of malloc(), that merits a POM violation.  This implies that there should be a type database for complex types. This will include structs and unions, which are declared statically, and any arrays or pointer-to-pointer towers declared for any variables in scope.

Whenever a pointer's state or lifetime changes, the map must be updated for that pointer.

Note that control-flow statements may have to maintain multiple versions of this map. For example, an if statement should present the same map to its then-clause and to its else-clause (that is, any changes to the map from the then-clause do not apply to the else-clause. After the if statement is concluded, the two maps must be unified.)  Unification of two maps into one comes when control flow merges together, usually when a control flow statement ends.  Pointers that exist in one map but not the other are pruned from the resulting map.  Pointers that exist in both maps but have different state subsets get their subsets unioned in the resulting map.  When unifying two maps together, an irresponsible pointer that has two distinct lifetime sets receives a union of these sets. This indicates that multiple lifetimes impose their constraints on the irresponsible pointer. This is the only way an irresponsible pointer can have two or more lifetimes.

This map not only captures all pointers in scope, it should capture pointers in composite objects that are in scope. For now, structs & unions must declare ownership at their own declarations. But arrays and pointer-to-pointer objects should be added to the map for static variables, function return values, function variables, and local variables.  It does this by noting the C-paths of composite objects. (It doesn't have to capture every C-path, just the ones that are read or written by the current function.).

In addition to the map, there should be a 'current C-path' variable, a stack that keeps track of the path currently referenced by the current expression. For example, an expression:

``` c
foo[0]->bar->baz
```

refers to a C-path of three elements `foo`, `bar`, and `baz`. As the analysis traverses through this expression, the current c-path should push each element to itself, to help in knowing which pointer in the map is being read or written.

### Death Map

This is a map of all lifetimes that have officially ended, because a pointer was passed to a function that consumed it (such as free()).  It starts off empty for each function.  It is used to detect use-after-free vulnerabilities.

### Verifier

The following algorithm shall be used by the verifier.e  The verifier takes source code (which can be represented through the source code file, the AST, or the LLVM-IR.). It also takes a POM, which could be read in through JSON or YAML.

    Obtain list of all functions in source code, and list of all functions in POM
    Report POM violations if there are any differences in the lists.
    For each function (in both the source code and POM):
      If the function returns a pointer in the source, then POM should indicate this
        Report POM violation if POM and the source disagree.
      Compare the list of function parameter pointers with the corresponding list in POM
        Report POM violation if there are any differences in the lists.
      (Non-pointer parameters can be ignored)
      Compare the list of local pointer variables in the function's definition with the corresponding list in POM
      Note that local pointer variables may have an associated line number to resolve shadowing.
      (That is, ptr:102 might shadow ptr:100, and the line numbers resolve the shadowing.)
        Report POM violation if there are any differences in the lists.
      (local non-pointer variables can be ignored)

      Initialize the pointer state map linking variables to pointer responsibility and permissible states.
      Now traverse the function body, noting pointer responsibilities & states for each expression.

#### To initialize the pointer state map:

      For each function parameter that is a pointer:
        Add the pointer's responsibility, states, mutability, and lifetime from POM.
        Unless otherwise specified, an irresponsible pointer's lifetime lasts for the entire function.
        While it is a pointer-to-pointer:
          Dereference the outermost pointer:
          Add the resulting pointer's responsibility, states, and lifetime from POM.

(Note that this only handles pointer-to-complex type; structs and unions are handled outside function annotations)

#### To traverse the function body, with the pointer state map:

    Some validations to do with expressions: Report POM violation if the expression is:
    * A function call where any of the pointer arguments' state is incompatible with what the function requires, according to POM
    * An assignment of any out-of-scope pointer to a responsible or irresponsible pointer
    * An assignment of any irresponsible pointer to a responsible pointer
    * An assignment of any function call returning a responsible pointer to an irresponsible pointer
    * An assignment of any irresponsible pointer to a pointer whose lifetime can exceed that of the function, but the pointer's lifetime is not known to exceed the second pointer.
    * An assignment of an irresponsible pointer to a mutable irresponsible pointer.
    * Any dereference of a pointer that might be INVALID, ZOMBIE, or NULL
    * Any dereference of an irresponsible pointer whose lifetime is on the death map
    
    Some expressions involve state changes: When a pointer's state changes, the map must be updated. If the expression is:
    * A function call where any argument takes a pointer, the function may change the pointer's state.
    * A function call where any argument takes the address of a pointer, the function may change the pointer's state    Any pointer that is consumed by the function (that is, it ends as a ZOMBIE but didn't start as a ZOMBIE) should have its lifetime added to the death map.
    * A function call where a responsible pointer is passed to the function, and the function requires a responsible pointer, in anticipation of consuming it. The responsible pointer's lifetime goes onto the death map. Use of any irresponsible pointers with the same lifetime constitute use-after-free. This includes any irresponsible pointers passed as arguments to the function, since the function could unknowingly create a use-after-free error in its own body.
    * An assignment operation from one responsible pointer to another converts the original responsible pointer to a ZOMBIE (even though it has the same pointer value).
    * A null-check expression in the body of a conditional operator (?:) partitions the pointer's state in the operator's then and else sub-expressions.
    * The address-of operator (&) may only be taken for a pointer that is GOOD or VALID.

    Some statements also involve state changes: If the statement is:
    * An if, while, do/while, or for statement with a null-check expression, partitions the pointer's state in the corresponding blocks.
    * A declaration of a pointer. The pointer's responsibility comes from POM.
      If uninitialized, its state is INVALID or ZOMBIE, otherwise its state comes from the initializer.
    * A return statement whose pointer responsibility or state are incompatible with what POM mandates.

## Future Work
### Out-of-scope pointers

For this project we will assume OOS pointers cause no problems, which is clearly unsafe. For a future project, we can investigate how OOS pointers interact with responsible & irresponsible pointers. This would include things like doubly-linked lists, reference-counted pointers, and similar beasts.

### alloca

We could handle extending the model to support alloca(). The easiest way is to require any pointers living in memory allocated by alloca() to be irresponsible. This is because if we allow responsible pointers there, then our definition of a responsible C-path breaks.

### C++

Supporting C++ should be a matter of engineering, but we will handle this in a future project.

### POM-Noncompliant Standard Functions
#### strcpy()

Consider strcpy(), which has the following POM:

    strcpy:
      args:
        s1:                   # dest
          resp: diligent
        s2:                   # source
          resp: diligent
      return:
        resp: irresponsible   # same as s1
        start: [VALID]
        lifetime: s1

The following code examples are both memory-safe, but only one complies with POM. They only differ in the final line.

``` c
char* s1 = malloc(10); // responsible
assert(s1);
char* s0 = strcpy( s1, "hello"); // s1 == s0, s0 irresponsible
free(s1); // OK
```

``` c
char* s1 = malloc(10); // responsible
assert(s1);
char* s0 = strcpy( s1, "hello"); // s1 == s0, s0 responsible (!), s1 ZOMBIE
free(s0); // OK
```

Ideally, we could extend POM to notice when a function returns a pointer that should match responsibility with an argumenbt.

#### realpath()

Now consider the POSIX realpath function, which has this model:

    realpath:
      args:
        path:
          resp: diligent
        resolved_path:
          resp: diligent
          start: [NUL|VALID]
      return:
        resp: responsible  # ?
        state: [NUL|VALID]

As before, the following code examples are both memory-safe, but only one complies with POM.

``` c
char* s1 = malloc(PATH_MAX); // responsible
assert(s1);
char* s0 = realpath("/usr/bin", s1); // irresponsible
free(s1);  // OK
```

``` c
char* s1 = malloc(PATH_MAX); // responsible
assert(s1);
char* s0 = realpath( "/usr/bin", s1); // responsible, s1 ZOMBIE if s0 GOOD
if (s0) // s0 GOOD
  free(s0);  // OK
else // s0 NULL, s1 still VALID
  free(s1); // OK
```

The last code example demonstrates the strategy of the ResolveSymbolicLink() function in dos2unix.

The problem is that realpath() behaves differently depending on whether or not its latter argument is NULL: If it is NULL, realpath() allocates an a string to hold its final path and returns it; as a responsible pointer. If it is not NULL, it is taken as the string to hold the pointer, and realpath() returns it.  Also, realpath() can return NULL if an error occurs.  Both arguments are diligent, but clearly, if you pass an irresponsible pointer to realpath() as its latter argument, it will return it, and you must not free it.  Therefore, realpath() violates POM.

### C function pointers that take or return pointer arguments

Their responsibility must be modeled

# End
<!--  LocalWords:  verifier ptr fopen const structs malloc strlen tmp
<!--  LocalWords:  ifdef'd realloc alloc fclose getline longjmp goto
<!--  LocalWords:  strcpy strdup struct's malloc'd POM's ChatGPT
                   getline strcpy ptrs realloc calloc strdup
 -->
