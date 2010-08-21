#!/usr/bin/python

#       USER MODIFIABLE
# Possible build flags are "force", "quiet", "nodebug", "nogc", "profile"
modules = ['Test', 'Std', 'Gsl', 'Sdl', 'Math', 'TextFile', 'Map', 'Gd']

#       BUILD SCRIPT FROM HERE ON
import os, sys, tools

CC = ["gcc"]
LD = ["ld"]
CFLAGS = ["-Wall", "-Wstrict-aliasing=0", 
          "-Wfatal-errors", "-std=gnu99", "-I."]
LFLAGS = ["-lgc"]

if "nogc" not in sys.argv:
    CFLAGS.append("-DCLIB_GC")
if "force" in sys.argv:
    tools.set_forcing(True)
if "quiet" in sys.argv:
    tools.set_quiet(True)
if "profile" in sys.argv:
    CFLAGS.append("-pg") 
    CFLAGS.append("-fno-omit-frame-pointer")
    CFLAGS.append("-O3")
    CFLAGS.append("-DNDEBUG")
if "nodebug" in sys.argv:
    CFLAGS.append("-O3")
    CFLAGS.append("-DNDEBUG")
else:
    CFLAGS.append("-g")

# Construct required directories
required_dirs = ['bin', 'product', 'product/include', 'product/include/clib',
                 'product/include/vm', 'product/include/modules',
                 'product/include/vm/builtin/']
for d in required_dirs:
    tools.mkdir_safe(d)

def link_objs(objs, output):
    # Link objects into the output program
    if tools.depends(output, objs):
        arr = tools.flatten([LD, '-r', objs, '-o', output])
        tools.pprint('LD', output)
        tools.call(arr)

def cons_obj(target, src, depends):
    tdepends = [src] + depends

    # Check timestamps
    if tools.depends(target, tdepends):
        # Build object
        tools.pprint('CC', src, target)
        tools.call([CC, CFLAGS, '-c', src, '-o', target])

def cons_yacc(target, src, depends):
    tdepends = [src] + depends

    # Check timestamps
    if tools.depends(target, tdepends):
        tools.pprint("YAC", src, target)
        tools.call(['bison', src])     

def cons_flex(target, src, depends):
    tdepends = [src] + depends

    # Check timestamps
    if tools.depends(target, tdepends):
        tools.pprint("LEX", src, target)
        tools.call(['flex', src])     

def cons_bin(target, objs, depends):
    tdepends = objs + depends
    if tools.depends(target, tdepends):
        tools.pprint('BIN', target)
        tools.call([CC, CFLAGS, LFLAGS, objs, '-o', target])

def cons_gen(gen_program, target, t):
    if tools.depends(target, [gen_program]):
        tools.pprint('GEN', gen_program, target)
        tools.call([gen_program, t, '>', target])

def cons_objs(srcs, depends):
    results = []
    for src in srcs:
        root, ext = os.path.splitext(src)
        dest = root + '.o'
        cons_obj(dest, src, depends)
        results.append(dest)
    return results

def copy_file(dest, src):
    import shutil
    if tools.depends(dest, [src]):
        tools.pprint('CP', src, dest)
        shutil.copy(src, dest)

# Temporary files
junkyard = []

# Clib
clib_hs =   [
              'clib/array.h',
              'clib/clib.h',
              'clib/dict.h',
              'clib/hash.h',
              'clib/mem.h',
              'clib/stringbuf.h',
              'clib/utf8.h'
            ]
clib_srcs = [
              'clib/array.c',
              'clib/dict.c',
              'clib/hash.c',
              'clib/mem.c',
              'clib/utf8.c',
              'clib/stringbuf.c'
            ]
clib_objs = cons_objs(clib_srcs, clib_hs)

###############################################################################
# RIPE TOOLS
###############################################################################

cons_yacc('ripe/parser.c', 'ripe/parser.y', 
          ['ripe/ripe.h'] + clib_hs)
cons_flex('ripe/scanner.c', 'ripe/scanner.l',
          ['ripe/parser.h'] + clib_hs)
