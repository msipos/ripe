#!/usr/bin/python

# Possible build flags are "force", "quiet", "nodebug", "nogc", "profile"


DATA_TYPES = ['Array1', 'Array3', 'Double', 'Flags', 'Integer', 'Map', 'Range',
              'Set', 'String', 'Tuple']
STDLIB = ['Err', 'Math', 'Os', 'Out', 'Std', 'Stream', 'Test', 'Template',
          'TextFile', 'Time']
OPTIONAL_MODULES = ['Curl', 'Gd', 'Gsl', 'Json', 'MainLoop', 'Povray',
                    'Pthread', 'Sci', 'Sdl', 'Speech']
MODULES = DATA_TYPES + STDLIB
DEF_MODULES = DATA_TYPES + STDLIB

#       BUILD SCRIPT FROM HERE ON
import os, sys, tools

conf = tools.conf
conf["CC"] = "gcc"
conf["LD"] = "ld"
conf["CFLAGS"] = ["-Wall", "-Wstrict-aliasing=0", "-Wfatal-errors", "-std=gnu99", "-I."]
conf["LFLAGS"] = ["-lgc"]
conf["YACC"] = "bison"
conf["LEX"] = "flex"
conf["VERBOSITY"] = 1
if "nogc" not in sys.argv:
    conf["CFLAGS"].append("-DCLIB_GC")
conf["FORCING"] = False
if "force" in sys.argv:
    conf["FORCING"] = True
if "quiet" in sys.argv:
    conf["VERBOSITY"] = 0
if "verbose" in sys.argv:
    conf["VERBOSITY"] = 2
if "profile" in sys.argv:
    conf["CFLAGS"].append("-pg")
    conf["CFLAGS"].append("-fno-omit-frame-pointer")
    conf["CFLAGS"].append("-O3")
    conf["CFLAGS"].append("-DNDEBUG")
if "nodebug" in sys.argv:
    conf["CFLAGS"].append("-O3")
    conf["CFLAGS"].append("-DNDEBUG")
else:
    conf["CFLAGS"].append("-g")

# Construct required directories
required_dirs = ['bin', 'product', 'product/include', 'product/include/clib',
                 'product/include/vm', 'product/include/modules',
                 'product/modules']
for d in required_dirs:
    tools.mkdir_safe(d)

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
clib_objs = tools.cons_objs(clib_srcs, clib_hs)

###############################################################################
# RIPE TOOLS
###############################################################################

tools.cons_yacc('ripe/parser.c', 'ripe/parser.y',
          ['ripe/ripe.h'] + clib_hs)
tools.cons_flex('ripe/scanner.c', 'ripe/scanner.l',
          ['ripe/parser.h'] + clib_hs)
ripe_hs = [
            'ripe/ripe.h',
            'ripe/parser.h',
            'ripe/scanner.h'
          ]
ripe_srcs = [
               'ripe/ripe.c',
               'ripe/dump.c',
               'ripe/cli.c',
               'ripe/build-tree.c',
               'ripe/parser.c',
               'ripe/scanner.c',
               'ripe/astnode.c',
               'ripe/operator.c',
               'ripe/generator.c'
             ]
ripe_objs = tools.cons_objs(ripe_srcs, ripe_hs + clib_hs)
tools.cons_bin('product/ripe', ripe_objs + clib_objs, [])
conf['RIPE'] = 'product/ripe'

###############################################################################
# VM
###############################################################################

func_gen_objs = tools.cons_objs(['vm/func-gen/func-gen.c'], [])
tools.cons_bin('bin/func-gen', func_gen_objs, [])
tools.cons_gen('bin/func-gen', 'vm/func-generated.c', 'c')
tools.cons_gen('bin/func-gen', 'vm/func-generated.h', 'h')

ops_gen_objs = tools.cons_objs(['vm/ops-gen/ops-gen.c'], [])
tools.cons_bin('bin/ops-gen', ops_gen_objs, [])
tools.cons_gen('bin/ops-gen', 'vm/ops-generated.c', 'c')
tools.cons_gen('bin/ops-gen', 'vm/ops-generated.h', 'h')

vm_hs = [
          'vm/vm.h',
          'vm/value_inline.c',
          'vm/ops-generated.h',
          'vm/func-generated.h',
        ]

# VM
vm_srcs = [
            'vm/vm.c',
            'vm/common.c',
            'vm/sym-table.c',
            'vm/ops.c',
            'vm/ops-generated.c',
            'vm/util.c',
            'vm/klass.c',
            'vm/stack.c',
            'vm/format.c',
            'vm/builtin/Object.c',
            'vm/builtin/Function.c',
            'vm/func-generated.c',
            'vm/builtin/HashTable.c',
            'vm/builtin/String.c',
            'vm/builtin/Integer.c',
            'vm/builtin/Double.c',
            'vm/builtin/Arrays.c',
            'vm/builtin/Range.c',
            'vm/builtin/Complex.c',
            'vm/builtin/Tuple.c'
          ]
vm_objs = tools.cons_objs(vm_srcs, vm_hs + clib_hs)

# Construct VM object
tools.link_objs(vm_objs + clib_objs, "product/vm.o")

include_headers = clib_hs + vm_hs + ['modules/modules.h']
for header in include_headers:
    tools.copy_file('product/include/' + header, header)
f = open('product/ripe.conf', 'w')
f.write('cflags=%s\n' % " ".join(conf["CFLAGS"]))
f.write('lflags=%s\n' % " ".join(conf["LFLAGS"]))
f.write('modules=%s\n' % (" ".join(DEF_MODULES)))
f.close()

##############################################################################
# BUILD MODULES
##############################################################################

module_deps = vm_hs + clib_hs + ['modules/modules.h', 'product/ripe']
failed_modules = []

def cons_module(src, dest, module, required, extra_CFLAGS=''):
    if tools.depends(dest, module_deps + [src]):
        tools.pprint('MOD', src, dest)
        args = ['product/ripe', '-n', module, '-c', src, '-o', dest,
                        '-f', '"%s"' % extra_CFLAGS]
        if conf["VERBOSITY"] > 1:
            args.append('-v')
        if required:
            tools.call(args)
        else:
            if not tools.try_call(args):
                failed_modules.append(module)

def build_module(module, required):
    import os.path
    tools.mkdir_safe('product/modules/%s' % module)
    out = 'product/modules/%s/%s.o' % (module, module)

    # Deal with metafile
    metafile = 'modules/%s/%s.meta' % (module, module)
    if os.path.exists(metafile):
        tools.copy_file('product/modules/%s/%s.meta' % (module, module), metafile)
    meta = tools.load_meta(metafile)
    extra_CFLAGS = ''
    if meta.has_key('includes'):
        extra_CFLAGS = meta['includes']

    if meta.has_key('builder'):
        builder = meta['builder']
        tools.call([builder], conf)
    else:
        path = 'modules/%s/%s.rip' % (module, module)
        if os.path.exists(path):
            cons_module(path, out, module, required, extra_CFLAGS)
            return
        else:
            path = 'modules/%s/%s.c' % (module, module)
            if os.path.exists(path):
                cons_module(path, out, module, required, extra_CFLAGS)
                return
            else:
                failed_modules.append(module)

for module in MODULES:
    build_module(module, True)
for module in OPTIONAL_MODULES:
    build_module(module, False)

if len(failed_modules) > 0:
    print("WARNING: Failed building optional module(s): %s" % ", ".join(failed_modules))
