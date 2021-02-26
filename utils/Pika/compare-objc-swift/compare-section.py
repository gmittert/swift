import sys
#import os

# map section to size
x = {}

# format: Section size
with open(sys.argv[1]) as f2:
    for line in f2:
         tokens = line.split()
         if len(tokens) < 2:
           continue;
         x[tokens[0]] = int(tokens[1])

y = {}
with open(sys.argv[2]) as f2:
    for line in f2:
         tokens = line.split()
         if len(tokens) < 2:
           continue;
         if tokens[0] not in x:
           x[tokens[0]] = 0
         y[tokens[0]] = int(tokens[1])

for key, value in x.items():
  if key not in y:
    print(key, ' ', value, ' ', 0)
  else:
    print(key, ' ', value, ' ', y[key])
