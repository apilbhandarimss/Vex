/*
 * Vex – Math Scripting Language
 * Version 0.2.1 (refined)
 *
 * A simple recursive‑descent parser/interpreter for arithmetic expressions,
 * built‑in constants, single‑argument functions, and variables.
 * Commands: vars, funcs, consts, clear, open, help, exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

/* ---------- Constants ---------- */
typedef struct { const char *name; double value; } ConstEntry;

static const ConstEntry consts[] = {
    { "pi",   M_PI             },
    { "e",    M_E              },
    { "tau",  2.0 * M_PI      },
    { "phi",  1.61803398874989 },
    { "inf",  HUGE_VAL         },
    { NULL,   0                }
};

/* ---------- Built‑in functions (all take one double argument) ---------- */
typedef double (*MathFn)(double);
typedef struct { const char *name; MathFn fn; } FuncEntry;

static double fnDeg2Rad(double d) { return d * M_PI / 180.0; }
static double fnRad2Deg(double r) { return r * 180.0 / M_PI; }
static double fnSind(double d)    { return sin(d * M_PI / 180.0); }
static double fnCosd(double d)    { return cos(d * M_PI / 180.0); }
static double fnTand(double d)    { return tan(d * M_PI / 180.0); }
static double fnLog2(double x)    { return log(x) / log(2.0); }

/* Factorial using double to avoid overflow (returns INF for n > 170) */
static double fnFact(double n) {
    if (n < 0 || n != floor(n)) return NAN;
    long long intN = (long long)n;
    if (intN > 170) return HUGE_VAL;   /* 171! overflows double */
    double r = 1.0;
    for (long long i = 2; i <= intN; i++) r *= (double)i;
    return r;
}

static double fnSign(double x)    { return (x > 0) - (x < 0); }
static double fnRecip(double x)   { return (x == 0) ? NAN : 1.0 / x; }

static const FuncEntry funcs[] = {
    { "sin",   sin        },
    { "cos",   cos        },
    { "tan",   tan        },
    { "asin",  asin       },
    { "acos",  acos       },
    { "atan",  atan       },
    { "sinh",  sinh       },
    { "cosh",  cosh       },
    { "tanh",  tanh       },
    { "sind",  fnSind     },
    { "cosd",  fnCosd     },
    { "tand",  fnTand     },
    { "deg",   fnRad2Deg  },
    { "rad",   fnDeg2Rad  },
    { "exp",   exp        },
    { "ln",    log        },
    { "log",   log10      },
    { "log2",  fnLog2     },
    { "sqrt",  sqrt       },
    { "cbrt",  cbrt       },
    { "ceil",  ceil       },
    { "floor", floor      },
    { "round", round      },
    { "abs",   fabs       },
    { "fact",  fnFact     },
    { "sign",  fnSign     },
    { "recip", fnRecip    },
    { NULL,    NULL       }
};

/* ---------- Variable storage ---------- */
#define maxVars    256
#define varNameMax 64

typedef struct { char name[varNameMax]; double value; } Var;
static Var  varStore[maxVars];
static int  varCount = 0;

static double *varFind(const char *name) {
    for (int i = 0; i < varCount; i++)
        if (strcmp(varStore[i].name, name) == 0)
            return &varStore[i].value;
    return NULL;
}

static void varSet(const char *name, double value) {
    double *slot = varFind(name);
    if (slot) { *slot = value; return; }
    if (varCount >= maxVars) {
        fprintf(stderr, "Error: variable table full, '%s' not stored\n", name);
        return;
    }
    strncpy(varStore[varCount].name, name, varNameMax - 1);
    varStore[varCount].name[varNameMax - 1] = '\0';
    varStore[varCount].value = value;
    varCount++;
}

/* ---------- Parser & evaluator ---------- */

static void skipSpaces(const char **s) {
    while (**s && isspace((unsigned char)**s)) (*s)++;
}

/* Extract content inside parentheses, handling nested '(' and ')'.
   Returns a newly allocated string, which the caller must free. */
static char *extractParens(const char **p) {
    if (**p != '(') return NULL;
    (*p)++;
    const char *start = *p;
    int depth = 1;
    while (**p && depth > 0) {
        if      (**p == '(') depth++;
        else if (**p == ')') depth--;
        (*p)++;
    }
    /* *p points after the closing ')' */
    size_t len = (size_t)((*p - start) - 1);  /* exclude the closing ')' */
    char *buf = malloc(len + 1);
    if (!buf) { fputs("Out of memory\n", stderr); exit(1); }
    strncpy(buf, start, len);
    buf[len] = '\0';
    return buf;
}

