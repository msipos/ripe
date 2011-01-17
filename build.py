#!/usr/bin/python

# Possible build flags are "force", "quiet", "nodebug", "nogc", "profile"

DATA_TYPES = ['Array1', 'Array2', 'Array3', 'Double', 'Flags', 'Integer', 'Map',
              'Range', 'Set', 'String', 'Tuple']
STDLIB = ['Character', 'DataFormat', 'Err', 'Iterable', 'Math', 'Num', 'Opt',
          'Os', 'Out', 'Path', 'Std', 'Stream', 'Test', 'Template',
          'TextFile', 'Time']
OPTIONAL_MODULES = ['Ast', 'Bio', 'Curl', 'Gd', 'Gsl', 'Json',
                    'Povray', 'Pthread', 'Sci', 'Sdl', 'Speech', 'Xml']
MODULES = DATA_TYPES + STDLIB
DEF_MODULES = DATA_TYPES + STDLIB

#       BUILD SCRIPT FROM HERE ON
import os, sys, tools

conf = tools.conf
conf["CC"] = "gcc"
conf["LD"] = "ld"
conf["CFLAGS"] = ["-Wall", "-Wstrict-aliasing=0", "-Wfatal-errors", "-std=gnu99", "-I.", "-Wno-unused"]
conf["LFLAGS"] = ["-lgc"]
conf["YACC"] = ["bison", "--warnings=all", "-v"]
conf["LEX"] = "flex"
conf["VERBOSITY"] = 1
if "valgrind" in sys.argv:
    conf["VALGRIND"] = ["valgrind", "--leak-check=no", "--track-origins=yes"]
else:
    conf["VALGRIND"] = []
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
if "nostack" in sys.argv:
    conf["CFLAGS"].append("-DNOSTACK")
if "nothreads" in sys.argv:
    conf["CFLAGS"].append("-DNOTHREADS")
if "memlog" in sys.argv:
    conf["CFLAGS"].append("-DMEMLOG")

# Construct required directories
required_dirs = ['bin', 'product', 'product/include', 'product/include/clib',
                 'product/include/vm', 'product/include/modules',
                 'product/include/lang', 'product/modules']
for d in required_dirs:
    tools.mkdir_safe(d)

###############################################################################
# CLIB

clib_hs =   [
              'clib/clib.h'
            ]
clib_srcs = [
              'clib/array.c',
              'clib/dict.c',
              'clib/hash.c',
              'clib/mem.c',
              'clib/path.c',
              'clib/stringbuf.c',
              'clib/utf8.c',
            ]
clib_objs = tools.cons_objs(clib_srcs, clib_hs)

###############################################################################
# LANG

tools.cons_yacc('lang/parser.c', 'lang/parser.y',
          ['lang/lang.h'] + clib_hs)
tools.cons_flex('lang/scanner.c', 'lang/scanner.l',
          ['lang/parser.h', 'lang/lang.h'] + clib_hs)

lang_hs = [
            'lang/lang.h',
            'lang/parser.h',
            'lang/scanner.h',
          ]
lang_srcs = [
              'lang/astnode.c',
              'lang/build-tree.c',
              'lang/input.c',
              'lang/scanner.c',
              'lang/parser.c',
            ]
lang_objs = tools.cons_objs(lang_srcs, lang_hs + clib_hs)
# Construct VM object
tools.link_objs(lang_objs, "lang/lang.o")

###############################################################################
# RIPE TOOLS

ripe_hs = [
            'ripe/ripe.h',
          ]
ripe_srcs = [
               'ripe/ast.c',
               'ripe/cli.c',
               'ripe/dump.c',
               'ripe/error.c',
               'ripe/generator.c',
               'ripe/operator.c',
               'ripe/ripe.c',
               'ripe/typer.c',
               'ripe/util.c',
               'ripe/vars.c'
             ]
ripe_objs = tools.cons_objs(ripe_srcs, ripe_hs + clib_hs + lang_hs)
tools.cons_bin('bin/ripeboot', ripe_objs + clib_objs + lang_objs, [])

###############################################################################
# VM

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
            'vm/builtin/Tuple.c'
          ]
vm_objs = tools.cons_objs(vm_srcs, vm_hs + clib_hs)

# Construct VM object
tools.link_objs(vm_objs + clib_objs, "product/vm.o")

