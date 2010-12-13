all:
		g++ -Wall src/main.cpp -lstrmanager -llog4cxx -lboost_thread -o bin/strBridge

clean:
		rm -f bin/*