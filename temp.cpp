#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <sstream>
#include <queue>
#include <pthread.h>
using namespace std;

// when the size of this queue is greater than 0
// the number of parse threads specified will be going
deque<string> toParse;
deque<string> finalContent;
int numParseRunning = 0;
pthread_mutex_t m;
pthread_cond_t c;
// every N seconds, this will be set to true
// when true, the number of threads specified will fetch web content
bool fetch = true;

// defaults:
int PERIOD_FETCH = 180;
int NUM_FETCH = 1;
int NUM_PARSE = 1;
string SEARCH_FILE = "Search.txt";
string SITE_FILE = "Sites.txt";

// fileToVector: returns vector of strings
//               each entry is one line of file
vector<string> fileToVector(string fileName){

	vector<string> tempVector;
	ifstream file;
	string tempString;
	file.open(fileName.c_str());
	while(getline(file, tempString)){
		tempVector.push_back(tempString);
	}
	return tempVector;

}

// function called by licburl to write the data to a string
// found at this stackoverflow post: http://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
static size_t WriteFunction(void * contents, size_t size, size_t nmemb, void *userp){
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


struct fetch_args{

	string site;

};

struct parse_args{

	string site;
	string siteURL;
	vector<string> terms;

};

void * fetchTask(void * data){
	struct fetch_args * args = (struct thread_args *) data;
	string websiteContent;
	CURL * curl = curl_easy_init();
	CURLcode res;
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, args->site.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &websiteContent);
		res = curl_easy_perform(curl);

		// if error:
		if(res != CURLE_OK)
			cout << stderr << " curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
		curl_easy_cleanup(curl);

		pthread_mutex_lock(&c);
		toParse.push_back(websiteContent);
		// TODO: implement condition variabe
		pthread_mutex_unlock(&c);
	}
}	
void * parseTask(void * data){
	struct parse_args * args = (struct parse_args *) data;
	string parse;
	vector<string> fileEntries;
	string writeToFile;
	int count = 0;
	for(int i = 0; i < (args->terms).size(); i++){
		count = 0;
		stringstream siteStream(args->site);
		while(siteStream >> parse){
			if(parse.find(args->terms[i]) != string::npos)
				count++;
		}
		writeToFile.clear();
		writeToFile.append("Time,Phrase,Site,Count\n");
		writeToFile.append(asctime(localtime(&time));
		writeToFile.append(",");
		writeToFile.append(args->terms[i]);
		writeToFile.append(",");
		writeToFile.append(args->siteURL);
		writeToFile.append(",");
		writeToFile.append(count);
		writeToFile.append("\n");
		fileEntries.push_pack(writeToFile);
	}
	pthread_mutex_lock(&c);
	toParse.pop_front();
	for(int i = 0; i < fileEntries.size(); i++)
		finalContent.push_back(fileEntries[i]);
	pthread_mutex_unlock(&c);
	// TODO: condition variable
}


int main(){
	
	// Variables...
	ofstream myfile;
	myfile.open("outTmp.txt");
	vector<string> sites = fileToVector(SITE_FILE);
	vector<string> searchTerms = fileToVector(SEARCH_FILE);
	CURL *curl;
	CURLcode res;
	string websiteContent;
	time_t time; // = time(nullptr);
	int searchWordCount = 0;
	void * thread_result;
	pthread_t tid_f[NUM_FETCH];
	pthread_t tid_p[NUM_PARSE];
	int siteCounter = 0;
	clock_t begin;
	clock_t end;
	int parsed = 0;
	// read in sites and search terms into vector<stirng>
	for(int i = 0; i < sites.size(); i++)
		cout << sites[i] << endl;
	for(int i = 0; i < searchTerms.size(); i++)
		cout << searchTerms[i] << endl;
	
	while(siteCounter < sites.size()){
		begin = clock();
		for(int i = 0; i < NUM_FETCH; i++){
			if(siteCounter == sites.size())
				break;
				struct fetch_args * args;
			args = malloc(sizeof(struct fetch_args));
			args->site = site[siteCounter];
			siteCounter++;
			pthread_create(&tid_f[i], NULL, fetch_task, (void *) args);
		}
		for(int i = 0; i < NUM_FETCH; i++){
			// on last loop of fetch tasks, all of NUM_FETCH 
			// threads may not be used 
			// so I am checking if i >= (sites.size() mod NUM_FETCH)
			// to break the loop
			if(siteCounter == sites.size() && i > (sites.size() % NUM_FETCH))
				break;
			pthread_join(tid_f[i], &thread_result);
		}
		int toJoin = 0;
		for(int i = 0; i < NUM_PARSE; i++){
			if(to_parse.size() == 0)
				break;
			else{
				parsed++;
				toJoin++;
			}
			struct parse_args * args;
			args = malloc(sizeof(struct parse_args));
			args->site = toParse.front();
			args->siteURL = sites[parsed-1];
			vector<string> tempVec(searchTerm);
			args->terms = tempVec;
			pthread_create(&tid_p[i], NULL, parse_task, (void *) args);
		}
		for(int i = 0; i < NUM_PARSE; i++){
			if(toJoin == 0)
				break;
			pthread_join(tid_p[i], &thread_result);
			toJoin--;
		}
		// time segment at bottom of while loop
		// so that threads begin immediately
		// then wait proper time for next batch of threading
		end = clock();
		elapsed_time = double(end - begin) / CLOCKS_PER_SEC;
		while(elapsed_time < PERIOD_FETCH){
			end = clock();
			elapsed_time = double(end - begin);
		}		
	}	
	return 0;
}
