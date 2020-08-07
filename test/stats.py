#Load Balancer timer
import time, os, threading, sys

if(len(sys.argv) > 1):
    if sys.argv[1] == "clean":
        for i in range(0, 13):
            if os.path.exists("results."+ str(i)):
                os.remove("results."+ str(i))
        exit()

class process_test():
    
    def __init__(self):
        self.file = ""
        
    def single_run(self):
        tm = time.time()
        command = "curl localhost:1234/httpserver > out"
        
        result = os.system(command)
        if result == 0:
            return time.time() - tm
        else:
            return -1

    
    def closed_loop(self, concurrency):
        count = 0
        while count < 200/int(concurrency):
            tm = self.single_run()
            count += 1
            if tm != -1:
                self.file.write(str(tm) + "\n")
            else:
                self.file.write(str(-1)+"\n")
                time.sleep(.25)
            time.sleep(.2)

    def test(self, concurrency):
        tm = time.time()
        thread_array = []
        self.file = open("results."+str(concurrency), "w+")
        for i in range(concurrency):
            thread_array.append(threading.Thread(target=self.closed_loop, args = [concurrency]))
            thread_array[i].start()

        for i in range(concurrency):
            thread_array[i].join()
            
        #print("TOTAL TIME", round(time.time() - tm, 3))
        self.file.close()
        return 200/round(time.time() - tm, 3)
    

process = process_test()
testlist = [2, 4, 6, 8, 10, 12, 14]
result = []
for i in testlist:
    array = []
    array.append(process.test(i))
    file = open("results."+str(i), "r")
    lines = file.readlines()
    arr = []
    for line in lines:
        line = line.strip()
        arr.append(float(line))
    if(len(arr) == 0):
        array.append(0)
        array.append(0)
    else:
        array.append(sum(arr)/len(arr))
        array.append(max(arr))
    result.append(array)
    file.close()

results = open("results", "w+")
for i in range(len(result)):
    print(testlist[i],": { Throughput:", result[i][0], "average latency:", result[i][1], "max latency:",result[i][2], "}")
    results.write(str(testlist[i]) +" "+ str(result[i][0])+" "+str(result[i][1]) +" " +  str(result[i][2]) + "\n")
