#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <time.h>
#include <thread>
#include <signal.h>
using namespace std;

FIFORequestChannel* create_new_channel(FIFORequestChannel* control)
{
    char name[1024];
    MESSAGE_TYPE m = NEWCHANNEL_MSG;
    control->cwrite(&m, sizeof(m));
    control->cread(name, 1024);
    FIFORequestChannel* newchan = new FIFORequestChannel (name, FIFORequestChannel::CLIENT_SIDE);
    return newchan;
}

// the following two thread funcitons are to be passed in the thred()

/* 
    in each thread, it creates the datamsg request and push them into the bounded buffer
    PARAMS:
    @n    the number of datapoint requests this thread will do
    @pno  the patient number indicatin which patient it is
    @request_buffer the bounded buffer that the datamsg will be pushed into
 */
void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer)
{
    datamsg d(pno, 0.0, 1);
    for(int i = 0; i < n; i++)
    {
        request_buffer->push((char*) &d, sizeof(datamsg));
        d.seconds = 0.004 * (i+1);
    }
}
// below is the patient_thread_function() of the naive approach
// void patient_thread_function(int n, int pno, FIFORequestChannel* chan, HistogramCollection* hc){
//     datamsg d(pno, 0.0, 1);
//     double resp = 0;

//     for(int i = 0; i < n; i++)
//     {
//         // request and get the data
//         chan->cwrite(&d, sizeof(datamsg));
//         chan->cread(&resp, sizeof(double));
//         // update the datamsg by adding 0.004 seconds to it
//         d.seconds = 0.004 * (i+1);

//         // update the histogram
//         hc -> update(pno, resp);
//     }

// }


void file_thread_function(string fname, BoundedBuffer* request_buffer, FIFORequestChannel* chan, int m)
{
    //1. create the file
    string recvfname = "received/" + fname;

    char buf[1024];
    // make the offset of the first filemsg to be 0, which will start reading from the beginning of the file
    filemsg f(0, 0);
    // copy the filemsg onto the buffer
    memcpy(buf, &f, sizeof(f));
    // append the filename after the filemsg in the buffer
    strcpy(buf + sizeof(f), fname.c_str());

    // write the special msg to the server and get the file length
    chan->cwrite(buf, sizeof(f) + fname.size() + 1);
    __int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    // create the file
    FILE* fp = fopen(recvfname.c_str(), "w");
    // make it as long as the original length
    fseek(fp, filelength, SEEK_SET);
    fclose(fp);


    //2. generate all the file messages and push them onto bounded buffer
    // though I had done it in my PA2, but I figure Dr.Ahmed's approach is better than what I used to do.
    filemsg* fm = (filemsg*) buf;
    __int64_t remlen = filelength;

    while(remlen > 0)
    {
        fm->length = min(remlen, (__int64_t) m);
        request_buffer->push(buf, sizeof(filemsg) + fname.size() + 1);
        fm->offset += fm->length;
        remlen -= fm->length;
    }

}

