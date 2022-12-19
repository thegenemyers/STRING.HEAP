# String Priority Queues
  
<font size ="4">**_Author:  Gene Myers_**<br>
**_First:   Aug. 17, 2022_**<br>

This repository contains the code and timing experiments for the paper ``Merging Sorted Lists of Similar Strings'' that is at the [arXiv](https://arxiv.org/abs/2208.09351) and currently under submission.

There are 5 programs that merge some number of sorted, non-redundant input lists of strings, into a single, non-redundant sorted list of all the strings.  The IO for the
five codes is identical, the only point of variation is how a priority queue of strings
is realized.  The five programs are:

* ```Heap```: uses a standard heap comparing string elements directly
* ```Sheap```: uses a string heap
* ```Cheap```: uses a collision heap but compares strings directly
* ```SCheap```: uses a combined string and collision heap
* ```Trie```: uses a compact trie, with S-element outedge array at each node

The ```Makefile``` herein compiles these codes and "make trials" will run the experiments reported in Tables 1 and 2 of the paper.  The trials rely on two auxiliary programs, ```Rran``` and ```Cran```, that generate synthetic data sets for the Uniform and Shotgun scenarios, respectively (see the paper).

I apologize that the trials for Table 3 on real data are not reproducible here, as the volume of data that would need to be stored at this repository seemed, to my mind, excessive in relation to its purpose.
