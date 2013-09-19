import numpy, pylab, sys

for arg in sys.argv[1:]:
    data = numpy.loadtxt(arg)
    if len(data[0,:]) == 3:
        t = data[:,0]
        x = data[:,1]
        y = data[:,2]
        pylab.plot(t, (x + y)/2.0, 'x-', label = arg)
        pylab.xlabel('time')
        pylab.ylabel('<x^2>')
    else:
        t = data[:,0]
        r = data[:,1]
        pylab.plot(t, r, 'x-', label = arg)
        pylab.xlabel('time')
        pylab.ylabel('fluctuations^2')

pylab.legend(loc='upper left')
pylab.show()
