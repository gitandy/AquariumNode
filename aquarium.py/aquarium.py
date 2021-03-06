'''
Created on 15.06.2013

@author: andreas
'''

import csv
import time
import struct
import threading

class AquariumControler(threading.Thread):
    def __init__(self, cntrlr, peer_id):
        threading.Thread.__init__(self)
        
        self._cntrlr = cntrlr
        
        self._peer_id = peer_id
                
    def setControlerParams(self, temp=25, gain_prop=1, gain_int=1, min_fan_dc=0.3):
        if temp >= 0 and temp <= 50:
            self._temp_sp = temp
        else:
            raise Exception('Temperature out of range')

        if gain_prop > 0 and gain_prop < 1:
            self._gain_prop = gain_prop
        else:
            raise Exception('Proportional gain out of range')
        
        if gain_int > 0 and gain_int < 1:
            self._gain_int = gain_int
        else:
            raise Exception('Integral gain out of range')

        if min_fan_dc > 0 and min_fan_dc < 1:
            self._min_fan_dc = min_fan_dc
        else:
            raise Exception('Minimal fan duty cycle out of range')
        
        self._cntrlr.send(self._peer_id, [int(self._temp_sp*5), 
                        int(self._gain_prop*100), 
                        int(self._gain_int*100),
                        int(self._min_fan_dc*100)])
    
    def capture(self, csv_file=None):
        self.csv_file = csv_file
        
        if not self.csv_file == None:
            self._csv_file = open(csv_file, 'wb')
            self._csv_writer = csv.writer(self._csv_file, 
                                          delimiter=';',
                                          quotechar='"', 
                                          quoting=csv.QUOTE_MINIMAL)
            self._csv_writer.writerow(['Date/Time', 
                                       'Temp. set point', 
                                       'Temp. current', 
                                       'Fan speed'])
                
        self._cntrlr.recv_start()
        
        self.start()
        
    def run(self):
        while True:
            if self._cntrlr.recv_available() == True:
                data = self._cntrlr.recv_line()
                
                if data[1] == self._peer_id:
                    dstr = ''
                    for b in data[2:]:
                        dstr += chr(b)

                    temp_sp, temp_pv, fan_spd = struct.unpack('<BfH', dstr)
                    
                    print('Temp: %.1fC, Fan speed: %i' %(temp_pv, fan_spd))
                 
                    if not self.csv_file == None:
                        self._csv_writer.writerow([data[0], 
                                                   temp_sp/5.0, 
                                                   temp_pv, 
                                                   fan_spd])
                        self._csv_file.flush()
                    
            time.sleep(.5)
        
        self._cntrlr.recv_stop()
    
    def __del__(self):
        if self._csv_file:
            self._csv_file.close()
        

if __name__ == '__main__':
    remote = False
    cntrlr = None
    
    if remote == True:
        import jlrcontroler
        cntrlr = jlrcontroler.JeeLinkRemoteControler('tcp://localhost:5556', 'tcp://localhost:5557')
    else:
        import jlcontroler
        cntrlr = jlcontroler.JeeLinkControler('/dev/ttyUSB1', node_id=0x01, group_id=0x01)
    
    ac = AquariumControler(cntrlr, 0x02)
    
    #Wait for JeeLink to get ready
    time.sleep(3)
    ac.setControlerParams(25, 0.2, 0.1, 0.2)
    
    ac.capture('capture.csv')
   
    
    
        