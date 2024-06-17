#! /usr/bin/python3

import numpy as np

target_w=2
stations_w=1
targets=( np.array(( target_w/2, target_w/2, target_w/2)), np.array((-target_w/2, target_w/2, target_w/2)), np.array((-target_w/2,-target_w/2, target_w/2)), np.array(( target_w/2,-target_w/2, target_w/2)), 
          np.array(( target_w/2, target_w/2, 0         )), np.array((-target_w/2, target_w/2, 0         )), np.array((-target_w/2,-target_w/2, 0         )), np.array(( target_w/2,-target_w/2, 0         )),
          np.array(( target_w/2, target_w/2,-target_w/2)), np.array((-target_w/2, target_w/2,-target_w/2)), np.array((-target_w/2,-target_w/2,-target_w/2)), np.array(( target_w/2,-target_w/2,-target_w/2)),
          np.array(( 0         , target_w/2, target_w/2)), np.array((-target_w/2, 0         , target_w/2)), np.array(( 0         ,-target_w/2, target_w/2)), np.array(( target_w/2, 0         , target_w/2)), 
          np.array(( 0         , target_w/2, 0         )), np.array((-target_w/2, 0         , 0         )), np.array(( 0         ,-target_w/2, 0         )), np.array(( target_w/2, 0         , 0         )),
          np.array(( 0         , target_w/2,-target_w/2)), np.array((-target_w/2, 0         ,-target_w/2)), np.array(( 0         ,-target_w/2,-target_w/2)), np.array(( target_w/2, 0         ,-target_w/2)))
stations=( np.array(( 0           , stations_w/2, 0)), np.array((-stations_w/2, 0           , 0)), np.array(( 0           ,-stations_w/2, 0)), np.array(( stations_w/2, 0           , 0)) )
stations_dev=( (0,0), (10,0), (0,-20), (-30,8) )
basc_dev = (-30,8)

"""xi=0.01*np.pi/180/3600
RotN = np.array(( (1, 0, 0,),
		  (0, np.cos(xi), -np.sin(xi)),
                  (0, np.sin(xi),  np.cos(xi)) ))
eta=0.01*np.pi/180/3600
RotE = np.array(( (np.cos(eta), 0, -np.sin(eta)),
		  (0, 1, 0),
                  (np.sin(eta), 0,  np.cos(eta)) ))"""

cor_file = open("coord.cor",'w')
cor_file.write("*gen by prepare.py\n")
cor_file.write("*stations vertical deviation: "+str(stations_dev)+"\n")
for i in range(len(targets)):
  cor_file.write("1 T%i %f %f %f 0.000001 0.000001 0.000001\n"%(i,targets[i][0],targets[i][1],targets[i][2]))
for i in range(len(stations)):
  cor_file.write("1 S%i %f %f %f 0.000001 0.000001 0.000001 %f %f\n"%(i,stations[i][0],stations[i][1],stations[i][2],stations_dev[i][0],stations_dev[i][1]))
cor_file.write("1 basc 0 0 0 0.000001 0.000001 0.000001 %f %f\n"%basc_dev)
cor_file.close()


obs_file = open("obs.obs",'w')
for i in range(len(stations)):
  st=stations[i]
  eta=stations_dev[i][0]*np.pi/180/3600
  xi=stations_dev[i][1]*np.pi/180/3600
  RotN = np.array(( (1, 0, 0,),
                    (0, np.cos(xi), -np.sin(xi)),
                    (0, np.sin(xi),  np.cos(xi)) ))
  RotE = np.array(( (np.cos(eta), 0, -np.sin(eta)),
                    (0, 1, 0),
                    (np.sin(eta), 0,  np.cos(eta)) ))
  R=RotE.dot(RotN)
  for j in range(len(targets)):
    t_rot=R.dot(targets[j]-st)+st
    print("S%i T%i "%(i,j),st,targets[j]," => ",t_rot)
    dist = np.linalg.norm(t_rot-st)
    hz = 200/np.pi*np.arctan2(t_rot[0] - stations[i][0], t_rot[1] - stations[i][1])
    zen = 200/np.pi*np.arccos( (t_rot[2] - stations[i][2]) / dist)
    obs_file.write("3 S%i T%i %f 0.0001 0 0 0\n"%(i,j,dist))
    obs_file.write("5 S%i T%i %f 0.0001 0 0 0\n"%(i,j,hz))
    obs_file.write("6 S%i T%i %f 0.0001 0 0 0\n"%(i,j,zen))
obs_file.write("11 basc @basc.xyz 1\n")
obs_file.close()

xyz_file = open("basc.xyz",'w')
eta=basc_dev[0]*np.pi/180/3600
xi=basc_dev[1]*np.pi/180/3600
RotN = np.array(( (1, 0, 0,),
                (0, np.cos(xi), -np.sin(xi)),
                (0, np.sin(xi),  np.cos(xi)) ))
RotE = np.array(( (np.cos(eta), 0, -np.sin(eta)),
                (0, 1, 0),
                (np.sin(eta), 0,  np.cos(eta)) ))
R=RotE.dot(RotN)
for j in range(len(targets)):
  t_rot=R.dot(targets[j])
  xyz_file.write("T%i %f %f %f 0.00001 0.00001 0.00001\n"%(j,t_rot[0],t_rot[1],t_rot[2]))
xyz_file.close()




