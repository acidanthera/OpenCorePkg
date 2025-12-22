# MATOPS - Matrix Operations Calculator

**Author:** Anant Rajput  
**Version:** 2.2.0  
**Last Updated:** December 22, 2025

A comprehensive C program for performing essential matrix operations including multiplication, determinants, inverses, characteristic equations, eigenvalues, eigenvectors, and transpose operations. Features password protection and an intuitive menu-driven interface.

---

## Table of Contents

- [Features](#features)
- [Operations Supported](#operations-supported)
- [Prerequisites](#prerequisites)
- [Compilation](#compilation)
- [Usage](#usage)
- [Examples](#examples)
- [Error Handling](#error-handling)
- [Code Structure](#code-structure)
- [Limitations](#limitations)
- [Possible Enhancements](#possible-enhancements)
- [Contributing](#contributing)
- [License](#license)

---

## Features

- **Seven Matrix Operations:** Multiplication, determinant calculation, matrix inversion, characteristic equations, eigenvalues, eigenvectors, and transpose
- **Password Protection:** Secure login system with 5 attempt limit (default password: 12345)
- **Input Validation:** Ensures valid dimensions and operation compatibility
- **Interactive Menu:** User-friendly interface with clear operation selection
- **System Settings:** View version, author, maintainer, and update information
- **Memory Efficient:** Uses Variable Length Arrays (VLA) for dynamic allocation
- **Square Matrix Support:** Specialized operations for 1×1, 2×2, and 3×3 matrices

---

## Operations Supported

### 1. Matrix Multiplication
- Multiplies two matrices of compatible dimensions
- Validates that columns of Matrix A equal rows of Matrix B
- Supports matrices of any valid size (limited by stack memory)

### 2. Determinant Calculation
- Computes determinants for square matrices
- Supports 1×1, 2×2, and 3×3 matrices
- Uses standard cofactor expansion method

### 3. Matrix Inverse
- Calculates inverse using the adjugate method
- Supports 1×1, 2×2, and 3×3 matrices
- Checks for singular matrices (determinant = 0)
- Output in floating-point format with 2 decimal precision

### 4. Characteristic Equation
- Derives the characteristic polynomial
- Supports 2×2 and 3×3 matrices
- Output format:
  - 2×2: `λ² - (trace)λ + det`
  - 3×3: `λ³ - (trace)λ² + (sum of minors)λ - det`

### 5. Eigenvalues
- Calculates eigenvalues for 2×2 matrices
- Uses quadratic formula from characteristic equation
- Displays both eigenvalues (real roots)

### 6. Eigenvectors
- Computes eigenvectors for 2×2 matrices
- Displays vectors corresponding to each eigenvalue
- Uses null space method

### 7. Transpose
- Computes transpose for any matrix size
- Swaps rows and columns
- Supports both square and rectangular matrices

### 8. System Settings
- View version number
- View author information
- View maintainer details
- Check recent update date
- Return to main menu

---

## Prerequisites

- **C Compiler:** GCC or any compiler supporting C99 standard
- **Math Library:** Requires linking with math library (`-lm` flag)
- **Basic understanding** of linear algebra concepts

---

## Compilation

```bash
gcc matops.c -o matops -lm
```

Or explicitly with C99 standard:

```bash
gcc -std=c99 matops.c -o matops -lm
```

**Note:** The `-lm` flag links the math library required for `sqrt()` function.

---

## Usage

1. **Run the program:**
   ```bash
   ./matops
   ```

2. **Enter password:** Default password is `12345` (5 attempts allowed)

3. **Select an operation** from the menu (0-8)

4. **Enter matrix dimensions** when prompted

5. **Input matrix elements** as requested

6. **View results** or error messages

7. **Access System Settings** (option 8) for program information

---

## Examples

### Example 1: Matrix Multiplication

```
WELCOME TO MATOPS
__________________
ENTER PASSWORD TO LOG IN: 12345

WELCOME TO MATOPS
__________________
CHOOSE OPERATIONS:
1. MATRIX MULTIPLICATION
...
ENTER YOUR CHOICE: 1

ENTER THE NUMBER OF ROWS OF MATRIX A: 2
ENTER THE NUMBER OF COLUMN OF MATRIX A: 3
ENTER THE NUMBER OF ROWS IN MATRIX B: 3
ENTER THE NUMBER OF COLUMN IN MATRIX B: 2

ENTER THE ELEMENTS OF MATRIX A:
1 2 3
4 5 6

ENTER THE ELEMENTS OF MATRIX B:
7 8
9 10
11 12

THE RESULT OF MULTIPLICATION OF MATRIX A AND MATRIX B IS:
58      64
139     154
```

### Example 2: Determinant (3×3 Matrix)

```
ENTER YOUR CHOICE: 2
ENTER THE NUMBER OF ROWS OF MATRIX A: 3
ENTER THE NUMBER OF COLUMN OF MATRIX A: 3

ENTER THE ELEMENTS OF MAT A:
1 2 3
0 1 4
5 6 0

DETERMINANT OF MATRIX A IS: 1
```

### Example 3: Matrix Inverse (2×2)

```
ENTER YOUR CHOICE: 3
ENTER THE NUMBER OF ROWS OF MATRIX A: 2
ENTER THE NUMBER OF COLUMN OF MATRIX A: 2

ENTER ELEMENTS OF MATRIX A:
4 7
2 6

INVERSE OF MATRIX A IS:
0.60    -0.70
-0.20    0.40
```

### Example 4: Eigenvalues (2×2)

```
ENTER YOUR CHOICE: 5
ENTER THE NUMBER OF ROWS OF MATRIX A: 2
ENTER THE NUMBER OF COLUMN OF MATRIX A: 2

ENTER ELEMENTS OF MATRIX A:
3 1
1 3

EIGEN VALUE OF MATRIX A IS: 4.000000 and 2.000000
```

### Example 5: Transpose

```
ENTER YOUR CHOICE: 7
ENTER THE NUMBER OF ROWS OF MATRIX A: 2
ENTER THE NUMBER OF COLUMN OF MATRIX A: 3

ENTER ELEMENTS OF MATRIX A:
1 2 3
4 5 6

THE TRANSPOSE OF THE MATRIX A IS:
1       4
2       5
3       6
```

---

## Error Handling

The program validates and handles:

- **Zero Dimensions:** Rejects matrices with zero rows or columns
- **Non-Square Matrices:** For operations requiring square matrices (determinant, inverse, eigenvalues)
- **Incompatible Dimensions:** For matrix multiplication (columns of A ≠ rows of B)
- **Singular Matrices:** Detects when inverse doesn't exist (determinant = 0)
- **Unsupported Sizes:** Provides clear messages for matrix sizes beyond current support
- **Password Protection:** Locks after 5 failed login attempts
- **Invalid Menu Choices:** Prompts for valid input (0-8)

---

## Code Structure

```
main()
├── Password Authentication Loop
│   └── 5 attempts allowed
├── Menu Display
├── Choice Selection
└── Operation Branches
    ├── Matrix Multiplication (choice 1)
    ├── Determinant Calculation (choice 2)
    ├── Matrix Inverse (choice 3)
    ├── Characteristic Equation (choice 4)
    ├── Eigenvalues (choice 5)
    ├── Eigenvectors (choice 6)
    ├── Transpose (choice 7)
    └── System Settings (choice 8)
        ├── Version Number
        ├── Author
        ├── Maintainer
        └── Recent Update Date
```

Each operation includes:
- Dimension input and validation
- Matrix element input
- Computation using appropriate algorithm
- Formatted output display

---

## Limitations

### Matrix Size:
- **Multiplication/Transpose:** Any valid size (limited by stack memory)
- **Determinant/Inverse:** Up to 3×3 matrices
- **Characteristic Equation:** 2×2 and 3×3 matrices only
- **Eigenvalues/Eigenvectors:** Only 2×2 matrices

### Data Types:
- Integer input for most operations
- Float output for inverse and eigenvalues
- No support for complex numbers

### Other Limitations:
- **Complex Numbers:** Does not handle complex eigenvalues/eigenvectors
- **Numerical Precision:** Subject to floating-point arithmetic limitations
- **Memory:** VLA size limited by available stack memory
- **Password:** Hardcoded (not user-configurable without recompilation)

---

## Possible Enhancements

- Support for larger matrices (4×4, 5×5, etc.)
- User-configurable password system
- Gaussian elimination for general matrix inversion
- LU decomposition for improved determinant calculation
- QR algorithm for eigenvalue computation of larger matrices
- Complex eigenvalue/eigenvector support
- Matrix addition and subtraction operations
- File I/O for loading/saving matrices
- Error tolerance settings for floating-point comparisons
- Matrix rank calculation
- Reduced row echelon form (RREF)
- Singular Value Decomposition (SVD)
- Save operation history
- Export results to file

---

## Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/enhancement`)
3. Commit your changes (`git commit -m 'Add new feature'`)
4. Push to the branch (`git push origin feature/enhancement`)
5. Open a Pull Request

Please ensure your code:
- Follows existing style conventions
- Includes appropriate comments
- Handles edge cases and errors
- Has been tested with various inputs
- Updates documentation as needed

---

## License

This project is open-source under the MIT License.

---

## Contact

**Maintainer:** Anant Rajput

For bug reports, feature requests, or contributions, please open an issue in the repository.

---

**Note:** Default password is `12345`. Change it in the source code for security purposes in production use.