import sys

data = sys.stdin.readlines()
best_seq_time = -1

for line in data:
    line = line.strip()
    
    toks = list(float(x) for x in line.split(','))

    if toks[0] == 1: 
        best_seq_time = min(x for x in toks if x != 1)
    
    print(toks[0], end="")
    for i in range(1, len(toks)):
        toks[i] = best_seq_time / toks[i]
        print(',{:f}'.format(toks[i]), end="")
    print("")
