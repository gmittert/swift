import sys
#import os

# map section to size
x = {}

#for subdir, dirs, files in os.walk(rootdir):
#    for file in files:
#      if file.endswith("Framework-section"):

# format: Section (xx, yy): zz
with open(sys.argv[1]) as f2:
    for line in f2:
         tokens = line.split()
         if len(tokens) < 4:
           continue;
         if tokens[0] == "Section":
           combinedname = tokens[1] + tokens[2]
           if combinedname not in x:
             x[combinedname] = int(tokens[3])
           else:
             x[combinedname] = x[combinedname] + int(tokens[3])
for key, value in x.items():
  print(key, ' ', value)
