#Load Balancer timer
import time, os, threading, sys

def single_run():
    tm = time.time()
    command = "curl localhost:1234/httpserver > out"
    
    result = os.system(command)
    if result == 0:
        return time.time() - tm
    else:
        return -1

file = open("results."+sys.argv[1], "w+")
def closed_loop():
    count = 0
    while count < 400/int(sys.argv[1]):
        tm = single_run()
        count += 1
        if tm != -1:
            file.write(str(tm) + "\n")
        else:
            file.write(str(-1)+"\n")
            time.sleep(.25)
        time.sleep(.2)

concurrency = int(sys.argv[1])

tm = time.time()
thread_array = []
for i in range(concurrency):
    thread_array.append(threading.Thread(target=closed_loop))
    thread_array[i].start()

for i in range(concurrency):
    thread_array[i].join()
    
print("TOTAL TIME", round(time.time() - tm, 3))
file.close()
#return round(time.time() - tm, 3)



# 2   2.71s user 4.36s system 14% cpu 47.554 total
# 10   3.13s user 5.63s system 48% cpu 17.905 total