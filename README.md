# xv6 OS - upgrade
In this project, the default scheduler of xv6 operating system is changed. 
Implemented scheduling algorithms are:
  - Completely Fair Scheduler (**CFS**) 
  - Shortest Job First (**SJF**).

Both of them are using Red-Black trees as queues for ready processes.

Beside these two scheduling algorithms, there is one global scheduler that takes care of **process affinity** and performs **load balancing** between multiple processors.
This way every CPU is (almost) equally loaded and miss rate in CPU caches (and possibly TLB) is lowered.

Also, in OS is implemented one system call (chsched) and one user program, with same name as system call, that can be accessed through command line interface and is used for changing between two implemented scheduling algorithms.

***More about project and implementation details can be read from "xv6 Operating System Project.pdf" file, that is given in this repository.***
