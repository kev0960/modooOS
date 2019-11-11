total = 0
cur = 1
while total < 50 * 1024 * 1024:
    print (str(cur)+ ",", end='')
    total += (len(str(cur))+1)
    cur += 1
