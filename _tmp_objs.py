import os, glob

objs = sorted(os.path.basename(p) for p in glob.glob(r'e:/automaton/obj/*.obj'))
print('obj dir count:', len(objs))
for o in objs:
    if any(k in o.lower() for k in ('wave', 'scatter', 'attractor', 'voxel', 'initsim', 'simulation', 'interaction', 'utils', 'globals', 'geometry', 'polar', 'charges', 'config')):
        print('  ', o)
print('--- build scripts / batch mentioning tests ---')
for p in glob.glob(r'e:/automaton/*.bat') + glob.glob(r'e:/automaton/*.ps1') + glob.glob(r'e:/automaton/tests/*.bat'):
    t = open(p, encoding='utf-8', errors='replace').read()
    if 'scatter' in t.lower() or 'attractor' in t.lower() or 'voxel' in t.lower() or 'test' in t.lower():
        print('  ', p)
