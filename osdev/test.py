import random
import numpy as np

'''
max_order = 13

orders = np.array([i for i in range(max_order + 1)])
arr = np.array([2 ** i for i in range(max_order + 1)])
prob = 1 / arr
prob = prob / np.sum(prob)

print(arr)
print(prob)

total = []
current_alloc = []
left = 2 ** max_order
while left > 0:
    c = np.random.choice(orders, 1, p=prob)[0]
    print(c)
    if 2 ** c >= left:
        o = int(np.log2(left))
        left -= (2 ** o)
        total.append(o)
        current_alloc.append(o)
    else:
        left -= (2 ** c)
        total.append(c)
        current_alloc.append(c)

alloc = np.power(2, total)
print(np.sum(alloc))
print(total)
'''

total_num = 1000
arr = [i for i in range(total_num)]
arr = arr + arr
np.random.shuffle(arr)
print(arr)
