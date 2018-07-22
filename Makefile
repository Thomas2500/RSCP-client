CXX=g++
#CXX=arm-linux-gnueabihf-g++
ROOT_VALUE=RscpExample

all: $(ROOT_VALUE)

$(ROOT_VALUE): clean
	rsync -vaP * 10.20.0.2:/root/ownRSCP
	ssh 10.20.0.2 "cd ownRSCP; $(CXX) -O3 RscpExampleMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -static-libstdc++ -std=c++11 -o $@"
	# $(CXX) -O3 RscpExampleMain.cpp RscpProtocol.cpp AES.cpp SocketConnection.cpp -static-libstdc++ -std=c++11 -o $@

clean:
	-rm $(ROOT_VALUE) $(VECTOR)
