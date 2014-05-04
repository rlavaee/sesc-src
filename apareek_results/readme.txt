prerequisites:
	run:
	gem install PriorityQueue

usage:
	./ctmc.rb [path/to/input/file] [nchannels] [nranks] [nbanks]


This will create a directory in the directory of the input file, with
the same name as the input file. If the directory is already there, it 
overwrites the results.

The ctmc goes over each line in the input file, models it as a ctmc, 
calls MATLAB equation solver, and writes one line in the output file 
(output-ctmc.csv).

During the run, you can inspect the output file to see the results.
