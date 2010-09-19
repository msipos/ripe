from subprocess import check_call as ccall
import os, sys

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

def call(args):
    cmd = ' '.join(flatten(args))
    try:
        if verbose == True:
            print(cmd)
        ccall(cmd, shell=True)
    except:
        print("Errors encountered while compiling!")
        sys.exit(1)

def try_call(args):
    cmd = ' '.join(flatten(args))
    try:
        if verbose == True:
            print(cmd)
        ccall(cmd, shell=True)
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
    if forcing == True:
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
    if quiet:
        return
    flag_len = 3
    src_len = 25
    target_len = 25
    flag = "[" + flag + "]"
    if target != None:
        print(" " + pad(flag, flag_len + 3) + pad(src, src_len) + ' -> '
              + pad(target, target_len))
    else:
        print(" " + pad(flag, flag_len + 3) + src)

forcing = False
def set_forcing(force):
    global forcing
    forcing = force

quiet = False
def set_quiet(q):
    global quiet
    quiet = q
def is_quiet():
    return quiet

verbose = False
def set_verbose(v):
    global verbose
    verbose = v
def is_verbose():
    return verbose

def flatten(x):
    # From:  http://kogs-www.informatik.uni-hamburg.de/~meine/python_tricks
    result = []
    for el in x:
        if hasattr(el, "__iter__") and not isinstance(el, basestring):
            result.extend(flatten(el))
        else:
            result.append(el)
    return result
