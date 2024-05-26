CFLAGS=  -m64 -g -w -std=c++11
CXX=g++


ILOG= /opt/ibm/ILOG/CPLEX_Studio201
CPPFLAGS= -DIL_STD -I$(ILOG)/cplex/include -I$(ILOG)/concert/include
CPLEXLIB=-L$(ILOG)/cplex/lib/x86-64_linux/static_pic -lilocplex -lcplex -L$(ILOG)/concert/lib/x86-64_linux/static_pic -lconcert -lm -lpthread

comp:  
	$(CXX) $(CFLAGS) $(CPPFLAGS) -o run *.cpp   $(CPLEXLIB) -ldl
clean:
	rm -f  *.out *.aux *.log *.nav *.snm *.out *.toc 
