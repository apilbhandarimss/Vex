<div align="center">
  <h1>🚀 Vex</h1>
  <p><strong>A simple, interactive Math Scripting Language built in C by Students of Pulchowk.</strong></p>
</div>

---

## 📖 Overview

**Vex** is an easy-to-use, interactive command-line math evaluator and scripting language. It allows you to perform complex mathematical operations, define custom variables, and utilize a wide array of built-in constants and functions instantly from your terminal!

## ✨ Features

- **Interactive REPL**: A responsive loop (`Vex> `) that instantly evaluates math expressions.
- **Variables**: Assign results to variables (e.g., `x = 5 * pi`) and use the implicit `ans` variable which always holds your last result.
- **Constants**: Pre-loaded with common math constants like `pi`, `e`, `tau`, and the golden ratio `phi`.
- **Functions**: Extensive suite of trig, logarithmic, rounding, and utility functions.
- **Error Handling**: Gracefully catches unknown variables, division by zero, and syntax errors.

---

## 🛠️ Quick Start

### 1. Compilation
You'll need a C compiler (`gcc` or `clang`) installed on your system. 
Open your terminal in the project directory and run:

```bash
gcc main.c -o vex
```

### 2. Execution
Run the compiled executable to start the Vex REPL:

```bash
./vex
```

---

## 💻 Usage & Syntax

Once inside Vex, you can type expressions exactly as you would write them mathematically.

### **Mathematical Expressions**
```text
Vex> 2 + 2 * (3^2)
= 20
Vex> 10 % 3
= 1
```

### **Assigning Variables**
Variables can be whatever name you like. 
```text
Vex> radius = 10
= 10
Vex> area = pi * radius^2
= 314.159
```
*Note: The `ans` variable is automatically updated and strictly holds the result of the last evaluated expression.*

---

## 📚 Built-in Reference

### **Mathematical Functions**
Use any of these within your expressions (e.g., `sin(pi/2)` or `fact(5)`):

| Trigonometry | Log/Exp | Other Math | Utilities |
|---|---|---|---|
| `sin(x)`, `cos(x)`, `tan(x)` | `exp(x)` | `sqrt(x)`, `cbrt(x)` | `fact(n)` (Factorial) |
| `sind(x)`, `cosd(x)`, `tand(x)` | `ln(x)` | `ceil(x)`, `floor(x)`, `round(x)` | `sign(x)` |
| `asin(x)`, `acos(x)`, `atan(x)` | `log(x)` *(base 10)* | `abs(x)` | `recip(x)` |
| `sinh(x)`, `cosh(x)`, `tanh(x)` | `log2(x)` | `deg(x)`, `rad(x)` | |

### **System Commands**
Type directly into the prompt to manage your session:
- `vars` : Lists all currently defined variables and their values.
- `funcs` : Lists all built-in mathematical functions you can use.
- `consts`: Displays all available built-in mathematical constants.
- `clear` : Erases all user-defined variables (resets your session data without clearing terminal).
- `exit` or `quit`: Exits the Vex application.

---

<div align="center">
  <i>Developed with ❤️ by the Students of Pulchowk.</i>
  <i>Apil Bhandari, Nischal Subedi, Aayush Parajuli</i>
</div>