/* Forward declarations */
static double evalExpr(const char *input);
static double parseExpr(const char **p);

/* Parse an atom: number, constant, variable, function call, or parenthesised expression */
static double parseAtom(const char **p) {
    skipSpaces(p);

    /* Unary + / - */
    if (**p == '-') { (*p)++; return -parseAtom(p); }
    if (**p == '+') { (*p)++; return  parseAtom(p); }

    /* Parenthesised expression */
    if (**p == '(') {
        char *inner = extractParens(p);
        double v = evalExpr(inner);
        free(inner);
        return v;
    }

    /* Identifier: variable, constant, or function */
    if (isalpha((unsigned char)**p) || **p == '_') {
        char id[varNameMax];
        int  len = 0;
        while ((isalnum((unsigned char)**p) || **p == '_') && len < varNameMax - 1)
            id[len++] = *(*p)++;
        id[len] = '\0';

        skipSpaces(p);

        /* Function call? */
        if (**p == '(') {
            for (int i = 0; funcs[i].name; i++) {
                if (strcmp(id, funcs[i].name) == 0) {
                    char *inner = extractParens(p);
                    double v = funcs[i].fn(evalExpr(inner));
                    free(inner);
                    return v;
                }
            }
            fprintf(stderr, "Warning: unknown function '%s', evaluating its argument\n", id);
            char *inner = extractParens(p);
            double v = evalExpr(inner);
            free(inner);
            return v;
        }

        /* Constant? */
        for (int i = 0; consts[i].name; i++)
            if (strcmp(id, consts[i].name) == 0)
                return consts[i].value;

        /* Variable? */
        double *slot = varFind(id);
        if (slot) return *slot;

        fprintf(stderr, "Error: undefined name '%s'\n", id);
        return NAN;
    }

    /* Numeric literal */
    char *end;
    double val = strtod(*p, &end);
    if (end == *p) {
        fprintf(stderr, "Error: unexpected character '%c'\n", **p);
        return NAN;
    }
    *p = end;
    return val;
}

/* Parse exponentiation (right‑associative) */
static double parsePower(const char **p) {
    double base = parseAtom(p);
    skipSpaces(p);
    if (**p == '^') {
        (*p)++;
        double expVal = parsePower(p);   /* right‑associative */
        return pow(base, expVal);
    }
    return base;
}

/* Parse * / % (left‑associative) */
static double parseTerm(const char **p) {
    double result = parsePower(p);
    skipSpaces(p);
    while (**p == '*' || **p == '/' || **p == '%') {
        char op = *(*p)++;
        double rhs = parsePower(p);
        if (op == '*') {
            result *= rhs;
        } else if (op == '/') {
            if (rhs == 0.0) { fputs("Error: division by zero\n", stderr); return NAN; }
            result /= rhs;
        } else { /* '%' */
            if (rhs == 0.0) { fputs("Error: modulo by zero\n", stderr); return NAN; }
            result = fmod(result, rhs);
        }
        skipSpaces(p);
    }
    return result;
}

/* Parse + / - (left‑associative) */
static double parseExpr(const char **p) {
    double result = parseTerm(p);
    skipSpaces(p);
    while (**p == '+' || **p == '-') {
        char op = *(*p)++;
        double rhs = parseTerm(p);
        result = (op == '+') ? result + rhs : result - rhs;
        skipSpaces(p);
    }
    return result;
}

/* Evaluate a whole expression; also handles assignment. */
static double evalExpr(const char *input) {
    const char *p = input;
    skipSpaces(&p);

    /* Check if it's an assignment: identifier followed by '=' */
    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *save = p;
        char id[varNameMax]; int len = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && len < varNameMax - 1)
            id[len++] = *p++;
        id[len] = '\0';
        skipSpaces(&p);
        if (*p == '=') {
            p++;   /* skip '=' */
            double val = parseExpr(&p);
            skipSpaces(&p);
            if (*p != '\0') {
                fprintf(stderr, "Warning: extra characters after assignment ignored\n");
            }
            varSet(id, val);
            return val;
        }
        p = save;  /* not an assignment, restore */
    }

    double result = parseExpr(&p);
    skipSpaces(&p);
    if (*p != '\0') {
        fprintf(stderr, "Warning: trailing characters ignored after expression\n");
    }
    return result;
}