include_headers = clib_hs + vm_hs + ['modules/modules.h', 'lang/lang.h']
for header in include_headers:
    tools.copy_file('product/include/' + header, header)
f = open('product/ripe.meta', 'w')
f.write('cflags=%s\n' % " ".join(conf["CFLAGS"]))
f.write('lflags=%s\n' % " ".join(conf["LFLAGS"]))
f.write('modules=%s\n' % (" ".join(DEF_MODULES)))
f.close()

##############################################################################
# BOOTSTRAP STEP 1

riperipesrcs = [
                'riperipe/dump.rip',
                'riperipe/eval.rip',
                'riperipe/generator.rip',
                'riperipe/main.rip',
                'riperipe/module.rip',
                'riperipe/namespace.rip',
                'riperipe/typer.rip',
               ]

bootstrap_srcs = ['product/vm.o', 'lang/lang.o', 'product/ripe.meta']
bootstrap_srcs.extend(riperipesrcs)
for module in DEF_MODULES:
    name = 'modules/%s/%s.meta' % (module, module)
    if os.path.exists(name):
        bootstrap_srcs.append(name)
    name = 'modules/%s/%s.rip' % (module, module)
    bootstrap_srcs.append(name)
bootstrap_srcs.append('modules/Ast/Ast.rip')
bootstrap_srcs.append('modules/Json/Json.rip')

tools.cons_obj('riperipe/bootstrap.o', 'riperipe/bootstrap.c', [])
bootstrap_srcs.append('riperipe/bootstrap.o')

##############################################################################
# BOOTSTRAP
def bootstrap(newripe, oldripe):
    if tools.depends(newripe, [oldripe]):
        tools.pprint('RIP', 'riperipe/*.rip', newripe)
        tools.call([conf["VALGRIND"], oldripe,
                   '-s', '-o', newripe, bootstrap_srcs])

bootstrap('bin/ripe2', 'bin/ripeboot')
bootstrap('bin/ripe3', 'bin/ripe2')
bootstrap('product/ripe', 'bin/ripe3')
conf['RIPE'] = 'product/ripe'

##############################################################################
# TYPE MODULES

type_deps = ['product/ripe']
def type_module(module):
    path = 'modules/%s/%s.rip' % (module, module)
    out = 'product/modules/%s/%s.typ' % (module, module)
    if tools.depends(out, type_deps + [path]):
        sys.stdout.write(module + " ")
        tools.mkdir_safe('product/modules/%s' % module)
        tools.call(['product/ripe', '-t', path, '>', out])
    return out

sys.stdout.write("Generating module types...")
type_infos = []
for module in MODULES + OPTIONAL_MODULES:
    type_infos.append(type_module(module))
sys.stdout.write("\n")

##############################################################################
# BUILD MODULES

module_deps = vm_hs + clib_hs + ['modules/modules.h', 'product/ripe'] + type_infos
failed_modules = []

def build_module(module, required):
    import os.path
    tools.mkdir_safe('product/modules/%s' % module)
    out = 'product/modules/%s/%s.o' % (module, module)
    #srcs = [type_infos, 'product/ripe.meta']
    srcs = []

    metafile = 'modules/%s/%s.meta' % (module, module)
    if os.path.exists(metafile):
        tools.copy_file('product/modules/%s/%s.meta' % (module, module), metafile)
        srcs.append(metafile)
    meta = tools.load_meta(metafile)
    extra_objs = ''
    if 'objs' in meta:
        extra_objs = meta['objs']
    src = 'modules/%s/%s.rip' % (module, module)
    if tools.depends(out, module_deps + [src]):
        tools.pprint('MOD', src, out)
        args = ['product/ripe', '-n', module,
                '-c', srcs, src, extra_objs, '-o', out]
        if conf["VERBOSITY"] > 1:
            args.append('-v')
        if required:
            tools.call(args)
        else:
            if not tools.try_call(args):
                failed_modules.append(module)

for module in MODULES:
    build_module(module, True)

if "nomods" not in sys.argv:
    for module in OPTIONAL_MODULES:
        build_module(module, False)
    if len(failed_modules) > 0:
        print("WARNING: Failed building optional module(s): %s" % ", ".join(failed_modules))

if "doc" in sys.argv:
  tools.call(["ripedoc/build.sh", "2>", "/dev/null"])
  tools.call(["ripedoc/ripedoc", "."])
  tools.call(["mv", "*.html", "doc/"])
