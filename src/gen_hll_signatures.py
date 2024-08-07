#!/usr/bin/env python3
import sys

signatures = [
    'f',
    'ff',
    'fff',
    'fi',
    'fif',
    'fii',
    'fiiff',
    'fiii',
    'fiiii',
    'fiiiiii',
    'fp',
    'i',
    'if',
    'ii',
    'iif',
    'iifff',
    'iifffff',
    'iiffffff',
    'iiffi',
    'iifi',
    'iii',
    'iiif',
    'iiiff',
    'iiifff',
    'iiii',
    'iiiif',
    'iiiiff',
    'iiiifff',
    'iiiifffppp',
    'iiiii',
    'iiiiif',
    'iiiiii',
    'iiiiiii',
    'iiiiiiif',
    'iiiiiiifiiifi',
    'iiiiiiifiiifii',
    'iiiiiiii',
    'iiiiiiiiif',
    'iiiiiiiiii',
    'iiiiiiiiiii',
    'iiiiiiiiiiif',
    'iiiiiiiiiiiii',
    'iiiiiiip',
    'iiiiip',
    'iiiiippp',
    'iiiip',
    'iiiipiiiiifiiifiii',
    'iiiipp',
    'iiiippp',
    'iiip',
    'iiipp',
    'iiippp',
    'iiipppp',
    'iip',
    'iipi',
    'iipii',
    'iipiiii',
    'iipiiiiiiiiii',
    'iipiiiiiiiiiiiii',
    'iipiiiip',
    'iipp',
    'iippp',
    'iipppp',
    'ip',
    'ipf',
    'ipi',
    'ipii',
    'ipiii',
    'ipiiiii',
    'ipiiiiiipp',
    'ipiiiipiiiiii',
    'ipiiip',
    'ipiip',
    'ipip',
    'ipipp',
    'ipp',
    'ippifpp',
    'ippii',
    'ippiii',
    'ippip',
    'ippp',
    'ipppii',
    'ippppp',
    'ipppppppp',
    'p',
    'pf',
    'pi',
    'pii',
    'piii',
    'pp',
    'ppii',
    'v',
    'vf',
    'vfi',
    'vi',
    'vif',
    'vifffff',
    'viffffiip',
    'viffiffff',
    'viffiip',
    'vifi',
    'vii',
    'viii',
    'viiii',
    'viiiif',
    'viiiii',
    'viiiiii',
    'viiiiiiff',
    'viiiiiiffff',
    'viiiiiiffffi',
    'viiiiiifiiif',
    'viiiiiii',
    'viiiiiiiff',
    'viiiiiiii',
    'viiiiiiiii',
    'viiiiiiiiii',
    'viiiiiiiiiii',
    'viiiiiiiiiiii',
    'viiiiiiiiiiiii',
    'viiiiiiip',
    'viiiiiip',
    'viiiiip',
    'viiiip',
    'viiip',
    'viiipp',
    'viip',
    'vip',
    'vipi',
    'vipp',
    'vippp',
    'vipppp',
    'vipppppppppp',
    'vp',
    'vpi',
    'vpii',
    'vpiii',
    'vpiiii',
    'vpiiip',
    'vpiip',
    'vpip',
    'vpp',
    'vppp',
    'vpppp',
    'vpppppppp',
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

''')

for sig in signatures:
    writeln(f'static union vm_value hllcall_{sig}(void *fun, void **args)')
    writeln('{')
    return_type, argtypes = sig[0], sig[1:]
    match return_type:
        case 'v':
            write(f'\t')
        case 'i':
            write(f'\treturn (union vm_value){{ .i = ')
        case 'f':
            write(f'\treturn (union vm_value){{ .f = ')
        case 'p':
            write(f'\treturn (union vm_value){{ .ref = ')
        case _:
            raise ValueError(f'unknown return type {return_type}')
    args = ", ".join(argexpr(i, t) for i, t in enumerate(argtypes))
    write(f'(({functype(sig)})fun)({args})')
    if return_type == 'v':
        writeln(';')
        writeln('\treturn (union vm_value){};')
    else:
        writeln(' };')
    writeln('}')
    writeln('')

write('''\
static union vm_value hllcall_unsupported(void *fun, void **args)
{
	ERROR("unsupported HLL signature");
}

typedef union vm_value (*hllcall_func)(void *fun, void **args);

static const hllcall_func hllcall_table[] = {
''')
for sig in signatures:
    writeln(f'\thllcall_{sig},')
write('''\
	hllcall_unsupported
};

static void ffi_call(const enum hll_signature *cif, void *fun, union vm_value *r, void **args)
{
	*r = hllcall_table[*cif](fun, args);
}
''')