/*
    it will collect datamsg from the bounded buffer and request the data from the server
    then it will update the histogram
    @chan the channel it will work through
    @hc   the histogram collection that the datapoint will be updated to
    @request_buffer the bounded buffer that worker will pull request from
*/
void worker_thread_function(FIFORequestChannel* chan, HistogramCollection* hc, BoundedBuffer* request_buffer, int mb){
    char buf[1024];
    double resp = 0.0;

    // receive buffer for file messages
    char recvbuf[mb];

    // here will simply use this infinite while loop that terminates upon some condition
    // because the worker thread doesn't know how many work it will do
    // some worker threads will process more messages than others
    while(true)
    {
        //1. pop from the request buffer
        request_buffer->pop(buf, 1024);

        // we don't know what the message type will be
        // message type could be datamsg, quitmsg or file request
        // dereference the buffer as MESSAGE_TYPE to get the actual message type
        MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;  //???? how can you just deference it as a message type?

        // send the message to the channel based on different message types
        if(*m == DATA_MSG)
        {
            chan->cwrite(buf, sizeof(datamsg));
            chan->cread(&resp, sizeof(double));
            // update the histogram
            hc->update(((datamsg*)buf)->person, resp);
        }
        else if(*m == FILE_MSG)
        {
            filemsg* fm = (filemsg*) buf;
            string fname = (char*) (fm + 1);   // obtain the filename from the filemessage
            int size = sizeof(filemsg) + fname.size() + 1;  // size of buffer = filemsg size + filename size + 1(for '\0')
            // request the msg from the server
            chan->cwrite(buf, size);
            // receive a chuck of file
            chan->cread(recvbuf, mb);

            // put the data chuck in the file
            string recvfname = "received/" + fname;

            // open the file in c-style way
            FILE* fp = fopen(recvfname.c_str(), "r+");  // make sure it is opened in read and write mode
            // always go to the right location to write the chunk
            fseek(fp, fm->offset, SEEK_SET);
            // write the data
            fwrite(recvbuf, 1, fm->length, fp);
            fclose(fp);
        }
        // where it breaks out the loop
        else if(*m == QUIT_MSG)
        {
            chan->cwrite((char*)m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;
        }
    }
}



int main(int argc, char *argv[])
{
    int n = 15000;    //default number of requests per "patient"
    int p = 5;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    string fname = ""; // file name, and based on this f flag, user decides to request filemsg or datamsg.
                            // all other flags are optional.
    char* m_char = nullptr;
    srand(time_t(NULL));
    
    int opt = -1;
    while((opt = getopt(argc, argv, "m:n:b:w:p:f:")) != -1)
    {
        switch(opt)
        {
            case 'm':
                m = atoi(optarg);
                m_char = optarg;
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'f':
                fname = optarg;
            default:
                break;
        }
    }
    

    int pid = fork();
    if (pid == 0){
		if(m_char)
        {
            execl ("server", "server", "-m", m_char, (char*) NULL);
        }
        else
        {
            execl ("server", "server", (char *)NULL);
        }
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
	// create some histograms for every patient
    for(int i = 0; i < p; i++)
    {
        /*
            Histogram params:
            @int the number of bins
            @double the min value - if a value is smaller than this,
            then it will be automatically put to the first bin
            @double the max value - same logic follows!
        */
        Histogram* h = new Histogram(10, -2.0, 2.0);
        // add the histogram to HistogramCollection hc
        hc.add(h);
    }

    // make w worker channels for each worker thread
    FIFORequestChannel* wchans [w];
    for(int i = 0; i < w; i++)
    {
        wchans[i] = create_new_channel(chan);
    }


	// measure the runtime
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */

    // this set of threads is for patients (generating datamsg requests)
	thread patient [p];
    thread * filethread;

    // -f flag detected
    if(fname != "")
    {
        filethread = new thread(file_thread_function, fname, &request_buffer, chan, m);
    }
    // requesting data points
    else
    {
        for(int i = 0; i < p; i++)
        {
            patient[i] = thread(patient_thread_function, n, i+1, &request_buffer);
        }
    }



    // this set of threads is for workers (commucating with server and get the data)
    thread workers[w];
    for(int i = 0; i < w; i++)
    {
        workers[i] = thread(worker_thread_function, wchans[i], &hc, &request_buffer, m);
    }


    // before the program is done, set the timer and generate signals. 


	/* Join all threads here */
    // join the patient threads

    // join the filethread only
    if(fname != "")
    {
        filethread->join();
        cout << "file thread finished" << endl;
        // clean up filethread
        delete filethread;
    }
    else// otherwise if requesting datapoints, join the patient threads only.
    {
        for(int i = 0; i < p; i++)
        {
            patient[i].join();  // join() is wait for the threads to finish.
        }
        cout << "patient threads finished" << endl;
    }


    // push the quit messages into the bounded buffer when patients has done pushing data in.
    for(int i = 0; i < w; i++)
    {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push((char*) &q, sizeof(q));
    }

    // join the worker threads
    for(int i = 0; i < w; i++)
    {
        workers[i].join();
    }
    cout << "worker threads finished" << endl;

    gettimeofday (&end, 0);
    // print the results
	hc.print ();

    // calculate the time difference
    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;

    int pid_1 = fork();
    if(pid_1 == 0)
    {
        execlp("rm", "rm", "-f", "fifo_data100_1", NULL);
    }
    
}
