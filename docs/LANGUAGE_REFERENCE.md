# CVM++ Language Syntax Reference

This document outlines the grammar and syntax rules for writing code in CVM++.

## 📖 Table of Contents
- [1. Variables & Types](#1-variables--types)
- [2. Console I/O & Built-ins](#2-console-io--built-ins)
- [3. Arrays & Strings](#3-arrays--strings)
- [4. Operators & Math](#4-operators--math)
- [5. Control Flow](#5-control-flow)
- [6. Loops](#6-loops)
- [7. Functions & Scoping](#7-functions--scoping)

---

## 1. Variables & Types
CVM++ is dynamically typed. Variables can hold integers, floats, characters, booleans, strings, or arrays. Use `assume` to declare and `:=` to assign.

```cvm
assume score := 100;         // Integer
assume pi := 3.14;           // Float
assume letter := 'C';        // Character
assume is_active := true;    // Boolean
assume name := "Ashutosh";   // String
```

## 2. Console I/O & Built-ins
* **`show(val)`**: Prints to the console. It behaves like C++ `cout` and does *not* auto-append a newline. Use `\n` to break lines.
* **`ask()`**: Pauses execution and waits for the user to type a string.
* **Type Casters**: 
  * `to_int(val)`: Converts strings/chars/floats to integers.
  * `to_char(val)`: Converts integers to ASCII characters.
  * `to_str(val)`: Converts any type to a printable string.
  * `to_bool(val)`: Evaluates truthiness (0 and "" are false).

```cvm
show("Enter your age: ");
assume age := to_int(ask());
show("You are " + to_str(age) + " years old.\n");
```

## 3. Arrays & Strings
Arrays and strings natively support bracket indexing (`[ ]`).

**Arrays:**
```cvm
assume my_arr := array(5, 0); // Creates array of size 5 filled with 0s
my_arr[0] := 42;              // Assignment
assume size := length(my_arr); // Returns 5
```

**Strings:**
```cvm
assume msg := "Hello";
assume first := msg[0];       // Gets 'H'
assume msg_len := length(msg); // Returns 5
```

## 4. Operators & Math
* **Arithmetic**: `+`, `-`, `*`, `/`, `%` (Modulo)
* **Cross-Type Math**: `'a' + 1` correctly evaluates to `'b'`.
* **String Concatenation**: `"abc" + "def"` yields `"abcdef"`.
* **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
* **Logical**: `and`, `or`, `!`

## 5. Control Flow
CVM++ uses `when`, `elsewhen`, and `otherwise` for conditional branching.

```cvm
when (score >= 90) {
    show("Grade: A\n");
} elsewhen (score >= 80) {
    show("Grade: B\n");
} otherwise {
    show("Grade: C\n");
}
```

## 6. Loops
**The `loop` (While) Statement:**
```cvm
assume i := 0;
loop (i < 5) {
    show(i);
    i := i + 1;
}
```

**The `each` (For) Statement:**
```cvm
each (assume j := 0; j < 5; j := j + 1) {
    show(j);
}
```

## 7. Functions & Scoping
Functions are defined with `fn` and return values with `give`. CVM++ features **Lexical Block Scoping**, meaning local variables inside functions safely shadow global variables without mutating them.

```cvm
assume x := 10; // Global

fn do_math(x, y) {
    assume local_res := x + y; // Uses local 'x' argument
    give local_res;
}

show(to_str(do_math(5, 5)) + "\n"); // Prints 10
show(to_str(x) + "\n");             // Prints 10 (Global remains untouched)
```
