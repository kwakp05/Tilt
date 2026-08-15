inductive True : Prop where
  | intro : True

inductive And (a : Prop) (b : Prop) : Prop where
  | intro : (ha : a) -> (hb : b) -> And a b

def And.left : (a : Prop) -> (b : Prop) -> (hand : And a b) -> a :=
  fun (a : Prop) =>
  fun (b : Prop) =>
  fun (hand : And a b) =>
  And.rec a b (fun (_ : And a b) => a) (fun (ha : a) => fun (hb : b) => ha) hand

inductive Iff (a : Prop) (b : Prop) : Prop where
  | intro : (mp : (_ : a) -> b) -> (mpr : (_ : b) -> a) -> Iff a b

def and_true : (p : Prop) -> Iff p (And p True) := Prop
