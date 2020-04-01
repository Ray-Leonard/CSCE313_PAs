/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

int buffercapacity = MAX_MESSAGE;

void request_data_point(int p, double t, int e, FIFORequestChannel &chan)
{
    datamsg d (p, t, e);
    chan.cwrite(&d, sizeof(datamsg));
    double data;
    chan.cread((char*)&data, sizeof(double));
    cout << "Response from request_data_point() " << data << endl;
}



void request_data_for_a_person(int person, FIFORequestChannel &chan)
{
    ofstream x1;
    x1.open("x1.csv");

    // Requesting Data Points:
    for(int i = 0; i < 15000; ++i)
    {
        // request the first person's ecg1 and ecg2
        datamsg d1(person, i*0.004, 1);
        chan.cwrite(&d1, sizeof(datamsg));
        double data1;
        chan.cread((char*)&data1, sizeof(double));

        datamsg d2(person, i*0.004, 2);
        chan.cwrite(&d2, sizeof(datamsg));
        double data2;
        chan.cread((char*)&data2, sizeof(double));

        // write the data in the file
        x1 << i*0.004 << "," << data1 << "," << data2 << ",\n";
    }
    x1.close();
}

void request_file(char* filename, FIFORequestChannel &chan)
{
    // all the requests must append the filename after it! thats the only way the server's gonna know the file location
    // make a request with 0,0 and get the length of the file. 
    // based on the file length and buffercapacity, calculate how many requests you need
    // loop and request several times, write the received c string from the server to your file

    // create a buffer with enough size for request+filename
    int buffersize = sizeof(filemsg) + strlen(filename) + 1;
    char* msgBuffer = new char[buffersize];

    // make a special request to get the file length
    filemsg fileLengthRequest(0, 0);
    // copy the filemsg to the beginning of the buffer
    memcpy(msgBuffer, &fileLengthRequest, sizeof(filemsg));
    // append the filename after the request
    memcpy(msgBuffer + sizeof(filemsg), filename, strlen(filename)+1);
    // write in the special request
    chan.cwrite(msgBuffer, buffersize);
    // read the return value from the server, getting the filesize
    __int64_t fileSize;
    chan.cread((char*)&fileSize, sizeof(__int64_t));
    
    // calculate how many requests are needed
    int numRequests = ceil((double)fileSize / buffercapacity);
    ofstream ofs;
    string dest = "received/" + string(filename);
    ofs.open(dest);
    // request from the server several times
    for(int i = 0; i < numRequests; ++i)
    {
        // update the msg request by makeing new filemsg and copy that memory into msgBuffer
        int offset = i * buffercapacity;
        int length = (fileSize - offset > buffercapacity) ? buffercapacity : (fileSize - offset);
        filemsg f(offset, length);
        memcpy(msgBuffer, &f, sizeof(filemsg));
        // request the window from server
        chan.cwrite(msgBuffer, buffersize);
        // receive the window from server
        char receiveBuffer[length];
        chan.cread(receiveBuffer, sizeof(receiveBuffer));
        // write the buffer in file
        ofs.write(receiveBuffer, length);
    }
    ofs.close();
}

FIFORequestChannel request_channel(FIFORequestChannel &chan)
{
    MESSAGE_TYPE newChan = NEWCHANNEL_MSG;
    chan.cwrite(&newChan, sizeof(NEWCHANNEL_MSG));
    char newName[500];
    chan.cread(newName, sizeof(newName));
    FIFORequestChannel chan1(newName, FIFORequestChannel::CLIENT_SIDE);
    return chan1;
}


int main(int argc, char *argv[]){

    int n = 100;    // default number of requests per "patient"
    int p = -1;     // number of patients
    srand(time_t(NULL));
    double t = -1;
    int e = -1;
    int m = MAX_MESSAGE;
    char* m_char = nullptr;
    char* f = nullptr;
    int opt;
    bool c = false;
    // getopt to process flags: -p -t -e, -f, -m
    while((opt = getopt(argc, argv, "p:t:e:m:f:c")) != -1)
    {
        switch(opt)
        {
            case 'p':
            p = atoi(optarg);
            break;

            case 't':
            t = atof(optarg);
            break;

            case 'e':
            e = atoi(optarg);
            break;

            case 'm':
            m = atoi(optarg);
            m_char = optarg;
            break;

            case 'f':
            f = optarg;
            break;

            case 'c':
            c = true;

            default:
            break;
        }
    }
    buffercapacity = m;

    int id = fork();
    // getopt should be here in order to capture all possible cases
    if(id == 0) // child process, run server
    {
        char* args[] = {"./server", NULL, NULL, NULL};
        // if requested a buffer change
        if(m_char)
        {
            args[1] = "-m";
            args[2] = m_char;
        }
        execvp(args[0], args);
    }
    else
    {

        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

        // request a single data point
        if(p != -1 && t != -1 && e != -1)
        {
            request_data_point(p, t, e, chan);
        }

        // ************* request data point
        clock_t start, end;
        if(p != -1)
        {
            start = clock();
            request_data_for_a_person(p, chan);
            end = clock();
            cout << "request_data_for_a_person runtime: " << (double(end - start)) << endl;
        }

        if(f)
        {
            // ************* request a file
            start = clock();
            request_file(f, chan);
            end = clock();
            cout << "request_file runtime: " << double(end - start) << endl;
        }


        FIFORequestChannel chan1 = request_channel(chan);
        if (c)
        {
            // ************** request a new channel
            // request some data point through new channel
            datamsg d (14, 0.008, 1);
            chan1.cwrite(&d, sizeof(datamsg));
            double data;
            chan1.cread((char*)&data, sizeof(double));
            cout << "Response " << data << endl;
        }


        // closing the channel    
        MESSAGE_TYPE m = QUIT_MSG;
        chan.cwrite (&m, sizeof (MESSAGE_TYPE));
        chan1.cwrite(&m, sizeof (MESSAGE_TYPE));
        wait(NULL);
    }

    
}
