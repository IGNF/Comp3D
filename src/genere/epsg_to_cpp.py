#!/usr/bin/python3
# -*- coding: utf-8 -*-

import re

authorized_proj=("lcc","merc","stere","sterea","tmerc","utm")

class Projection(object):
	def __init__(self, line1,line2):
		#1st line: # name
		#2nd line: <code> definition
		self.name=re.sub(r"# *(.*)$", r"\1", line1)
		self.code=int(re.sub(r"<(.*)> .*", r"\1", line2))
		self.definition=re.sub(r"[^>]+> *(.*)$", r"\1", line2)
		self.type="??"
		self.type=re.sub(r".*\+proj=([^ ]+) .*", r"\1", line2)

fname="epsg"
with open(fname) as f:
    content = f.readlines()

content = [x.strip() for x in content] 

all_proj=[]
for i in range(len(content)-1):
    line1=content[i]
    line2=content[i+1]
    if (line1[0]=="#")and(line2[0]!="#"):
    	all_proj.append(Projection(line1,line2))

print("std::vector<EPSGproj> Projection::allEPSG={")
for p in all_proj:
	if (p.type in authorized_proj):
		if (not "height" in p.name):
			if (not "+vunits=" in p.definition):
				if (not "deprecated" in p.name):
					if ("+units=m" in p.definition):
						print('    {%d,"%s","%s"},'%(p.code,p.name,p.definition))

print("};")
