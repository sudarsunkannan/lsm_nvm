import xml.etree.ElementTree as ET
import subprocess
import os, datetime
import re
from subprocess import Popen, PIPE


DBDIR=os.environ['TEST_TMPDIR']
DBBENCH=os.environ['DBBENCH']
APP=DBBENCH + "/db_bench"
SOURCE=os.environ['NOVELSMSRC']
SCRIPTS=os.environ['NOVELSMSCRIPT']
INFILE=os.environ['INPUTXML']
tree = ET.parse(INFILE)
root = tree.getroot()


def cleandb():
    #print "rm -rf " + DBDIR    
    os.system("rm -rf " + DBDIR + "/*")    


def makedb():
    cleandb();
    os.chdir(SOURCE)
    os.system("make -j4")
    os.chdir(DBBENCH)
    APP = DBBENCH + "/db_bench"


class prettyfloat(float):
    def __repr__(self):
        return "%0.2f" % self


class stats(object):
  
  def __init__(self):
      self.init = 1


  def print_bwlat(self, header, bwidth_set, lat_set, f):
      print "****************************"
      print "Header size " + str(header)
      print "[%s]"%", ".join(map(str,bwidth_set))
      print "[%s]"%", ".join(map(str,lat_set))
      f.write("[%s]\n"%", ".join(map(str,bwidth_set)))
      f.write("[%s]\n"%", ".join(map(str,lat_set)))
      print "****************************\n"
      f.write("\n");


class system(object):

  def __init__(self):
      self.init = 1
      self.diskspace = 0
      self.system_schema = root.find('./system-main')


  def cleandb(self):
     os.system("rm -rf " + self.dbdir + "/*")    


  def makedb(self):
     cleandb();    
     os.system("cd .. && make -j4 && cs scripts")    


  def set_dbdir(self):
      self.dbdir = self.system_schema.find('dbdir').text
      os.environ['TEST_TMPDIR'] = self.dbdir
      print "Database in " + self.dbdir


  def get_diskspace(self, root):
      system_schema = root.find('./system-main')
      partition = system_schema.find('partition').text 
      s = os.statvfs(partition) 
      self.diskspace = (s.f_bavail * s.f_frsize)
      return self.diskspace


  def fitto_diskspace(self, elements, key, value, logspace):  
      usage = ((value + key) * elements) + logspace
      if( usage > self.diskspace):     
          diff = usage - self.diskspace 
          maxele = elements - (diff/(key+value))
          return maxele 
      else:
          return elements


############# Check the tests that are enabled #############
benchmarks = []
for child in root.findall('benchmarks'):
    for subchild in child:
        benchmarks.append(subchild.text)


bench_str = "--benchmarks=" + benchmarks[0]
for i in range(1, len(benchmarks)):
    bench_str = bench_str + "," + benchmarks[i]


