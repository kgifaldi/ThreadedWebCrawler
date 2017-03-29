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
#include <errno.h>
#include <string.h>
using namespace std;

bool KEEP_RUNNING = true;
int fileCount = 1;
ofstream output;

deque<string> toFetch; // stores URLS to CURL
deque<string> toParse; // stores WEB CONTENT to PARSE
deque<string> finalContent; // contains content TO PRINT to OUTFILE

pthread_mutex_t m_file = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_fetch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_parse = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_fetch = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_parse = PTHREAD_COND_INITIALIZER;

int PERIOD_FETCH = 1;
int NUM_FETCH = 2; // number of FETCH THREADS
int NUM_PARSE = 2; // number of PARSE THREADS
string SEARCH_FILE = "Search.txt"; 
string SITE_FILE = "Sites.txt";
string ParseSiteURL;
vector<string> sites;
vector<string> searchTerms;
vector<string> ParseTerms;
vector<string> chartData;


/*
// extra credit write to HTML PAGE
void writeToHTML(vector<string> content){
	// write initial structure to HTML VECTOR
	vector<string> html;
	string dataString;
	for(int i = 1; i < content.size(); i+=2){
		dataString.append(content[i]);
		dataString.append(", ");
	}
	dataString.append("];");
	// dataString should now have the FORM: "var data = [4, 5, 3, ...];"
	dataString.append("var data = [");
	html.push_back("<!DOCTYPE html>"
"					<meta charset=\"utf-8\">"
"					<style>"
"						body{ font: 10px sans-serif }"
"						.axis path,"
"						.axis line{"
"							fill: none;"
"							stroke: #000;"
"							shape-rendering: crispEdges;"
"						}"
"						.bar{ fill: steelblue; }"
"						.x.axis path{ display: none; }"
"						.chart div{"
"							font: 10px sans-serif;"
"							background-color: steelblue;"
"							text-align: right;"
"							padding: 3px;"
"							margin: 1;"
"							color: white;"
"						}"
"					</style>"
"					<html>"
"					<head>"
"						<script type=\"text/javascript\" src=\"https://d3js.org/d3.v4.min.js\"></script>"
"					</head>"
"					<script>");
					
					html.push_back(dataString);	
					
					html.push_back(" "
"						var margin = {top: 20, right: 20, bottom: 30, left: 40},"
"							width = 960-margin.left - margin.right,"
"							height = 500 - margin.top - margin.bottom;"
"						var formatPercent = d3.format(\".0%\");"
"						var x = d3.scale.ordinal().rangeRoundBands([0, width], .1);"
"						var xAxis = d3.svg.axis().scale(x).orient(\"bottom\");"
"						var svg = d3.select(\"body\").append(\"svg\");"
" "
		"				d3.select(\".chart\")"
		"					.selectAll(\"div\")"
		"					.data(data)"
		"					.enter()"
		"					.append(\"div\")"
		"					.style(\"width\", function(d) {return d*10 + \"px\"; })"
		"					.text(function(d) {return d;});"
		"			</script>"
		"			<body>"
		"				<h1>WEB.CPP</h1>");



	html.push_back("</body>"
			"		</html>");
	
	// TODO: Write content of html to HTML FILE...
	string toWrite;
	for(int i = 0; i < html.size(); i++)
		toWrite.append(html[i]);
	ofstream out("try.html");
	out << toWrite;
	out.close();
}
*/
// converts FILE TO VECTOR: each line in file is string in vector
vector<string> fileToVector(string fileName){
	vector<string> tempVector;
	ifstream file;
	string tempString;
	file.open(fileName.c_str());
	if(file.fail()){
		cout << "File: " << fileName << " cause error, exiting..." << endl;
		cerr << "Error: " << strerror(errno);
		exit(1);
	}
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
	while(KEEP_RUNNING){
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
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunction);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &websiteContent);
			res = curl_easy_perform(curl);
			// for sites with errors:
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
	while(KEEP_RUNNING){

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
		writeToFile.clear();
		//writeToFile.append("Time,Phrase,Site,Count\n");
		cout << "size of ParseTerms: " << ParseTerms.size() << endl;
		for(int i = 0; i < ParseTerms.size(); i++){
			count = 0;
			fileEntries.clear();
			stringstream siteStream(ParseSite);
			ostringstream countStream;
			while(siteStream >> parse){
				if(parse.find(ParseTerms[i]) != string::npos){
					count++;
				}
				if(ParseTerms[i] == "Notre")
					cout << "NO NOtre: " << parse << endl;
			}

			char buffer[200];
			// *************** WRITING CONTENT TO FILE ***************** // 
			time(&rawtime); 
			timeinfo = localtime(&rawtime);
			strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S", timeinfo);
			string str(buffer);
			writeToFile.append(str); 
			writeToFile.append(",");
			writeToFile.append(ParseTerms[i]); 
	//		chartData.push_back(ParseTerms[i]); // extra credit
			writeToFile.append(",");
			writeToFile.append(url); 
			writeToFile.append(",");
			countStream << count;
			string temp = countStream.str();
	//		chartData.push_back(temp); // extra credit
			writeToFile.append(temp); 
			writeToFile.append("\n");
			fileEntries.push_back(writeToFile);
			// *************** WRITING CONTENT TO FILE ***************** // 
		}
		// push results to GLOBAL VECTOR
		pthread_mutex_lock(&m_file);
		for(int i = 0; i < fileEntries.size(); i++)
			output << fileEntries[i];
		pthread_mutex_unlock(&m_file);
	}
}		
	

