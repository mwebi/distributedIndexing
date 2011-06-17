#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <map>
#include <set>

using boost::asio::ip::tcp;

class Node {
public:
	Node(int nodeNumber) {
		this->nodeNumber = nodeNumber;
	}

	// Initialize the node withe the given routing table
	// Note that the algorithm assumes an identical routing table on all nodes.
	void setup(char const *routingTableName) {
		// Setup the networking system
		ioService = new boost::asio::io_service();
		readRoutingTable(routingTableName);
        readTextFile(textFile.c_str());
		try {
			acceptor = new tcp::acceptor(*ioService, routingTable[nodeNumber]);
		} catch (std::exception &) {
			std::cout << "error listening on " << routingTable[nodeNumber] << std::endl;
			throw;
		}
	}

	void run() {
        std::cout << "NodeNumber: " << nodeNumber << std::endl;
        int amountOfNodes = routingTable.size();
        std::string wordToSearch ="";
     
        //------------------------------------
        //calculate receives
        int tester = 1;       
        int receives = 0;
        while((nodeNumber & tester) == 0 && (nodeNumber + tester) < amountOfNodes){
            ++receives;
            tester = tester << 1;
        }
        while((nodeNumber & tester) == 0 && nodeNumber != 0){
            tester = tester << 1;
        }
        
        std::cout << "receives: " << receives << std::endl;
        //------------------------------------

        if(nodeNumber == 0) {            
            
            printf("Word to search: ");
            std::cin >> wordToSearch;

            for(int i(1); i < amountOfNodes; ++i){                
                sendDataToNode(wordToSearch, i);
            }

        } else {  
            std::cout << "send to node: " << (nodeNumber - tester) << std::endl;
            wordToSearch = receiveData();     
            std::cout << "received: " << wordToSearch << std::endl;
        }

        
        WordCounts wordToSearchNextWords;
        wordToSearchNextWords = nextWordCounts[wordToSearch];           

        for (
            WordCounts::iterator wordCount = wordToSearchNextWords.begin();
			wordCount != wordToSearchNextWords.end();
			wordCount++
		) {
			std::cout << " " << wordCount->first << "(" << wordCount->second << ")";
        }
        
        while(receives){
			std::cout << "vor receive" << std::endl;

            std::string receivedResults = receiveData();
            //
            std::stringstream receivedResultsStream(receivedResults);
			
			std::cout << "received this string: " << receivedResults<<std::endl;

            
			while(!receivedResultsStream.eof())
            {
                int x;
                std::string word;
                receivedResultsStream >> word >> x;
                wordToSearchNextWords[word]+=x;
            }
			std::cout << "received this string: " << receivedResults<<std::endl;

            std::cout << --receives << "receives left " << std::endl;
        }
        
        if(nodeNumber == 0){
            //std::cout << "sum: " << sum << std::endl;
            for (
			WordCounts::iterator wordCount = wordToSearchNextWords.begin();
			wordCount != wordToSearchNextWords.end();
			wordCount++
		    ) {
			    std::cout << "merged data: " << wordCount->first << "(" << wordCount->second << ")";
		    }
        }
        else
        {
            //sendDataToNode(mapToString(wordToSearchNextWords), nodeNumber - tester);  
            
            std::stringstream resultsToSend;
            std::string currentWord;
            int currentMax;
            for(int i(0); i < 10; ++i)
            {
                currentMax = 0;
                for (
                WordCounts::iterator wordCount = wordToSearchNextWords.begin();
			    wordCount != wordToSearchNextWords.end();
			    wordCount++
		        ) {
                    if( currentMax < wordCount->second) 
                    {
                        currentMax = wordCount->second;
                        currentWord = wordCount->first;
                    }

			        //std::cout << " " << wordCount->first << "(" << wordCount->second << ")";
                }
				
				if( currentMax == 0) break;
				
				wordToSearchNextWords.erase(currentWord);
				std::string space = " ";
				resultsToSend << currentWord  << space << currentMax << space;
				
				
            }
			std::cout << "string to send: " << resultsToSend.str() << std::endl;
            sendDataToNode(resultsToSend.str(), nodeNumber - tester);
        }

        std::cin.clear();
        int e;
        std::cin >> e;                
	}

private:
	// Sends the given data to the given target node and waits
	// until the data is received by the remote endpoint.
    
    // use typedef to abbreviate the type names
    typedef std::map<std::string,int> WordCounts;
    typedef std::map<std::string,WordCounts> NextWordCounts;

    NextWordCounts nextWordCounts;

