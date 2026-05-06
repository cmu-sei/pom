# Main constraints

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

## Intro
Our Pointer-Ownership Model (POM) for C is a Rust-inspired ownership/borrowing discipline enforced at the LLVM IR level. It tracks who owns each heap block, which pointers may be used for writing to memory, and which pointers have outstanding borrows at each program point.

This document describes generation of constraints other than those for borrowing/lifetimes.
See `borrow_design.txt` for borrowing/lifetimes constraints.


## Type of atomic propositions used in constraints
Type-qualifier properties (doesn't depend on program point):
- "responsible(p)": indicates that p is a responsible pointer.
- "irresponsible(p): indicates that p is an irresponsible pointer.
- "mut(p): indicates that p can be used to write to memory (as opposed to only read from memory).
- "outscope(p)": indicates p is an out-of-scope pointer.

State properties (depends on program point) for pointers:
- "good(p, t)": indicates that p MAY be a good pointer at program point t.
- "null(p, t)": indicates that p MAY be a null pointer at program point t.
- "zombie(p, t)": indicates that p MAY be a zombie at program point t.  A pointer variable is a zombie if it is uninitialized or it is freed or its value has been assigned to another responsible pointer variable.

A *program point* is a point immediately before or immediately after a given statement is executed.  Given a statement *S*, we write *S-pre* to denote that program point before *S* and we write *S-post* to denote the program point after *S*.

In the absence of structs, unions, and arrays, a *C-path* is a local or global variable with zero or more dereferences.  E.g., if `v` is a variable, then `v`, `*v`, and `**v` are all C-paths.

Note: Properties like `good(p, t)` are written using predicate notation, but they are treated as atomic propositions (boolean variables) for SAT solving.

Note: Initially, we started with C-paths like `**p` (and this is still used in this file) but we switched to `p[0][0]` (which is used in `borrow_design.txt`).  Some parts of the implementation still use the old notation, but there are helper functions that interconvert where necessary.


## Desired properties:

 - A zombie pointer is never used for reading from memory, writing to memory, or freeing memory.
 - ...


## Definitions

Local identifier / virtual register / SSA variable / LLVM temporary: An identifier that appears on the left-hand side of an IR instruction `var = opcode ...`.

Memory location: A location in memory.  A single memory location may be named by multiple C-paths.

Location: A memory location, a virtual register, `func_name::return`, or `func_name::args::arg_name`.


## Invariants

Our constraints are designed to establish the following invariants:
 - At every program point, for every live memory block M allocated via malloc/calloc/realloc, there is exactly one location that holds a good responsible pointer that points to M.  (Here, the term "location" encompasses both allocated memory locations and LLVM virtual registers.)
 - At every program point, for every memory location M, there is at most one C-path that can be used to write to M.  Such a C-path must be marked `mut` and must have no outstanding borrows.
 - The borrow info is kept accurate and complete for all live C-paths.  Specifically:
   - For every live C-path P, we record all live borrows from P.
   - If p2 and p3 both immutably borrow from p1, and subsequently p1 dies, then we DO NOT remember that p2 and p3 are pointer aliases, because remembering this is not necessary for soundness.
 - For pred in {good, null, zombie}: for every live C-path `p`, if the state indicated by pred (i.e., being a good ptr, being a null ptr, being a zombie ptr) actually holds true of `p` on any execution trace at program point `t`, then `pred(p, t)` is constrained to be true.  (Note: we do not have any constraints specifically for preventing the state properties from spuriously becoming true, since this isn't necessary for soundness.)
 - ...

## Constraint Generation

We generate constraints for (1) specifying what happens during execution of a statement (an LLVM instruction) and (2) connecting the end of one statement to the beginning of the next statement.

### Control Flow

Consider two LLVM IR instructions P and S, where P is a predecessor of S.  (By "P is a predecessor of S", I mean the following:  If P and S are in the same basic block, then P is the instruction immediately preceding S in the basic block.  If P and S are in different basic blocks, then P must be a `br` instruction at the end of a basic block and S must be the first instruction of a basic block that is a jump target of P.)

On any given path, the time point `P-post` is essentially the same time point as `S-pre`.  However, if S has multiple predecessors, then it is useful to have separate names, so that we can generate constraints for P and S separately and then tie them together with control-flow constraints.

We generate the following constraints for every C-path `ptr` that is live at P-post:
- `good(ptr, P-post) -> good(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being null.
- `null(ptr, P-post) -> null(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being non-null.
- `zombie(ptr, P-post) -> zombie(ptr, S-pre)` except if P is a conditional jump to S that is conditioned on ptr being null.

Exception: if ptr (or an alias of it) is the first argument to realloc at instruction S-realloc, then on a branch guarded by realloc returning NULL, the GOOD and ZOMBIE properties of ptr are not preserved from P-post to S-pre, and instead:
- `good(ptr, S-realloc-pre) -> good(ptr, S-pre)`
- `zombie(ptr, S-realloc-pre) -> zombie(ptr, S-pre)`
On the branch guarded by realloc not returning NULL:
- `good(ptr, S-realloc-pre) -> zombie(ptr, S-pre)`
- `zombie(ptr, S-realloc-pre) -> zombie(ptr, S-pre)`

If the first argument to realloc would naturally die inside the basic block of the realloc call, then it is artificially kept half-alive until the beginning of the successor basic blocks.  Specifically, it is kept alive for purposes of generating preservation constraints and warning about the death of a good responsible pointer, but it is not kept alive for the purpose of borrowing constraints.

### Preservation Constraints

State properties of most C-paths are preserved during execution of an instruction.  We identify which C-paths need to be excluded from being preserved:
1. A C-path of a memory location whose contents change during the instruction.
2. A C-path that points to a different memory location after the instruction than before the instruction.
3. A C-path of a pointer whose value doesn't change but becomes a zombie because it is free or copied to another responsible pointer.

Let us define `IsExcluded` as in the following pseudocode:
```
function IsExcluded(string cpath, set<string> changedCPaths, set<string> maybeZombifiedCPaths) {
    // changedCPaths is the set of cpaths whose contents change during the instruction.
    if ((cpath in changedCPaths) or (cpath in maybeZombifiedCPaths)) {
        return true;
    }
    else if (cpath has the form "*parentPath") {
        // If an ancestor path points to a mem loc whose contents changed, then
        // this cpath points to a different mem loc after the instruction than
        // before the instruction.
        return IsExcluded(parentPath, changedCPaths, {});
    }
    else {
        return false;
    }
}
```

For example:
- `IsExcluded("x", {"x"}, {}) == true`   // excluded because it appears in changedPaths.
- `IsExcluded("x", {}, {"x"}) == true`   // excluded because it appears in maybeZombifiedCPaths.
- `IsExcluded("*x", {"x"}, {}) == true`  // excluded because parent path appears in changedCPaths.
- `IsExcluded("*x", {}, {"x"}) == false` // parent path appearing in maybeZombifiedCPaths doesn't cause exclusion.
- `IsExcluded("x", {"*x"}, {}) == false` // child path appearing in changedCPaths doesn't cause exclusion.


For a variable (or more accurately, an LLVM `Value*`), we use the term *live*
in the sense of `https://en.wikipedia.org/wiki/Live-variable_analysis`.
We say that a C-path is *live* if it is rooted at a live variable.

Let `PreservationConstraints(changed, maybeZomb)` consist of the following constraints
for all live C-paths `x` where `!IsExcluded(x, changed, maybeZomb)`:
- `good(x, S-post)  <->  good(x, S-pre)`
- `null(x, S-post)  <->  null(x, S-pre)`
- `zombie(x, S-post) <-> zombie(x, S-pre)`


### Pointer-Copying Constraints

We say that two pointers are *pointer aliases* of each other iff they point to the same memory location.
We say that two l-values are *location aliases* of each other if their addresses are pointer aliases of each other.
E.g., given pointers p1 and p2, if p1 == p2, then p1 is a pointer alias of p2 and `*p1` is a location alias of `*p2`.

We define `location_aliases_of(e)` as follows:
 - If `e` is an LLVM virtual register, then `location_aliases_of(e) = {e}`.
 - If `e` is a pointer dereference `*p`, then `location_aliases_of(e)` is the set of location aliases of `*p`, including `*p` itself.  (This is a MAY analysis, not a MUST analysis.)
We define `location_must_aliases_of(e)` similarly, except using a MUST analysis instead of a MAY analysis.

Let `PtrCopyConstraints(dest, src, S-pre, S-post, S-post-dest)` consist of the following constraints:

- `responsible(dest) -> responsible(src)` # Cannot assign a non-resp ptr value to a resp ptr dest.
- `mut(dest) -> mut(src)` # Cannot assign a non-mut ptr value to a mut ptr dest.
- `responsible(dest) | !zombie(src, S-pre)` # POM constraint: cannot assign zombie value to irresp ptr (this constraint is not necessary for soundness, but it can simplify UNSAT cores that we get from the SAT solver)

- For each `src_ali` in `location_aliases_of(src)`:
  - `responsible(dest) -> (good(src_ali, S-pre) -> zombie(src_ali, S-post))` # Move semantics: copying a resp ptr from src to dest makes src a zombie.
  - if `src_ali` is a must alias:
    - `!responsible(dest) -> (good(src_ali, S-pre) -> good(src_ali, S-post))`
  - if `src_ali` isn't a must alias:
    - `good(src_ali, S-pre) -> good(src_ali, S-post)`
  - `null(src_ali, S-pre) -> null(src_ali, S-post)`
  - `zombie(src_ali, S-pre) -> zombie(src_ali, S-post)`

- For each `dest_ali` in `location_aliases_of(dest)`:
  - `good(src, S-pre) -> good(dest_ali, S-post-dest)`
  - `null(src, S-pre) -> null(dest_ali, S-post-dest)`
  - `zombie(src, S-pre) -> zombie(dest_ali, S-post-dest)`

- DeepPtrCopyConstraints(dest, src, S-pre, S-post-dest)

If S-post-dest is omitted, it defaults to S-post.

Let `DeepPtrCopyConstraints(dest, src, S-pre, S-post-dest)` consist of the following constraints:
- `responsible(*src) <-> responsible(*dest)` and ditto for deeper levels of indirection
- `mut(*dest) -> mut(*src)` and ditto for deeper levels of indirection
- For each `dest_ali` in `location_aliases_of(dest)`:
  - `good(*src, S-pre) -> good(*dest_ali, S-post-dest)` and ditto for `null` and `zombie`, and ditto for deeper levels of indirection

Note: Since LLVM has switched to opaque pointers, it is nontrivial to determine the pointer depth of pointers.  We can get this info fairly easily from the debug info for function arguments and return values.  For other LLVM Value objects, we piggyback off seenCPaths to infer the pointer depth.  (TODO: It seems we can also get type info for locals from the debug info.  This will speed up convergence.)


### Statement: `p = alloca ptr ...`

Constraints: 
- `!responsible(p)`
- `good(p, S-post)`
- `responsible(*p) -> zombie(*p, S-post)` if `*p` is a pointer type, and ditto for deeper derefs
- PreservationConstraints({p}, {})

### Global variables

The address of a global variable is not responsible:
- `!responsible(addr_global_var)`

### Statement: `p = call ... @malloc(...)` or `p = call ... @calloc(...)`

Constraints: 
- `responsible(p)`
- `good(p, S-post)`
- `null(p, S-post)`
- `!zombie(p, S-post)`
- `responsible(*p) -> zombie(*p, S-post)` if `*p` is a pointer type and the callee isn't calloc, and ditto for deeper derefs
- PreservationConstraints({p}, {})


### Statement: `call ... @free(p)`

Constraints: 
- `responsible(p)`
- `!zombie(p, S-pre)`
- `good(p, S-pre) -> zombie(p, S-post)`
- `null(p, S-pre) -> null(p, S-post)`
- `!responsible(*p) | !good(*p, S-pre)` (if the type of p is a pointer to a pointer) # prevent memory leak
- PreservationConstraints({p}, {})

Note: p is always an SSA virtual register.  So, there are no location aliases of p.  A statement `free(*ptr)` at the C source code level gets translated into two instructions at the LLVM IR level:
```
    temp = *ptr
    free(temp)
```
The first of these instructions (`temp = *ptr`) moves the value out from `*ptr`, making `*ptr` (along with all its location aliases) a zombie.

If `isa<UndefValue>(p)`, then we generate the constraint:
- `false # An uninitialized pointer must not be passed to free`


### Statement: `t = call ... @userfunc(act_arg_1, ..., act_arg_n)`

Constraints:
- `PtrCopyConstraints(
    dest = userfunc::formal_param_i,
    src = act_arg_i,
    S-pre = S-pre,
    S-post = S-post,
    S-post-dest = "start")` for i in [1, ..., n] if arg[i] is of pointer type
- `!zombie(act_arg, stmtPre)` for args of pointer type  # Don't pass zombies to functions.
- `good(*userfunc::formal_param_i, end) -> good(*act_arg_i, S-post)` for i in [1, ..., n] if arg[i] is of pointer type, and ditto for `null` and `zombie`, and ditto for further deref levels, and ditto for aliases. # Function summary indicates how pointed-to object change during the function.
- If the return value is of pointer type:
    - `responsible(userfunc::return) <-> responsible(t)`
    - If the return value has dependent mutability, then:
        - `mut(t) -> mut(rhs_dep_arg_actual)`
      else:
        - `mut(t) -> mut(userfunc::return)` # Cannot assign a non-mut ptr value to a mut ptr dest.
    - `good(userfunc::return, end) -> good(t, S-post)` and ditto for `null` and `zombie`
    - `DeepPtrCopyConstraints(dest=t, src=userfunc::return, S-pre="end", S-post=S-post)`
- PreservationConstraints({`t`, `act_arg_1`, `act_arg_2`, ..., `act_arg_n`}, {})


### Statement: `%t = load typ, ptr %p1`, where `typ` is a non-pointer type

Constraints: 
- `!null(p1, S-pre)`  # Cannot read via a NULL ptr.
- `!zombie(p1, S-pre)`  # Cannot read via a zombie ptr.
- `PreservationConstraints({}, {})`

### Statement: `%t = load ptr, ptr %p1`

Constraints: 
- `!null(p1, S-pre)`  # Cannot read via a NULL ptr.
- `!zombie(p1, S-pre)`  # Cannot read via a zombie ptr.
- `!mut(p1) -> !mut(t)`  # Cannot get mutable access via an immutable ptr.
- `PtrCopyConstraints(dest=t, src=*p1, S-pre, S-post)`
- Let `maybeZombified` be `location_aliases_of(*p1)`
- `PreservationConstraints({t}, maybeZombified)`


### Statement: `store typ t, ptr %p1`, where `typ` is a non-pointer type

Constraints: 
- `mut(p1)`
- `!null(p1, S-pre)`
- `!zombie(p1, S-pre)`
- PreservationConstraints({}, {})

### Statement: `store ptr t, ptr %p1`

Constraints: 
- `mut(p1)`
- `!null(p1, S-pre)`
- `!zombie(p1, S-pre)`
- `responsible(*p1) -> !good(*p1, S-pre)` # to prevent memory leaks
- `PtrCopyConstraints(dest=*p1, src=t, S-pre, S-post)`
- Let `changedCPaths` be `location_must_aliases_of(*p1)`
  (Note: The purpose of `changedCPaths` is to prevent preservation of existing state properties.  A location alias that is only a MAY alias, not a MUST alias, will effectively have its existing state properties OR'd together the state properties of what is being written.)
- `PreservationConstraints(changedCPaths, {t})`


### Statement: `%var = __pom_var_store(%src)`

Constraints:
- `PtrCopyConstraints(dest=var, src=src, S-pre, S-post)` if var is of pointer type
- `PreservationConstraints({var}, {src})`


### Statement: `%t = select i1 %cond, typ %val1, typ %val2`, where `typ` is a non-pointer type

Constraints: 
- `PreservationConstraints({t}, {})`

### Statement: `%t = select i1 %cond, ptr %val1, ptr %val2`

Constraints:
- `PtrCopyConstraints(dest=t, src=val1, S-pre, S-post)`
- `PtrCopyConstraints(dest=t, src=val2, S-pre, S-post)`
- PreservationConstraints({`t`}, {`val1`, `val2`})


### Statement: `%t = phi ptr [ %x_1, %L_1 ], ..., [ %x_n, %L_n ]`

Constraints:
- `PtrCopyConstraints(dest=t, src=x_i, S-pre=S-pre, S-post=S-post)` if t is of pointer type
- PreservationConstraints({`t`}, {`x_1`, ..., `x_n`})

### Statement: `return %t`

Constraints (if t is of pointer type):
- `responsible(t) <-> responsible(containing_func::return)` and ditto for deeper indirection
- `good(t, S-pre) -> good(containing_func::return, end)` and ditto for `null` and `zombie` and deeper indirection.
- `mut(containing_func::return) -> mut(t)` and ditto for deeper indirection

### Statement: `%t = getelementptr ...` where it calculates address of element of array

Constraints:
- `!responsible(t)`
- `mut(t) -> mut(base_ptr)`
- `PtrCopyConstraints(dest=t, src=base_ptr, S-pre=S-pre, S-post=S-post)`
- PreservationConstraints({t}, {})

### Statement: `%t = getelementptr ...` where it calculates address of field of struct

Constraints:
- `!responsible(t)`
- `mut(t) -> mut(base_ptr)`
- `responsible(*t) <-> responsible(pomFieldName)` where `pomFieldName` is `struct::$structName::$fieldName`
- `mut(*t) <-> mut(pomFieldName)` where `pomFieldName` is `struct::$structName::$fieldName`
- `PtrCopyConstraints(dest=t, src=base_ptr.fieldname, S-pre=S-pre, S-post=S-post)` // See `borrow_design.txt` for weirdness regarding the ".fieldname" notation that we are using.
- PreservationConstraints({t}, {})


### All other statements (e.g., br, cmp, etc.):

Constraints:
- PreservationConstraints({}, {})


### Dead variables

When a variable x is live immediately before an instruction S but dead immediately after the instruction S (e.g., the instruction S has the last use of x), then generate the following constraint to prohibit memory leaks:
- `responsible(x) -> !good(x, S-pre)`
If the instruction S has multiple predecessors, it is a little more complicated: If a variable x is live immediately after any predecessor of S but dead immediately after S, then we generate the constraint:
- `responsible(x) -> !good(x, S-pre)`

If x is the result of an `alloca` instruction, then we have the following additional constraint:
- `responsible(*x) -> !good(*x, S-pre)`

At the end of a function, we check that responsible arguments do not hold GOOD values:
- `responsible(arg) -> !good(arg, end)`


### All responsible pointers are mutable

For every seen cpath `x`, we generate a constraint `responsible(x) -> mut(x)`.


### Trying to catch casts that increase pointer depth

At the entry point of a function, for each argument, let `starPfx_argName` be `argName` derereferenced to a depth one more than the declared pointer depth.  (E.g., if `p` is declared `int**`, then `starPfx_argName` for p would be `***p`.)
- `zombie(starPfx_argName, start)` # Try to flag exceeding declared pointer depth


### Enforcing constraints in ".pom.yml" file

For each argument of a function, at the entry point of the function:
- `responsible(internalArgName) <-> responsible(func_name::args::arg_name)` and similarly for mut and for {good, null, zombie} at 'start'

Let varNam be the name of an LLVM SSA variable and let pomName be `func_name::locals::var_in_source_code`.
Constraints:
- `responsible(varName) <-> responsible(pomName)`
- `mut(varName) <-> mut(pomName)`



### Note regarding  `__pom_var_store`

In the LLVM IR, you will see `__pom_var_store`.  This comes a LLVM pass `VarStorePass` that we run (before `mem2reg`) to better preserve the correspondence between the C-source-code variables and the SSA variables in LLVM IR.

Our `VarStorePass` LLVM pass changes LLVM code such as
  ```
  store ptr %val_to_store, ptr %dest_local_var
  ```
to something like:
  ```
  %temp = call ptr @__pom_var_store(ptr %val_to_store), !pom_orig_var !39
  store ptr %temp, ptr %dest_local_var
  ```
where `!39` comes from debug metadata for the variable `dest_local_var`.

This change prevents the `mem2reg` pass from changing the code in ways that would present difficulties for the POM verifier.