ripe_hs = [
            'ripe/ripe.h',
            'ripe/parser.h',
            'ripe/scanner.h'
          ]
ripe_srcs = [
               'ripe/ripe.c',
               'ripe/cli.c',
               'ripe/build-tree.c',
               'ripe/parser.c',
               'ripe/scanner.c',
               'ripe/astnode.c',
               'ripe/operator.c',
               'ripe/generator.c'
             ]
ripe_objs = cons_objs(ripe_srcs, ripe_hs + clib_hs)
cons_bin('product/ripe', ripe_objs + clib_objs, [])

###############################################################################
# VM
###############################################################################

func_gen_objs = cons_objs(['vm/func-gen/func-gen.c'], [])
cons_bin('bin/func-gen', func_gen_objs, [])
cons_gen('bin/func-gen', 'vm/func-generated.c', 'c')
cons_gen('bin/func-gen', 'vm/func-generated.h', 'h')

ops_gen_objs = cons_objs(['vm/ops-gen/ops-gen.c'], [])
cons_bin('bin/ops-gen', ops_gen_objs, [])
cons_gen('bin/ops-gen', 'vm/ops-generated.c', 'c')
cons_gen('bin/ops-gen', 'vm/ops-generated.h', 'h')

vm_hs = [
          'vm/vm.h',
          'vm/value_inline.c',
          'vm/ops-generated.h',
          'vm/func-generated.h',
        ]

# VM
vm_srcs = [
            'vm/vm.c',
            'vm/sym-table.c',
            'vm/ops.c',
            'vm/ops-generated.c',
            'vm/util.c',
            'vm/klass.c',
            'vm/exceptions.c',
            'vm/builtin/Object.c',
            'vm/builtin/Function.c',
            'vm/func-generated.c',
            'vm/builtin/String.c',
            'vm/builtin/Integer.c',
            'vm/builtin/Flags.c',
            'vm/builtin/Double.c',
            'vm/builtin/Arrays.c',
            'vm/builtin/Range.c',
            'vm/builtin/Complex.c',
            'vm/builtin/Map.c'
          ]
vm_objs = cons_objs(vm_srcs, vm_hs + clib_hs)

# Construct VM object
link_objs(vm_objs + clib_objs, "product/vm.o")

include_headers = clib_hs + vm_hs + ['modules/modules.h']
for header in include_headers:
    copy_file('product/include/' + header, header)
f = open('product/ripe.conf', 'w')
f.write('cflags=%s\n' % " ".join(CFLAGS))
f.write('lflags=%s\n' % " ".join(LFLAGS))
f.close()

##############################################################################
# BUILD MODULES
##############################################################################

module_deps = vm_hs + clib_hs + ['modules/modules.h', 'product/ripe']

def cons_module(src, dest, module, extra_CFLAGS=''):
    if tools.depends(dest, module_deps + [src]):
        tools.pprint('MOD', src, dest)
        args = ['product/ripe', '-n', module, '-c', src, '-o', dest,
                        '-f', '"%s"' % extra_CFLAGS]
        if not tools.is_quiet():
            args.append('-v')
        tools.call(args)
    
def build_module(module):
    import os.path
    out = 'product/%s.o' % module
    
    # Deal with metafile
    metafile = 'modules/%s/%s.meta' % (module, module)
    if os.path.exists(metafile):
        copy_file('product/%s.meta' % module, metafile)
    meta = tools.load_meta(metafile)
    extra_CFLAGS = ''
    if meta.has_key('includes'):
        extra_CFLAGS = meta['includes']
    path = 'modules/%s/%s.rip' % (module, module)
    if os.path.exists(path):
        cons_module(path, out, module, extra_CFLAGS)
        return

    path = 'modules/%s/%s.c' % (module, module)
    if os.path.exists(path):
        cons_module(path, out, module, extra_CFLAGS)
        return
for module in modules:
    build_module(module)

# Cleanup
for filename in junkyard:
    tools.delete_safe(filename)

