#include <signal.h>
#include "mongoose.h"

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}

// static const char *s_url = "ws://localhost:8000/websocket";
static const char *s_url = "ws://localhost:8080/websocket";

// Print websocket response and signal that we're done
static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_OPEN) {
    c->is_hexdumping = 1;
  }
  else if (ev == MG_EV_ERROR) {
    // On error, log error message
    MG_ERROR(("%p %s", c->fd, (char *) ev_data));
  }
  else if (ev == MG_EV_WS_OPEN) {
    // When websocket handshake is successful, send message
    mg_ws_send(c, "hello", 5, WEBSOCKET_OP_TEXT);
  }
  else if (ev == MG_EV_WS_MSG) {
    // When we get echo response, print it
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    printf("GOT ECHO REPLY: [%.*s]\n", (int) wm->data.len, wm->data.ptr);

    // Store echo response in a buffer that can be read by hdmi.html to visualize the data
    FILE *fp;
    fp = fopen("./web_root/test.txt", "w");
    fprintf(fp, "%.*s", (int) wm->data.len, wm->data.ptr);
    fclose(fp);
  }


  if (ev == MG_EV_ERROR || ev == MG_EV_CLOSE || ev == MG_EV_WS_MSG) {
    printf("Data Transfer Finished. Now Streaming HDMI...\n");
    *(bool *) c->fn_data = true;  // Signal that we're done
  }
}

// HTTP request handler function. It implements the following endpoints:
//   /api/video1 - hangs forever, returns MJPEG video stream
//   all other URI - serves web_root/ directory
static void cb(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_http_match_uri(hm, "/api/video1")) {
      c->data[0] = 'S';  // Mark that connection as live streamer
      mg_printf(
          c, "%s",
          "HTTP/1.0 200 OK\r\n"
          "Cache-Control: no-cache\r\n"
          "Pragma: no-cache\r\nExpires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
          "Content-Type: multipart/x-mixed-replace; boundary=--foo\r\n\r\n");
    } else {
      struct mg_http_serve_opts opts = {.root_dir = "web_root"};
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
}

int main(void) {
  struct mg_mgr mgr;        // Event manager
  bool done = false;        // Event handler flips it to true
  struct mg_connection *c;  // Client connection
  mg_mgr_init(&mgr);        // Initialise event manager
  mg_log_set(MG_LL_DEBUG);  // Set log level

  // Initialise signal interrupts
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Connect to websocket server to get pixel buffer data
  c = mg_ws_connect(&mgr, s_url, fn, &done, NULL);     // Create client
  while (c && done == 0) mg_mgr_poll(&mgr, 1000);  // Wait for echo
  mg_mgr_free(&mgr);                                   // Deallocate resources

  // Initialize HDMI drawing server
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, "http://localhost:8000", cb, NULL);
  while(s_signo == 0) mg_mgr_poll(&mgr, 1000);
  MG_INFO(("Exiting data_transfer on signal %d", s_signo));
  mg_mgr_free(&mgr);

  return 0;
}
