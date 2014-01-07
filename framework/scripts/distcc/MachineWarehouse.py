# Load the required modules
import os, sys, subprocess, socket, multiprocessing
from random import shuffle
from sets import Set

# Import the Machine and DmakeRC class
from Machine import *
from DmakeRC import *
from DistccDaemon import *

## Helper function for creating machine objects, this is required to be outside
#  of the class to allow the use of running the object creation in parallel
#  @param line A string of comma sperated values containing one line of machine information from server
#  @return A Machine object generated from the given input
def createMachine(line):
  if len(line) == 0:
    return
  return Machine(*line)

## @class MachineWarehouse
#  Creates and controls the local and worker Machine objects, it mainly provides the
#  getHosts() method that returns the needed DISTCC_HOSTS environmental variable for
#  use with 'make'
# @see Machine MachineWarehouse::getHosts
class MachineWarehouse(object):


  ## Class constructor
  #
  #  Optional Arguments:
  #    local = True | {False}     - perform a local build (i.e., no remote machines)
  #    jobs  = <int>              - override the automatic computation of number of jobs for 'make'
  #    buck  = True | {False}     - utilize the test machine (buck) for the list of machines on the s
  #                                 server (passed to DmakeRC)
  #    dedicated = True | {False} - run the machine as a dedicated build box (passed to DmakeRC)
  #    disable = list<str>        - list of hostnames, username, or ip addresses to exclude from DISTCC_HOSTS
  #    allow = list<str>          - list of ip addresses/hostnames to allow in DISTCC_HOSTS
  #    serial = True | {False}    - toggle the parallel creation of the Machine objects
  #    restrict = list<str>       - a list of ip addresses to consider for local build restriction, if the
  #                                 local machine starts with any of the values in this list,
  #                                 perform a local build
  #    allow_off_network = True | {False} - if true, the above restrictions are ignored
  #    no_daemon = True | {False} - set to true to disable the starting of daemons and kill existings
  #
  # @see DmakeRC
  def __init__(self, **kwargs):

    # Build the master machine and set initial values
    self.master = Machine(localhost=True)
    self.workers = []
    self.down = []

    # Flag set to true of machines are built
    self._machines = False

    # Flag if DISTCC_HOSTS line needs to be created
    self._refresh = kwargs.pop('refresh', False)

    # If disable/enable are used a refresh is required
    if kwargs.get('disable', None) != None or kwargs.get('enable', False):
      self._refresh = True

    # Get the optional input
    self._jobs = kwargs.pop('jobs', None)
    self._allow = kwargs.pop('allow', None)
    self._serial = kwargs.pop('serial', False)
    self._no_daemon = kwargs.pop('no_daemon', False)

    # If this is a local build, there is nothing more to do
    self._local = kwargs.pop('local', False)
    if self._local:
      return

    # Check that your IP address against the restricted
    restrict = kwargs.pop('restrict', None)
    if (not kwargs.pop('allow_off_network')) and (restrict != None):
      on_network = False
      for r in restrict:
        if self.master.address.startswith(r):
          on_network = True
          break

      if not on_network:
        print "Your machine (" + self.master.address + ") is off network, performing a local build"
        self._local = True
        return

    # Read the .dmakerc and remote file
    self._dmakerc = DmakeRC(self.master, **kwargs)

    # Check if the remote access failed, if so preform local build
    if self._dmakerc.fail():
      self._local = True
      return

    # Create the Deamon object
    self._daemon = DistccDaemon(self.master, dedicated=kwargs.pop('dedicated', False))

    # Kill daemons
    if self._no_daemon:
      self.killDaemon()


  ## Return the hosts line and number of jobs (public)
  #  @param kwargs Optional keyword/value pairings
  #  @return DISTCC_HOSTS
  #  @return jobs
  #
  #  Optional Arguments:
  #    refresh = True | False   - Force a refresh from the server, defaults to value from constructor
  #    max = True | {False}     - Run the maximum no. of threads (passed to _buildHosts)
  #    localhost = <int>        - Set the DISTCC_HOSTS localhost value (passed to _buildHosts)
  #    localslots = <int>       - Set the DISTCC_HOSTS localslots value (passed to _buildHosts)
  #    localslots_cpp = <int>   - Set the DISTCC_HOSTS localslots_cpp value (passed to _buildHosts)
  #
  #  @see _buildHosts
  def getHosts(self, **kwargs):

    # Gather the options
    self._refresh = kwargs.pop('refresh', self._refresh)

    # Set refresh to true of any of the optional arguments are set
    if kwargs['max'] == True \
       or kwargs.pop('localhost', None) != None \
       or kwargs.pop('localslots', None) != None \
       or kwargs.pop('localslots_cpp', None) != None:
      self._refresh = True

    # Return the local
    if self._local:
      jobs = self.master.threads
      if self._jobs != None:
        jobs = self._jobs
      distcc_hosts = 'localhost/' + str(jobs)
      return distcc_hosts, jobs

    # Check if the IP lists between the local and remote lists are different
    if self._dmakerc._remote_ip != self._dmakerc._local_ip:
      self._refresh = True

    # Build the machine objects (if desired or needed)
    if self._refresh:
      self.startDaemon()
      self._buildHosts(**kwargs)
    else:
      print 'Using cached DISTCC_HOSTS, use --refresh to rebuild'

    # Extract and return the variables
    distcc_hosts = self._dmakerc.get('DISTCC_HOSTS')
    jobs = self._dmakerc.get('JOBS')
    if self._jobs != None:
      jobs = self._jobs
    return distcc_hosts, jobs


  ## Starts the distccd daemons (public)
  # @see DistccDaemon
  def startDaemon(self):

    # Return if no_daemon = True
    if self._no_daemon:
      return

    # The Machine objects are required
    self._buildWorkers()

    # Start the daemon
    self._daemon.start(self.workers + self.down, allow=self._allow)


  ## Kills all distccd processes (public)
  # @see DistccDaemon
  def killDaemon(self):
    self._daemon.kill()


  ## Build the remote Machine objects (private)
  #  Read the list of machines from the HOST_LINES from the server and
  #  build the Machine objects (in parallel)
  #
  #  @see createMachine Machine
  def _buildWorkers(self):

    # Return if the workers are already built
    if self._machines:
      return

    # Handle empty host lines
    host_lines =  self._dmakerc.get('HOST_LINES')
    if len(host_lines) == 0:
      return

    # Create the Machine objects (in parallel)
    output = []
    if not self._serial:
      pool = multiprocessing.Pool(processes=self.master.threads)
      output = pool.map(createMachine, host_lines)
      pool.close()
      pool.join()

    # Create the Machine objects serially
    else:
      for line in host_lines:
        output.append(createMachine(line))
        output[-1].info()

    # Check the disabled list against the address and ip, the user is allowed to
    # supply partial ip/hostnames so loop through each and test that the substring
    # is not contained in either
    for machine in output:
      if (self._dmakerc.get('DISABLE') != None):
        for d in self._dmakerc.get('DISABLE'):
          if (d in machine.address) or (d in machine.hostname) or (d in machine.username):
            machine.status = 'disabled'
            machine.available = False
            break

    # Populate the two lists of machines
    for machine in output:

      # Available machines
      if machine.available:
        self.workers.append(machine)

      # Unavailable machines
      else:
        self.down.append(machine)

    # Update the build flag
    self._machines = True


  ## Update the DISTCC_HOSTS enviornmental variable (private)
  #  @param kwargs Optional keyword/value pairings
  #
  #  Optional Arguments:
  #    max = True | {False}     - Run the maximum no. of threads (passed to _buildHosts)
  #    localhost = <int>        - Set the DISTCC_HOSTS localhost value (passed to _buildHosts)
  #    localslots = <int>       - Set the DISTCC_HOSTS localslots value (passed to _buildHosts)
  #    localslots_cpp = <int>   - Set the DISTCC_HOSTS localslots_cpp value (passed to _buildHosts)
  def _buildHosts(self, **kwargs):

    # Extract the custom options
    use_max = kwargs.pop('max', False)
    localhost = kwargs.pop('localhost', None)
    localslots = kwargs.pop('localslots', None)
    localslots_cpp = kwargs.pop('localslots_cpp', None)

    # Randomize the workers
    shuffle(self.workers)

    # Create the distcc_hosts and jobs output
    jobs = 0
    distcc_hosts = ''
    for machine in self.workers:
      if use_max:
        distcc_hosts += " " + machine.hostname + '/' + str(machine.threads)
        jobs += int(machine.threads)
      elif machine.use_threads != 0:
        distcc_hosts += " " + machine.hostname + '/' + str(machine.use_threads)
        jobs += machine.use_threads

    # Get the default or user-defined values for localhost/localslots_cpp/localslots
    if localhost == None:
      localhost = min(2, self.master.threads)

    if localslots == None:
      localslots = self.master.threads/4

    if localslots_cpp == None:
      localslots_cpp = self.master.threads - localhost

    # Create the DISTCC_HOSTS variable
    if jobs == 0:
      localhost = self.master.threads
      distcc_hosts = 'localhost/' + str(int(localhost))

    else:
      distcc_hosts = '--localslots=' + str(int(localslots)) + ' --localslots_cpp=' + \
                   str(int(localslots_cpp)) + ' localhost/' + str(int(localhost)) + \
                   distcc_hosts

    # Add the local machine to the jobs total
    jobs += int(localhost)

    # Override jobs if the option is given
    if self._jobs != None:
      jobs = int(self._jobs)
    elif not use_max and jobs > 70:
      jobs = 70

    # Populate ._dmake_data to be written (do not save to the .dmakerc if the --max option was used)
    write = True
    if use_max:
      write = False
    self._dmakerc.set(DISTCC_HOSTS=distcc_hosts, JOBS=jobs, write=write)