	void sendDataToNode(std::string x, int targetNodeNumber) {
		std::stringstream dataStream;
		dataStream << x << "###";
		try {
			tcp::socket socket(*ioService);
			socket.connect(routingTable[targetNodeNumber]);
			socket.send(boost::asio::buffer(dataStream.str()));
			socket.shutdown(tcp::socket::shutdown_both);
			socket.close();
		} catch (std::exception &) {
			std::cout << "error sending data to " << targetNodeNumber << ": " << routingTable[targetNodeNumber] << std::endl;
			throw;
		}
	}
    void printIndex(){
    

	std::cout << "index:\n";
	for (
		NextWordCounts::iterator nextWordCount = nextWordCounts.begin();
		nextWordCount != nextWordCounts.end();
		nextWordCount++
	    ) {
		std::cout << nextWordCount->first << ":";
		for (
			WordCounts::iterator wordCount = nextWordCount->second.begin();
			wordCount != nextWordCount->second.end();
			wordCount++
		) {
			std::cout << " " << wordCount->first << "(" << wordCount->second << ")";
		}
		std::cout << std::endl;
	}
    
    }
	// Waits until data from any incoming node is received and
	// returns the received data.
	// Note that this functions accepts data from any endpoint even if
	// this endpoint is not entered in the routing table.
	std::string receiveData() {
		std::string incommingString;
		char buffer[1024];
		tcp::socket socket(*ioService);
		try {
			acceptor->accept(socket);
			socket.receive(boost::asio::buffer(buffer, 1024));
			socket.shutdown(tcp::socket::shutdown_both);
			socket.close();
			std::stringstream dataStream(buffer);
			std::cout << "received this buffer: " << dataStream.str()<<std::endl;
			std::cout << "pausing: ";
			std::string asd;
			std::cin >> asd;
			dataStream >> incommingString;
			incommingString.erase(incommingString.find("###"));
			return incommingString;
		} catch (std::exception &) {
			std::cout << "error receiving data." << std::endl;
			throw;
		}
	}

	// Reads the routing table from the given file.
	// The number of nodes envolved in the calculation is derived from
	// the routing table.
	void readRoutingTable(char const *routingTableName) {
		char line[1024];
		std::ifstream file;
		std::string ipAddress, port, textFile;
		int nodeNumber = 0;
		tcp::resolver resolver(*ioService);

		file.open(routingTableName);
		while (!file.eof()) {
			file.getline(line, 1024);
			std::stringstream lineStream(line);
			// ignore empty lines
			if (lineStream.str().length() == 0) continue;
			// ignore lines that start with '#'
			if (lineStream.str().at(0) == '#') continue;
			lineStream >> ipAddress;
			lineStream >> port;
            lineStream >> textFile;

			std::cout << nodeNumber << "= " << ipAddress << ":" << port << std::endl;
            if(this->nodeNumber == nodeNumber)
            {
                this->textFile = textFile;
            }
           
			tcp::resolver::query query(tcp::v4(), ipAddress, port);
			try {
				routingTable.push_back(*resolver.resolve(query));
			} catch (std::exception &cause) {
				std::cout << nodeNumber << ": " << cause.what() << std::endl;
			}
			++nodeNumber;
		}
	}

    void readTextFile(char const *fileName) {
		char line[1024];
        
		std::ifstream file;				
        std::string firstWord = "" , secondWord = "";
        int character = 0;

		file.open(fileName);        
		while (!file.eof()) {
            
			file.getline(line, 1024);
            //prepare string for further usage
            std::string lineString(line);
            int stringLength = lineString.size();
            
            boost::to_lower(lineString);
            for( int i(0); i < stringLength; i++)
            {
                character = lineString[i];
                if(character < 0 || character > 255) lineString[i] = ' ';
                if(!std::isalpha(lineString[i])) lineString[i] = ' ';
            }            
            boost::trim(lineString);
            
			std::stringstream lineStream(lineString);            
            
            if (lineStream.str().length() == 0) continue;
            while(!lineStream.eof())
            {		        
                if(firstWord.empty()) lineStream >> firstWord;
		        lineStream >> secondWord;                       

                //put Words into Structur;
                //std::cout << firstWord << ", " << secondWord << std::endl;
                nextWordCounts[firstWord][secondWord]++;
                //
                firstWord = secondWord;
            }           
		}
	    //printIndex();
    }

	// Node topology data
	int nodeNumber;
    std::string textFile;

	// network data
	boost::asio::io_service *ioService;
	std::vector<tcp::endpoint> routingTable;
	tcp::acceptor *acceptor;
};


int main(int argc, char const *argv[]) {
	int nodeNumber = 0;
	char const *routingTableName = "routingTable.txt";
	if (argc > 1) {
		std::stringstream argumentStream(argv[1]);
		argumentStream >> nodeNumber;
	}
	if (argc > 2) {
		routingTableName = argv[2];
	}

	try {
		Node node(nodeNumber);
		node.setup(routingTableName);
		node.run();
	} catch (std::exception &cause) {
		std::cout << cause.what() << std::endl;
	}

	return 0;
}
