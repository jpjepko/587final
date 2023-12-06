CXX			:= g++
CXXFLAGS	:= -g

delta: delta.o
	$(CXX) $(CXXFLAGS) -fopenmp delta.o -o delta

delta.o: delta.cpp
	$(CXX) -c delta.cpp -fopenmp -o delta.o

.PHONY: clean
clean:
	rm -f delta delta.o delta_time.txt delta.stdout delta.stderr slurm*.out
