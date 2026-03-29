#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>


typedef struct { const char *name; double value; } ConstEntry;

static const ConstEntry consts[] = {
    { "pi",   M_PI             },
    { "e",    M_E              },
    { "tau",  2.0 * M_PI      },
    { "phi",  1.61803398874989 },
    { "inf",  HUGE_VAL         },
    { NULL,   0                }
};

typedef double (*MathFn)(double);
typedef struct { const char *name; MathFn fn; } FuncEntry;

static double fnDeg2Rad(double d) { return d * M_PI / 180.0; }
static double fnRad2Deg(double r) { return r * 180.0 / M_PI; }
static double fnSind(double d)    { return sin(d * M_PI / 180.0); }
static double fnCosd(double d)    { return cos(d * M_PI / 180.0); }
static double fnTand(double d)    { return tan(d * M_PI / 180.0); }
static double fnLog2(double x)   { return log(x) / log(2.0); }  
static double fnFact(double n) {
    if (n < 0 || n != floor(n)) return NAN;
    long long r = 1;
    for (long long i = 2; i <= (long long)n; i++) r *= i;
    return (double)r;
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
    { "sind",  fnSind    },
    { "cosd",  fnCosd    },
    { "tand",  fnTand    },
    { "deg",   fnRad2Deg },
    { "rad",   fnDeg2Rad },
    { "exp",   exp        },
    { "ln",    log        },
    { "log",   log10      },
    { "log2",  fnLog2   },
    { "sqrt",  sqrt       },
    { "cbrt",  cbrt       },
    { "ceil",  ceil       },
    { "floor", floor      },
    { "round", round      },
    { "abs",   fabs       },
    { "fact",  fnFact    },
    { "sign",  fnSign    },
    { "recip", fnRecip   },
    { NULL,    NULL       }
};


#define maxVars     256
#define varNameMax  64

typedef struct { char name[varNameMax]; double value; } Var;
static Var   varStore[maxVars];
static int   varCount = 0;

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
        fputs("Error: variable table full\n", stderr);
        return;
    }
    strncpy(varStore[varCount].name, name, varNameMax - 1);
    varStore[varCount].name[varNameMax - 1] = '\0';
    varStore[varCount].value = value;
    varCount++;
}


static void skipSpaces(const char **s) {
    while (**s && isspace((unsigned char)**s)) (*s)++;
}

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
    size_t len = (size_t)((*p - start) - 1);
    char *buf = malloc(len + 1);
    if (!buf) { fputs("Out of memory\n", stderr); exit(1); }
    strncpy(buf, start, len);
    buf[len] = '\0';
    return buf;
}



static double evalExpr(const char *input);   
static double parseExpr(const char **p);     

static double parseAtom(const char **p) {
    skipSpaces(p);

    if (**p == '-') { (*p)++; return -parseAtom(p); }
    if (**p == '+') { (*p)++; return  parseAtom(p); }

    if (**p == '(') {
        char *inner = extractParens(p);
        double v = evalExpr(inner);
        free(inner);
        return v;
    }

    if (isalpha((unsigned char)**p) || **p == '_') {
        char id[varNameMax];
        int  len = 0;
        while ((isalnum((unsigned char)**p) || **p == '_') && len < varNameMax - 1)
            id[len++] = *(*p)++;
        id[len] = '\0';

        skipSpaces(p);

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

        
        for (int i = 0; consts[i].name; i++)
            if (strcmp(id, consts[i].name) == 0)
                return consts[i].value;

        
        double *slot = varFind(id);
        if (slot) return *slot;

        fprintf(stderr, "Error: undefined name '%s'\n", id);
        return 0;
    }

    
    char *end;
    double val = strtod(*p, &end);
    if (end == *p) {
        fprintf(stderr, "Error: unexpected character '%c'\n", **p);
        return 0;
    }
    *p = end;
    return val;
}


static double parsePower(const char **p) {
    double base = parseAtom(p);
    skipSpaces(p);
    if (**p == '^') {
        (*p)++;
        double expVal = parsePower(p);   
        return pow(base, expVal);
    }
    return base;
}


static double parseTerm(const char **p) {
    double result = parsePower(p);
    skipSpaces(p);
    while (**p == '*' || **p == '/' || **p == '%') {
        char op = *(*p)++;
        double rhs = parsePower(p);
        if (op == '*') {
            result *= rhs;
        } else if (op == '/') {
            if (rhs == 0.0) { fputs("Error: division by zero\n", stderr); return 0; }
            result /= rhs;
        } else {                          
            if (rhs == 0.0) { fputs("Error: modulo by zero\n", stderr); return 0; }
            result = fmod(result, rhs);
        }
        skipSpaces(p);
    }
    return result;
}


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


static double evalExpr(const char *input) {
    const char *p = input;
    skipSpaces(&p);

    
    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *save = p;
        char id[varNameMax]; int len = 0;
        while ((isalnum((unsigned char)*p) || *p == '_') && len < varNameMax - 1)
            id[len++] = *p++;
        id[len] = '\0';
        skipSpaces(&p);
        if (*p == '=') {
            p++;   
            double val = parseExpr(&p);
            varSet(id, val);
            return val;
        }
        p = save;   
    }

    return parseExpr(&p);
}


static void printResult(double r) {
    if (isnan(r))              { puts("= NaN (not a number)"); return; }
    if (isinf(r))              { puts(r > 0 ? "= +Infinity" : "= -Infinity"); return; }
    if (r == (long long)r && fabs(r) < 1e15)
                               printf("= %lld\n", (long long)r);
    else                       printf("= %g\n", r);
}


#define inputMax 4096
static char line[inputMax];

int main(void) {
    puts("Vex Version 0.2.0");
    puts("Vex — Math Scripting Language by Students of Pulchowk");
    puts("Commands: vars | funcs | consts | clear | exit");
    puts("Example:  x = 3 * pi   then   sin(x) + ans\n");

    varSet("ans", 0.0);   

    while (1) {
        fputs("Vex> ", stdout);
        fflush(stdout);

        if (!fgets(line, inputMax, stdin)) break;   

        
        int end = (int)strlen(line) - 1;
        while (end >= 0 && isspace((unsigned char)line[end])) line[end--] = '\0';

        if (line[0] == '\0') continue;   

        
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
            
            double lastAns = varStore[0].value;   
            varCount = 0;
            varSet("ans", lastAns);
            puts("Variables cleared.");
            continue;
        }

        
        double res = evalExpr(line);
        varSet("ans", res);
        printResult(res);
    }

    puts("\nBye!");
    return 0;
}