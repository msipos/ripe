from subprocess import check_call as ccall
import os, sys

conf = {}

def link_objs(objs, output):
    if depends(output, objs):
        arr = flatten([conf["LD"], '-r', objs, '-o', output])
        pprint('LD', output)
        call(arr)

def cons_obj(target, src, deps):
    if depends(target, deps + [src]):
        pprint('CC', src, target)
        call([conf["CC"],
                    conf["CFLAGS"], '-c', src, '-o', target])

def cons_yacc(target, src, deps):
    if depends(target, deps + [src]):
        pprint("YAC", src, target)
        call([conf["YACC"], src])

def cons_flex(target, src, deps):
    if depends(target, deps + [src]):
        pprint("LEX", src, target)
        call([conf["LEX"], src])

def cons_bin(target, objs, deps):
    if depends(target, deps + objs):
        pprint('BIN', target)
        call([conf["CC"],
                    conf["CFLAGS"],
                    conf["LFLAGS"], objs, '-o', target])

def cons_gen(gen_program, target, t):
    if depends(target, [gen_program]):
        pprint('GEN', gen_program, target)
        call([gen_program, t, '>', target])

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
    if depends(dest, [src]):
        # pprint('CP', src, dest)
        shutil.copy(src, dest)

def load_meta(filename):
    if not os.path.exists(filename):
        return {}
    hsh = {}
    for line in open(filename):
        arr = line.split('=')
        if len(arr) == 2:
            key = arr[0].strip()
            value = arr[1].strip()
            hsh[key] = value
    return hsh

def to_env(env):
    m = {}
    for k, v in env.items():
        if isinstance(v, list):
            m[k] = " ".join(v)
        else:
            m[k] = str(v)
    return m

def call(args, env = None):
    cmd = ' '.join(flatten(args))
    if env != None:
        env = to_env(env)
    try:
        if conf["VERBOSITY"] > 1:
            print(cmd)
        ccall(cmd, shell=True, env = env)
    except:
        print("Errors encountered while compiling!")
        sys.exit(1)

def try_call(args, env = None):
    cmd = ' '.join(flatten(args))
    if env != None:
        env = to_env(env)
    try:
        if conf["VERBOSITY"] > 1:
            print(cmd)
        ccall(cmd, shell=True, env=env)
        return True
    except:
        return False

def mkdir_safe(path):
    if not os.path.exists(path):
        os.mkdir(path)

def delete_safe(path):
    try:
        os.remove(path)
    except:
        pass

# Return True if target should be rebuilt
def depends(target, dependencies):
    if conf["FORCING"]:
       return True

    ttime = get_stamp(target)
    if ttime == None:
        return True
    for dependency in dependencies:
        dtime = get_stamp(dependency)
        if dtime > ttime:
            return True
    return False

def get_stamp(path):
    try:
        return os.path.getmtime(path)
    except:
        return None

def pad(s, ln):
    if len(s) < ln:
        return s + " " * (ln - len(s))
    return s

def pprint(flag, src, target = None):
    if conf["VERBOSITY"] < 1:
        return
    flag_len = 3
    src_len = 30
    target_len = 30
    flag = "[" + flag + "]"
    if target != None:
        print(" " + pad(flag, flag_len + 3) + pad(src, src_len) + ' -> '
              + pad(target, target_len))
    else:
        print(" " + pad(flag, flag_len + 3) + src)

def flatten(x):
    # From:  http://kogs-www.informatik.uni-hamburg.de/~meine/python_tricks
    result = []
    for el in x:
        if hasattr(el, "__iter__") and not isinstance(el, str):
            result.extend(flatten(el))
        else:
            result.append(el)
    return result