// HANDLES SIGINT
void my_handler(int signum){
	cout << "Cleaning up threads..." << endl;
	KEEP_RUNNING = false;
//	for(int i = 0; i < NUM_FETCH; i++)
//		pthread_join(tid_f[i], NULL);
//	for(int i = 0; i < NUM_PARSE; i++)
//		pthread_join(tid_p[i], NULL);
	cout << "PROGRAM TERMINATED." << endl;
	exit(1);
}
	
	
int main(int argc, char *argv[]){
	if(argc != 2){
		cout << "Incorrect number of arguments: Usage: web <config>: exiting..." << endl;
		exit(1);
	}
	string temp(argv[1]);
	pthread_t tid_f[NUM_FETCH]; pthread_t tid_p[NUM_PARSE];
	vector<string> config = fileToVector(temp);
	for(int i = 0; i < config.size(); i++){
		string param;
		string value;
		string temp = config[i];
		bool equalsFound = false;
		for(int j = 0; j < temp.size(); j++){
			if(temp[j] == '\n')
				continue;
			else if(temp[j] == '='){
				equalsFound = true;
			}
			else if(!equalsFound){
				stringstream ss;
				ss << temp[j];
				string x;
				ss >> x;
				param.append(x);
			}
			else{
				stringstream ss;
				ss << temp[j];
				string x;
				ss >> x;
				value.append(x);
			}
		}
		// protect against config error
		if(!equalsFound)
			continue;
		if(param == "PERIOD_FETCH"){
			if(param.find_first_not_of("0123456789") == string::npos)
				istringstream(param) >> PERIOD_FETCH;
			else
				cout << "PERIOD_FETCH param not integer: using default value..." << endl;
		}
		else if(param == "NUM_FETCH"){
			if(param.find_first_not_of("0123456789") == string::npos)
				istringstream(param) >> NUM_FETCH;
			else
				cout << "NUM_FETCH param not integer: using default value..." << endl;
		}
		else if(param == "NUM_PARSE"){
			if(param.find_first_not_of("0123456789") == string::npos)
				istringstream(param) >> NUM_PARSE;
			else
				cout << "NUM_PARSE param not integer: using default value..." << endl;
		}
		else if(param == "SEARCH_FILE")
			SEARCH_FILE = value;
		else if(param == "SITE_FILE")
			SITE_FILE = value;
		else
			cout << "Warning: Parameter " << param << " doesn't exist." << endl;
	}
	CURL *curl;
	CURLcode res;
	void * thread_result;
	string websiteContent;
	int searchWordCount = 0;
	signal(SIGINT, my_handler);
	signal(SIGHUP, my_handler);
	sites = fileToVector(SITE_FILE);  
	searchTerms = fileToVector(SEARCH_FILE);
	for(int i = 0; i < NUM_FETCH; i++){
		pthread_create(&tid_f[i], NULL, fetchTask, NULL);
	}
	vector<string> tempVec(searchTerms);
	ParseTerms = tempVec;
	for(int i = 0; i < NUM_PARSE; i++)
		pthread_create(&tid_p[i], NULL, parseTask, NULL);
	while(1){	
		string fileName;
		ostringstream ss;
		ss << fileCount;
		fileName = ss.str();
		fileName.append(".csv");
		output.open(fileName.c_str());
		pthread_mutex_lock(&m_file);
		output << "Time,Phrase,Site,Count\n";
		pthread_mutex_unlock(&m_file);
		finalContent.clear(); // maybe remove???
		chartData.clear();
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
		fileCount++;
	//	writeToHTML(chartData); // extra credit
		output.close();
	}		
	return 0;
}
