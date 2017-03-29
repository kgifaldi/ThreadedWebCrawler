// Author: Kyle Gifaldi
// Program: web.cpp
// Purpose: Use threads to search given web pages
// 			and to parse web pages for given terms
// Date: March 2017

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <curl/curl.h>
#include <sstream>
#include <queue>
#include <pthread.h>
#include <ctime>
#include <stdlib.h>
#include <signal.h>
using namespace std;

deque<string> toFetch; // stores URLS to CURL
deque<string> toParse; // stores WEB CONTENT to PARSE
deque<string> finalContent; // contains content TO PRINT to OUTFILE

pthread_mutex_t m_fetch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_parse = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_fetch = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_parse = PTHREAD_COND_INITIALIZER;

int PERIOD_FETCH = 10;
int NUM_FETCH = 2; // number of FETCH THREADS
int NUM_PARSE = 2; // number of PARSE THREADS
string SEARCH_FILE = "Search.txt"; 
string SITE_FILE = "Sites.txt";
string ParseSiteURL;
vector<string> sites;
vector<string> searchTerms;
vector<string> ParseTerms;


// converts FILE TO VECTOR: each line in file is string in vector
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


// REFERENCE: http://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c
static size_t WriteFunction(void * contents, size_t size, size_t nmemb, void *userp){
	((string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}


void * fetchTask(void *){
	string url; string websiteContent;
	while(1){
		// *********** LOCKED *********** //
		pthread_mutex_lock(&m_fetch);
		while(toFetch.size() == 0)
			pthread_cond_wait(&c_fetch, &m_fetch);
		url = toFetch.front();
		toFetch.pop_front();
		pthread_mutex_unlock(&m_fetch);
		// *********** UNLOCKED *********** //
		websiteContent.clear(); 
		CURL * curl = curl_easy_init();
		CURLcode res;
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &websiteContent);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
				cout << stderr << " curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
			curl_easy_cleanup(curl);
			// ***** LOCKED ******* //
			pthread_mutex_lock(&m_parse);
			toParse.push_back(url);
			toParse.push_back(websiteContent);
			pthread_cond_broadcast(&c_parse);
			pthread_mutex_unlock(&m_parse);
			// ***** UNLOCKED ****** //
		}
	}
}	


void * parseTask(void *){
	string url;
	string ParseSite;
	string parse;
	vector<string> fileEntries;
	string writeToFile;
	int count = 0;
	struct tm * timeinfo;time_t rawtime; 
	while(1){

		// ***** LOCKED ****** //
		pthread_mutex_lock(&m_parse);
		while(toParse.size() == 0)
			pthread_cond_wait(&c_parse, &m_parse);
		url = toParse.front();
		toParse.pop_front();
		ParseSite = toParse.front();
		toParse.pop_front();
		pthread_mutex_unlock(&m_parse);
		// ***** UNLOCKED ****** //

		fileEntries.clear();
		count = 0;
		for(int i = 0; i < (ParseTerms).size(); i++){
			count = 0;
			stringstream siteStream(ParseSite);
			ostringstream countStream;
			while(siteStream >> parse){
				if(parse.find(ParseTerms[i]) != string::npos)
					count++;
			}

			char buffer[200];
			// *************** WRITING CONTENT TO FILE ***************** // 
			writeToFile.clear();
			writeToFile.append("Time,Phrase,Site,Count\n");
			time(&rawtime); 
			timeinfo = localtime(&rawtime);
			strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S", timeinfo);
			string str(buffer);
			writeToFile.append(str); 
			writeToFile.append(",");
			writeToFile.append(ParseTerms[i]); 
			writeToFile.append(",");
			writeToFile.append(url); 
			writeToFile.append(",");
			countStream << count;
			string temp = countStream.str();
			writeToFile.append(temp); 
			writeToFile.append("\n");
			fileEntries.push_back(writeToFile);
			// *************** WRITING CONTENT TO FILE ***************** // 
		}
		// push results to GLOBAL VECTOR
		for(int i = 0; i < fileEntries.size(); i++)
			finalContent.push_back(fileEntries[i]);
	}
}		
	

// HANDLES SIGINT
void my_handler(int signum){
	cout << "PROGRAM TERMINATED." << endl;
	exit(1);
}
	
	
int main(){
	CURL *curl;
	CURLcode res;
	void * thread_result;
	string websiteContent;
	int searchWordCount = 0;
	signal(SIGINT, my_handler);
	sites = fileToVector(SITE_FILE);  
	searchTerms = fileToVector(SEARCH_FILE);
	pthread_t tid_f[NUM_FETCH]; pthread_t tid_p[NUM_PARSE];
	for(int i = 0; i < NUM_FETCH; i++){
		pthread_create(&tid_f[i], NULL, fetchTask, NULL);
	}
	vector<string> tempVec(searchTerms);
	ParseTerms = tempVec;
	for(int i = 0; i < NUM_PARSE; i++)
		pthread_create(&tid_p[i], NULL, parseTask, NULL);
	while(1){
		finalContent.clear();
		time_t begin = clock();
		time_t end = clock();
		double elapsed_time = double(end - begin) / CLOCKS_PER_SEC; // differnece between END - BEGIN
		while(elapsed_time < PERIOD_FETCH){
			end = clock();
			elapsed_time = double(end - begin) / CLOCKS_PER_SEC;
		}
		for(int i = 0; i < sites.size(); i++){
			toFetch.push_back(sites[i]);
		}
		pthread_cond_broadcast(&c_fetch);
		for(int i = 0; i < finalContent.size(); i++)
			cout << finalContent[i];
	}		
	return 0;
}
