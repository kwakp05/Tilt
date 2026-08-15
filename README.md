# Tilt

A simple (and incomplete) interpreter for the Lean programming language, written in C++.

# A Quick Showcase - Proof of the Identity Law

The Identity Law for logical AND states that for any proposition `p`,

`p ∧ True ⇔ p`

We will write a formal proof, verified by Tilt. Note that this theorem is named [`and_true`](https://leanprover-community.github.io/mathlib4_docs/Init/SimpLemmas.html#and_true) in Lean's built in library.

## Foundations

The statement of the Identity Law above makes use of the following math concepts:

* True
* Logical AND
* "If and only if" (⇔)

The way that these concepts are defined isn't important for this demo, but if you're curious, you can see their definitions in [`samples/00.lean`](samples/00.lean).

## Step 1 - Defining the type of the Identity Law

To start with, we can define a constant named `and_true` whose type is the Identity Law. Under Lean's propositions-as-types paradigm, proving the Identity Law simply amounts to assigning a value to `and_true` such that Tilt type checks successfully.

For now, we use `Prop` as a dummy value:

```
def and_true : (p : Prop) -> Iff p (And p True) := Prop
```

If we point Tilt at this code ([`samples/01.lean`](samples/01.lean)), we predictably get a type error because we haven't supplied a valid proof:

```
PS C:\Tilt> ./Tilt.exe ./samples/01.lean
Type mismatch
  Prop
has type
  Type
but is expected to have type
  (p : Prop) -> Iff p (And p True)
```

## Step 2 - Using the Type System to Guide the Proof

Now we can create a function abstraction and use `Iff.intro` to kick off the proof:

```
def and_true : (p : Prop) -> Iff p (And p True) :=
  fun (p : Prop) => Iff.intro p (And p True)
```

Although we haven't finished the proof yet, we can see something interesting if we run Tilt against our incomplete proof. The type error emitted by Tilt basically outlines the rest of the proof for us ([`samples/02.lean`](samples/02.lean)):

```
PS C:\Tilt> ./Tilt.exe ./samples/02.lean
Type mismatch
  fun p : Prop => Iff.intro p (And p True)
has type
  (p : Prop) -> (mp : (_ : p) -> And p True) -> (mpr : (_ : And p True) -> p) -> Iff p (And p True)
but is expected to have type
  (p : Prop) -> Iff p (And p True)
```

The `mp` and `mpr` in the error message are exactly the bits that we need to provide in order to complete the proof. These correspond with each direction of the "if and only if" implication.

## Step 3 - Finishing the proof

We prove each direction of the `Iff` via function abstractions and use the `#check` command to verify that `and_true` proves the Identity Law:

```
def and_true : (p : Prop) -> Iff p (And p True) :=
  fun (p : Prop) => Iff.intro p (And p True)
    (fun (hp : p) => And.intro p True hp True.intro)
    (fun (hand : And p True) => And.left p True hand)

#check and_true
```

Output ([`samples/03.lean`](samples/03.lean)):

```
PS C:\Tilt> ./Tilt.exe ./samples/03.lean
info: and_true : (p : Prop) -> Iff p (And p True)
```
