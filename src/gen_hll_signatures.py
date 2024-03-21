#!/usr/bin/env python3
import sys

signatures = [
    'iip',
    'ip',
    'ipi',
    'vi',
    'vip',
    'f',
    'ff',
    'fff',
    'i',
    'ii',
    'iii',
    'iiii',
    'iiiii',
    'iiiiiiii',
    'iiiip',
    'iiiippp',
    'iiip',
    'iiipp',
    'iiipppp',
    'iipp',
    'ipp',
    'ippifpp',
    'p',
    'v',
    'viffiffff',
    'vii',
    'viii',
    'viiii',
    'viiiiii',
    'viiiiiiff',
    'viiiiiii',
    'viiiiiiiff',
    'viiiiiiii',
    'viiiiiiiii',
    'viiiiiiiiii',
    'viiiiiiiiiii',
    'viiiiiiiiiiii',
    'viiip',
    'vipi',
    'vp',
    'vpi',
    'vpii',
    'vpiii',
    'vpp',
    'vppp',
    'vpppp',
]

if len(sys.argv) > 1:
    output = open(sys.argv[1], 'w')
else:
    output = sys.stdout

def write(s):
    print(s, end='', file=output)

def writeln(s):
    print(s, file=output)

def ctype(t):
    return {
        'v': 'void',
        'i': 'int',
        'f': 'float',
        'p': 'void*',
    }[t]

def functype(sig):
    argtypes = ", ".join(ctype(t) for t in sig[1:]) or 'void'
    return f'{ctype(sig[0])}(*)({argtypes})'

def argexpr(i, t):
    match t:
        case 'i':
            return f'((union vm_value*)args[{i}])->i'
        case 'f':
            return f'((union vm_value*)args[{i}])->f'
        case 'p':
            return f'*(void**)args[{i}]'
        case _:
            raise ValueError(f'unknown arg type {t}')

write('''\
enum hll_signature {
''')

for sig in signatures:
    writeln(f'\tHLL_SIG_{sig.upper()},')

write('''\
	HLL_SIG_UNSUPPORTED,
};

struct signature_table {
	const char *key;
	enum hll_signature val;
};

static const struct signature_table hll_signatures[] = {
''')

for sig in signatures:
    writeln(f'\t{{ "{sig}", HLL_SIG_{sig.upper()} }},')

write('''\
	{ NULL, HLL_SIG_UNSUPPORTED },
};

static void ffi_call(const enum hll_signature *cif, void *fun, union vm_value *r, void **args)
{
	switch (*cif) {
''')

for sig in signatures:
    writeln(f'\tcase HLL_SIG_{sig.upper()}:')
    return_type, argtypes = sig[0], sig[1:]
    match return_type:
        case 'v':
            write(f'\t\t')
        case 'i':
            write(f'\t\tr->i = ')
        case 'f':
            write(f'\t\tr->f = ')
        case 'p':
            write(f'\t\tr->ref = ')
        case _:
            raise ValueError(f'unknown return type {return_type}')
    args = ", ".join(argexpr(i, t) for i, t in enumerate(argtypes))
    writeln(f'(({functype(sig)})fun)({args});')
    writeln(f'\t\tbreak;')

write('''\
	case HLL_SIG_UNSUPPORTED:
		ERROR("unsupported HLL signature");
	}
}
''')
