#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <event2/listener.h>

#include <base/thread_pool.h>
#include <base/smart_ptr/smart_ptr.hpp>
#include <base/closure.h>
#include <base/atomic.h>

using namespace comm;

const int LISTEN_PORT = 9999;
const int LISTEN_BACKLOG = 32;
const int MAX_IO_THREAD_COUNT = 5;
const int MAX_JOB_THREAD_COUNT = 10;
const int MAX_JOB_PENDING_COUNT = 100000;

std::vector<struct event_base *> g_ebase;
ptr::scoped_ptr<MainThreadPool > g_io_pool;
ptr::scoped_ptr<ThreadPool<2> > g_job_pool;

struct IOWorkder {
	struct event_base * base;
	IOWorkder() : base(NULL) {}
	void Loop() {
		if (base) {
			event_base_dispatch(base);
			event_base_free(base);
            fprintf(stderr, "io_worker @%x go way\n", base);
		}
	}
};

struct JobData {
	bufferevent * job_out_bev;
	bufferevent * job_in_bev;
	bufferevent * remote_bev;
	uint64_t ref_count;
	Mutex mutex;
	JobData() : job_out_bev(NULL), job_in_bev(NULL), remote_bev(NULL), ref_count(0) {}

	void AddJobOutput(const char* buf, size_t buf_len) {
		SafeMutex _(mutex);
		bufferevent_write(job_in_bev, buf, buf_len);
	}

	int ReadJobOutput(char* buf, size_t buf_len) {
		SafeMutex _(mutex);
		return bufferevent_read(job_out_bev, buf, buf_len);
	}

	void CloseRemote() {
		SafeMutex _(mutex);
		bufferevent_free(remote_bev);
		remote_bev = NULL;
	}

	void IncRef() {
		SafeMutex _(mutex);
		ref_count++;
	}

	void DecRef() {
		SafeMutex _(mutex);
		ref_count--;
		if (ref_count<=0) {
			bufferevent_free(job_in_bev);
			bufferevent_free(job_out_bev);
			fprintf(stderr, "delete job data %x\n", this);
			delete this;
		}
	}

	void SendToRemote(const char* buf, size_t n_bytes) {
		SafeMutex _(mutex);
		if (remote_bev) {
			bufferevent_write(remote_bev, buf, n_bytes);
		}
	}
};

void OnConnection(struct evconnlistener *listener, evutil_socket_t fd,
	           struct sockaddr *a, int slen, void *base);
void OnMessage(struct bufferevent *bev, void *arg);

void OnError(struct bufferevent *bev, short event, void *arg);

void OnSignal(evutil_socket_t sig, short events, void *user_data);

void OnJobComplete(struct bufferevent * job_out_bev, void* arg);

void OnJobError(struct bufferevent *bev, short event, void *arg);

void HandleMessage(char* request, size_t size, void* args);

int main(int argc, char *argv[])
{
	signal(SIGPIPE, SIG_IGN);
	evthread_use_pthreads();

    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(LISTEN_PORT);

    struct evconnlistener *listener;

    g_io_pool.reset(new comm::MainThreadPool());
    g_io_pool->Run(MAX_IO_THREAD_COUNT, MAX_IO_THREAD_COUNT);
    g_job_pool.reset(new comm::ThreadPool<2>() );
    g_job_pool->Run(MAX_JOB_THREAD_COUNT, MAX_JOB_PENDING_COUNT);

    for (int i=0; i < MAX_IO_THREAD_COUNT; i++) {
		struct event_base *base = event_base_new();
		IOWorkder * worker = new IOWorkder();
		worker->base = base;
		struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, OnMessage, NULL, OnError, worker);
		bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST); //dummy event, never triggered;
		g_ebase.push_back(base);
		g_io_pool->Submit(NewClosure(worker,&IOWorkder::Loop));
    }

    printf ("Listening...\n");

    struct event_base *base = event_base_new();
	assert(base != NULL);
	listener = evconnlistener_new_bind(base, OnConnection, base,
			LEV_OPT_CLOSE_ON_FREE | LEV_OPT_CLOSE_ON_EXEC | LEV_OPT_REUSEABLE | LEV_OPT_THREADSAFE,
			-1, (struct sockaddr*) &sin, sizeof(struct sockaddr));
	assert(listener);

	struct event* signal_event = evsignal_new(base, SIGINT, OnSignal, (void *)base);
	event_add(signal_event, NULL);

	event_base_dispatch(base);

	evconnlistener_free(listener);

    event_base_free(base);

    fprintf(stderr, "close\n");

    return 0;
}

void OnConnection(struct evconnlistener *listener, evutil_socket_t fd,
	    struct sockaddr *a, int slen, void *arg)
{
	static int64_t req_counter = 0;
	req_counter++;
	struct event_base *base = g_ebase[req_counter % MAX_IO_THREAD_COUNT];
	struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE );

	struct bufferevent * rsps_bev[2];
	int ret = bufferevent_pair_new(base, BEV_OPT_CLOSE_ON_FREE, rsps_bev);
	assert(ret>=0);
	JobData* job_data = new JobData();
	job_data->job_in_bev = rsps_bev[0];
	job_data->job_out_bev = rsps_bev[1];
	job_data->remote_bev = bev;
	job_data->IncRef();

	bufferevent_setcb(bev, OnMessage, NULL, OnError, job_data);
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);

	bufferevent_setcb(job_data->job_out_bev, OnJobComplete, NULL, OnJobError, job_data);
	bufferevent_enable(job_data->job_out_bev, EV_READ | EV_PERSIST);
}

void OnJobComplete(struct bufferevent * job_out_bev, void* arg) {
	JobData * job_data = (JobData*)arg;
	assert(job_data);
	char buf[256];
	int n_bytes;
	while (true) {
		n_bytes = job_data->ReadJobOutput(buf, sizeof(buf));
		if (n_bytes<=0) {
			break;
		}
		job_data->SendToRemote(buf, n_bytes);
		job_data->DecRef();
	}
}

void OnJobError(struct bufferevent *bev, short event, void *arg) {
	JobData * job_data = (JobData*)arg;
	job_data->DecRef();
}

void HandleMessage(char* request, size_t size, void* args) {
	assert(args);
	std::string resps(request, size);
	resps = "ACK:" + resps + "\n";
	free(request);
	JobData* job_data = (JobData*)args;
	job_data->AddJobOutput(resps.data(), resps.size());
}

void OnMessage(struct bufferevent *bev, void *args)
{
	assert(args);
	JobData* job_data = (JobData*)args;
    evbuffer* input = bufferevent_get_input(bev);
    size_t nbytes = 0;
    char* line = NULL;
    while (line = evbuffer_readln(input, &nbytes, EVBUFFER_EOL_ANY), line) {
		printf("recv: %s\n", line);
		job_data->IncRef();
		g_job_pool->Submit(NewClosure(&HandleMessage, line, nbytes, args));
    }

}

void OnError(struct bufferevent *bev, short what, void *args)
{
	assert(args);
	JobData* job_data = (JobData*)args;
	fprintf(stderr, "err %d\n", what);
	job_data->CloseRemote();
	job_data->DecRef();
}

void OnSignal(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = (struct event_base*) user_data;
	struct timeval delay = { 0, 0 };

	event_base_loopexit(base, &delay);

	for (int i=0; i < g_ebase.size(); i++ ) {
		fprintf(stderr,"Signal -> WORKER[%d]\n", i);
		event_base_loopexit(g_ebase[i], &delay);
	}
}















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