/* ---------- Output formatting ---------- */
static void printResult(double r) {
    if (isnan(r))             { puts("= NaN (not a number)"); return; }
    if (isinf(r))             { puts(r > 0 ? "= +Infinity" : "= -Infinity"); return; }
    if (r == (long long)r && fabs(r) < 1e15)
                              printf("= %lld\n", (long long)r);
    else                      printf("= %g\n", r);
}

/* ---------- File execution ---------- */
static void runFile(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "Error: cannot open file '%s'\n", filename); return; }
    char fileLine[4096];
    int lineNum = 0;
    while (fgets(fileLine, sizeof(fileLine), f)) {
        lineNum++;
        /* Remove trailing newline */
        char *nl = strchr(fileLine, '\n');
        if (nl) *nl = '\0';
        /* Skip empty lines */
        if (fileLine[0] == '\0') continue;

        printf("  %s\n", fileLine);
        double res = evalExpr(fileLine);
        if (!isnan(res)) {   /* only update ans if valid */
            varSet("ans", res);
        }
        printf("  ");
        printResult(res);
    }
    fclose(f);
}

/* ---------- Main REPL ---------- */
#define inputMax 4096
static char line[inputMax];

int main(void) {
    puts("Vex Version 0.2.1");
    puts("Vex — Math Scripting Language (refined)");
    puts("Commands: vars | funcs | consts | clear | open | help | exit");
    puts("Example:  x = 3 * pi   then   sin(x) + ans\n");

    varSet("ans", 0.0);

    while (1) {
        printf("Vex> ");
        if (!fgets(line, inputMax, stdin)) break;

        /* Remove trailing newline and spaces */
        int len = (int)strlen(line);
        while (len > 0 && isspace((unsigned char)line[len-1])) line[--len] = '\0';

        if (line[0] == '\0') continue;

        /* ----- Handle built‑in commands ----- */
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) break;

        if (strcmp(line, "vars") == 0) {
            if (varCount == 0) { puts("(no variables defined)"); continue; }
            for (int i = 0; i < varCount; i++) {
                double v = varStore[i].value;
                if (v == (long long)v && fabs(v) < 1e15)
                    printf("  %-20s = %lld\n", varStore[i].name, (long long)v);
                else
                    printf("  %-20s = %g\n",   varStore[i].name, v);
            }
            continue;
        }

        if (strcmp(line, "funcs") == 0) {
            puts("Built-in functions (all take one argument):");
            for (int i = 0; funcs[i].name; i++)
                printf("  %s()\n", funcs[i].name);
            continue;
        }

        if (strcmp(line, "consts") == 0) {
            puts("Built-in constants:");
            for (int i = 0; consts[i].name; i++)
                printf("  %-10s = %g\n", consts[i].name, consts[i].value);
            continue;
        }

        if (strcmp(line, "clear") == 0) {
            double lastAns = varStore[0].value;  /* ans is always at index 0 */
            varCount = 0;
            varSet("ans", lastAns);
            puts("Variables cleared.");
            continue;
        }

        if (strcmp(line, "open") == 0) {
            char fname[512];
            printf("Enter filename: ");
            if (!fgets(fname, sizeof(fname), stdin)) {
                puts("Input error.");
                continue;
            }
            /* Remove newline */
            char *nl = strchr(fname, '\n');
            if (nl) *nl = '\0';
            runFile(fname);
            continue;
        }

        if (strcmp(line, "help") == 0) {
            puts("Available commands:");
            puts("  vars     – show all defined variables");
            puts("  funcs    – list built‑in functions");
            puts("  consts   – list built‑in constants");
            puts("  clear    – clear all variables (keeps 'ans')");
            puts("  open     – read and execute a script file");
            puts("  help     – show this help");
            puts("  exit     – quit the program");
            puts("\nExpressions can use:");
            puts("  +  -  *  /  %  ^  (  )");
            puts("  unary + and -");
            puts("  variable assignment:  x = 3 * pi");
            puts("  built‑in functions:   sin(0.5)");
            puts("  constants: pi, e, tau, phi, inf");
            puts("  the special variable 'ans' holds the last result.");
            continue;
        }

        /* ----- Evaluate expression ----- */
        double res = evalExpr(line);
        if (!isnan(res) && !isinf(res)) {  /* only store if valid numeric */
            varSet("ans", res);
        }
        printResult(res);
    }

    puts("\nBye!");
    return 0;
}