#!/usr/bin/python3

import math
import numpy as np
import random

axis_name="a"

pos1=(np.array((50,30,10)),np.array((53,35,15)),np.array((58,28,12)))

n=np.array((1,0.6,0.3))
n=n/n[0]

Or=np.array((50,25,13))

angles_deg=(10,15,10,10)

sigma=0.001

def abc2R(a,b,c,theta):
    cost1 = 1-math.cos(theta);
    sint  =   math.sin(theta);
    d     = math.sqrt(a*a+b*b+c*c);
    u     = a/d;
    v     = b/d;
    w     = c/d;
    R = np.zeros( (3, 3) )
    R[0][0]= 1-(1-u*u)*cost1;
    R[0][1]=  -w*sint+u*v*cost1;
    R[0][2]=   v*sint+u*w*cost1;
    R[1][0]=   w*sint+u*v*cost1;
    R[1][1]= 1-(1-v*v)*cost1;
    R[1][2]=  -u*sint+v*w*cost1;
    R[2][0]=  -v*sint+u*w*cost1;
    R[2][1]=   u*sint+v*w*cost1;
    R[2][2]= 1-(1-w*w)*cost1;
    return R


def coord2cor(name,coord,sigma):
    #splat operator : *
    return "1 {} {} {} {} {} {} {}".format(
        name,coord[0]+random.gauss(0,sigma),coord[1]+random.gauss(0,sigma),coord[2]+random.gauss(0,sigma),sigma,sigma,sigma)

print("COR :")
print("*n={}, Or={}".format(n,Or))
ang_total=0
for j,ang in enumerate(angles_deg):
    ang_total=ang_total+ang
    R=abc2R(*n,ang_total*math.pi/180)
    for i,pt in enumerate(pos1):
        pt_bis=R.dot(pt-Or)+Or
        name="{}_{}_{}".format(axis_name,i+1,j+1)
        print(coord2cor(name,pt_bis,sigma))

print("axis:")
print("*cible pos   pt sigma_rayon  sigma_perp")
for j,ang in enumerate(angles_deg):
    for i,pt in enumerate(pos1):
        name="{}_{}_{}".format(axis_name,i+1,j+1)
        print(i+1,j+1,name,sigma,sigma)


# ~ text_file = open("coord.cor", "w")
# ~ text_file.write()
# ~ text_file.close()
