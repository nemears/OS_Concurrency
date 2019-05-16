  All test output is in problem_1_explanation.txt and problem_2_explanation.txt.
For the pirates and ninjas i used semaphores exclusively and locks and
conditions for the intersection one. ninjas has to be run with the proper
amount of commands or the key char 'r' for random and 'g' for general (i used
these for testing) e.g:

  ./ninjas 3 10 50 0 1 2 3
  ./ninjas r
  ./ninjas g

  For the intersection simulation there are no command line arguments for
executing the code. The thread function pointer has an embedded do while loop
it is written in, so the variable in the while can be changed from a zero to a
one then the intersection will loop forever.
