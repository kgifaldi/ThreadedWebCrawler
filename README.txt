Kyle Gifaldi
README.txt

stucture and function: 

MAIN: My program first initiates the set number of fetch and parse threads 
(determined by NUM_FETCH and NUM_PARSE in the config file). Then, the program 
runs a while(true) loop within the main function which serves to keep track 
of time between loops and to signal the fetch threads once the set number of 
seconds is reached for each cycle of operation. So, within the while loop, 
another while loop is entered until the elapsed_time equals the time set by 
PERIOD_FETCH. Upon which, the program moves on to repopulate the toFetch deque 
(used by the fetch threads) and broadcasts to all fetch threads. 

FETCH: within the fetch thread function (called fetchTask in my program) 
there is another while(True) loop. The fetch thread first locks the fetch
 mutex, then calls pthread_cond_wait while toFetch.size() equals 0. Then, 
once a url is in the toFetch deque (and the main function signals all fetch 
threads), the fetch thread will use libcurl to fetch the content at a url 
in toFetch. Then the fetch thread will lock the parse mutex and push back 
the toParse deque with whatever web content was returned from curl. Then 
the fetch thread broadcasts to all parse threads and unlocks the parse mutex.

PARSE: within the parse thread function (called parseTask in my program) 
there is also a while(True) loop. The parse thread locks the parse mutex, 
and waits until toParse.size != 0. Once toParse.size() != 0 and is signaled 
by the fetch threads, the parse thread will pop_front from the toParse 
deque and give up the parse mutex. Then the parse thread counts all 
occurances of all search words in whatever was returned from pop_front. 
Finally, the parse threads all write to the same file once they’re all finished.
thread_mutex_unlock(&m_parse);

