'''
Created on Jul 27, 2018

@author: florian
'''
from threading import Thread
import  queue  
        
        
class Watcher:
     
    
     
    def worker(self):
        while True:
            item = self.q.get()
            if item is None:
                break
            print ("Worker")
            item()
            self.q.task_done() 

    def __init__(self): 
        
        self.q = queue.Queue()
        self.num_worker_threads = 7
        self.threads = []
        for _ in range(self.num_worker_threads):
            t = Thread(target=self.worker)
            t.start()
            self.threads.append(t)

    def addWork(self, config):
        self.q.put(config)
         
    def stop(self):  
        self.q.join()

        # stop workers
        for _ in range(self.num_worker_threads):
            self.q.put(None)
        for t in self.threads:
            t.join()