class ParamTest:

    seed_count = 0
    num_tests = 0
    num_elements = 0
    value_size = 0
    dram_size = 0
    nvm_size = 0
    levels = 0
    rdthrds = 0
    
    output = " "    
    resarr = []    
    xincr = 0
    xmanual = []
    xlegend = []

    #def __init__(self):

    def setvals(self, params):    

        self.seed_count = params.find('seed-count').text
        self.num_tests = params.find('num-tests').text
        
        self.num_vals = params.find('num-elements').text
        self.value_size = params.find('value-size').text
        self.thrdcnt = params.find('thread-count').text
        self.dram_size = params.find('DRAM-mem').text
        self.nvm_size = params.find('NVM-mem').text
        self.levels = params.find('memory-levels').text
        self.rdthrds = params.find('num-readthreads').text
        self.numtests = params.find('num-tests').text

        self.num_str = "--num=" + self.num_vals  
        self.val_str = "--value_size=" + self.value_size
        self.thrd_str = "--threads=" + self.thrdcnt
        self.dramsz_str = "--write_buffer_size=" + str(int(self.dram_size))
        self.nvmsz_str = "--nvm_buffer_size=" + str(int(self.nvm_size))
        self.levels_str = "--num_levels=" + str(self.levels)
        self.rdthrds_str = "--num_read_threads=" + str(self.rdthrds) 
        self.xincr = 1 #int(self.seed_count)


    def runapp(self, APP, index):

        bwidth_set = [[] for x in range(int(self.rdthrds)+1)]
        i = 0
        x_values = []

	#Clean the exisiting database; we don't want to read old database
	cleandb();
	print bench_str +" "+ self.num_str +" "+ self.val_str +" "+ \
	      self.thrd_str + " " + self.dramsz_str +" "+ self.nvmsz_str \
	      +" "+ self.levels_str +" "+ self.rdthrds_str

	process = Popen([APP, bench_str, self.num_str, self.val_str, \
		  self.thrd_str, self.dramsz_str, \
		  self.nvmsz_str, self.levels_str, self.rdthrds_str], \
		  stdout=PIPE)
	(self.output, err) = process.communicate()
	exit_code = process.wait()
        print self.output

	"""
	for line in self.output.splitlines():
	    if re.search(str(benchmarks[0]), line):
		my_set = line.split();
		if my_set[0] in benchmarks:
		    print  my_set[2] + "\t" +  my_set[4]
	    i = i + 1
        """

    # Vary read thread count from seed_count to rdthrds
    def run_rdthrdcnt_test(self, params):

        for count in range(0, int(self.rdthrds) + int(self.seed_count)):
            self.rdthrds_str = "--num_read_threads=" + str(count)
            self.runapp(APP, count)


    # Vary value size from base value_size to value_size * 2 * num_tests
    def run_valsz_test(self, params):

        valsz=int(self.value_size) 
        num = int(self.num_vals)
     
        for loop in range(0, int(self.numtests)):
            self.val_str = "--value_size=" + str(valsz)
            self.num_str = "--num=" + str(num)
            self.runapp(APP, valsz)
            valsz = valsz * int(self.seed_count) 
            num = num/int(self.seed_count) 


    # Vary value size from base value_size to value_size * 2 * num_tests
    # Vary read thread count from seed_count to rdthrds    
    def run_thrdcnt_valsz_test(self, params):

	for count in range(0, int(self.rdthrds) + 1):

	    self.rdthrds_str = "--num_read_threads=" + str(count)
	    valsz=int(self.value_size) 
	    num=int(self.num_vals)    

	    for loop in range(0, int(self.numtests)):
		self.val_str = "--value_size=" + str(valsz)
		self.num_str = "--num=" + str(num)
		self.runapp(APP, count)
		valsz = valsz * int(self.seed_count) 
		num = num/int(self.seed_count) 


    # Vary num elements (keys) from base num-elements to num-elements * 2 * num_tests
    def run_valcnt_test(self, params):

        count=int(self.num_vals)      
             
        for loop in range(0, int(self.numtests)):
            self.num_str = "--num=" + str(count)
            self.runapp(APP, count)
            count = count * int(self.seed_count) 
            print count;     


    # Vary read thread count from 1 to seed_count + self.levels
    def run_memlevls_test(self, params):

        for count in range(1, int(self.levels) + int(self.seed_count)):
            self.levels_str = "--num_levels=" + str(count)
            self.runapp(APP, count)


def main():

    p = ParamTest()

    rdthrd_test = root.find('./readthread-main')
    is_rdthrd_test = False if int(rdthrd_test.get('enable')) == 0 else True

    valsz_test = root.find('./value-size-main')
    is_valsz_test = False if int(valsz_test.get('enable')) == 0 else True

    valcnt_test = root.find('./num-elements-main')
    is_valcnt_test = False if int(valcnt_test.get('enable')) == 0 else True

    thrdcnt_valsz_test = root.find('./thread-count-main')
    is_thrdcnt_valsz_test = False if int(thrdcnt_valsz_test.get('enable')) \
                     == 0 else True

    if is_rdthrd_test:
        p.setvals(rdthrd_test)
        p.run_rdthrdcnt_test(rdthrd_test)

    if is_valsz_test:
        p.setvals(valsz_test)
        p.run_valsz_test(valsz_test)

    if is_valcnt_test:
        p.setvals(valcnt_test)
        p.run_valcnt_test(valcnt_test)

    if is_thrdcnt_valsz_test:
        p.setvals(thrdcnt_valsz_test)
        p.run_thrdcnt_valsz_test(thrdcnt_valsz_test)

    print " "   

# MAke database 
makedb()
main()
exit()
