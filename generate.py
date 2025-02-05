import random

f = open("in.txt", "w")

n = int(input())

f.write(str(n) + "\n");

for i in range(n):
    for j in range(n):
        v = random.randint(-100, 100)
        f.write(str(v) + " ");
    f.write("\n")

f.close()
