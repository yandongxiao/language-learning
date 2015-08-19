#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>

void read_from_client_cb (struct bufferevent *bev, void *ctx)
{
    printf ("enter read_from_client_cb\n");
}




void write_to_client_cb (struct bufferevent *bev, void *ctx)
{
    /* do not use read/write to receive or send data
        * fd = bufferevent_get_fd (bev);
        * write (fd, )...
        */
    printf ("enter write_to_client_cb\n");
    char buff[512];

    /* the write call_back will run immediately, If it is not interrupted by fgets. */
    fgets (buff, 512, stdin);
    int len = strlen(buff);
    if (buff[len-1] == '\n')
        buff[len-1] = '\0';
    
    bufferevent_write (bev, buff, strlen(buff));
}

void event_cb(struct bufferevent *bev, short what, void *ctx)
{
    printf ("event_cb\n");
    if (what & BEV_EVENT_ERROR) {
        printf ("receive an error <%s>\n", 
            evutil_socket_error_to_string (EVUTIL_SOCKET_ERROR()));
    }
    else if (what & BEV_EVENT_CONNECTED) {
        printf ("connected to client\n");
    }
    else if (what & BEV_EVENT_EOF){
        printf ("client close the connection\n");
        bufferevent_free (bev);
    }
}

void*
thread_func (void *arg)
{
    evutil_socket_t fd = arg;

    struct event_base *base = event_base_new ();

    struct bufferevent * bufev = NULL;
    bufev = bufferevent_socket_new (base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bufev,read_from_client_cb, write_to_client_cb, event_cb, NULL);
    bufferevent_enable (bufev, EV_WRITE | EV_READ);
    event_base_dispatch (base);
}

void listener_cb (struct evconnlistener *listener, 
                                    evutil_socket_t fd,
                                    struct sockaddr *addr, 
                                    int socklen, 
                                    void *arg)
{
    struct sockaddr_in *clientaddr = (struct sockaddr_in *)addr;

    char buff [512];
    evutil_inet_ntop (AF_INET, &clientaddr->sin_addr, buff, 512);
    printf ("receive a connection from addr:<%s>, port=<%d>\n", 
            buff,
            htons (clientaddr->sin_port));

    pthread_t tid;
    pthread_create (&tid, NULL, thread_func, fd);
}

void listener_errorcb (struct evconnlistener *listener, void *arg)
{
    printf ("error occured when listen\n");
    /* whether there is a way, that listener has error, but you can fix it. */
    struct event_base *base = evconnlistener_get_base (listener);
    event_base_loopexit (base, NULL);
}

/* can not stop the error. address already in use. when restart program immediately. */
void
sig_int(evutil_socket_t fd, short events, void *arg)
{
    struct event_base *base = arg;

    event_base_loopexit (base, NULL);
}


int main (void)
{
    /*
        * step 1 : create a struct event_base object.
        */
    struct event_base *base;
    base = event_base_new ();

    evsignal_new (base, SIGINT, sig_int, base);

    /*
        * step 2 : create the server listener socket.
        */
    evutil_socket_t listenfd;
    listenfd = socket (AF_INET, SOCK_STREAM, 0);
    assert (listenfd > 0);

    /*
        * step 3 : bind the address.
        */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);
    int cc = bind (listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (cc!=0) {
        perror ("bind error");
        return 1;
    }


    /*
        * set socket option, by evutil_* fucntion.
        */
    evutil_make_socket_closeonexec (listenfd);
    evutil_make_socket_nonblocking (listenfd);  /* necessary for evconnlistener_new */

    struct evconnlistener *listener = NULL;
    listener = evconnlistener_new (base, listener_cb, NULL, 
                           LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                           32, listenfd);
    assert (NULL != listener);

    evconnlistener_set_error_cb (listener, listener_errorcb);

    event_base_dispatch (base);

    event_base_free (base);
    evconnlistener_free (listener);

    /* how to get all bufferevent instance, created by listener */
    printf ("done\n");
    return 0;
}



