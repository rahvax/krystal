## Interpreter pipeline

1. The **lexer** reads a `.krys` file and converts every character into tokens
   (`INT`, `BOOL`, `ATOM`, `LET`, `;`, etc.).
2. The **parser** consumes those tokens and builds the AST that describes the
   structure of the program (sequence of statements, literals, bindings).
3. The **printer** walks the AST and emits a friendly representation. In the
   future, this step will hand the AST to the evaluator/runtime.

### AST overview diagram

```
source.krys
    |
    v
 +--------+     tokens      +--------+      AST      +-----------+
 |  Lexer | ------------->  | Parser | ----------->  | AST Print |
 +--------+                 +--------+               +-----------+
    ^                                                    |
    |                                                    v
 characters                                         printed tree
```

A quick example:

```text
let answer = 42;
:done;
```

Produces:

```
Program
  Let(answer)
    Int(42)
  Atom(:done)
```

## Grammar subset

- `let name = literal;`
- Literal statements (`literal;`)
- Literals allowed: integers (`123`), booleans (`true`/`false`), atoms (`:foo_bar`)
- Comments start with `#` and continue to the end of the line.

## Building

```bash
make
```

This generates the `krystal` binary in the project root.

## Running

```bash
./krystal examples/demo.krys
```

The program will always print the AST for the provided file, so tweaking the
source is a simple way to see how the parser understands your code.
