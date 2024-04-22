// Copyright (c) 2020 Cesanta Software Limited
// All rights reserved

#include <signal.h>
#include "mongoose.h"

static int s_debug_level = MG_LL_INFO;
static const char *s_root_dir = ".";
static const char *s_listening_address = "http://0.0.0.0:8000";
static const char *s_enable_hexdump = "no";
static const char *s_ssi_pattern = "#.html";
static const char *s_upload_dir = "/upload";  // File uploads disabled by default if set to NULL

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}

// Handle the CORS preflight request to solve the failed .m3u8 request
// Note that the asterisk * should be replaced with an actual origin in a production environment
// Since not doing so is a security vulnerability :(
static void set_cors_headers(struct mg_connection *c) {
    mg_http_reply(c, 204, "Access-Control-Allow-Origin: *\n"
                          "Access-Control-Allow-Methods: *\n"
                          "Access-Control-Allow-Headers: *\n"
                          "Access-Control-Expose-Headers: Content-Length,Content-Range", "");
}




// Event handler for the listening connection.
// Simply serve static files from `s_root_dir`
static void cb(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;

    // print the hm struct
   // printf("hm->method: %.*s\n", (int)hm->method.len, hm->method.ptr);

    //print just the hm struct:
    printf("hm: %.*s\n", (int)hm->message.len, hm->message.ptr);

    if (mg_vcmp(&hm->method, "OPTIONS") == 0) {
          printf("Pre-flight OPTIONS request received\n");
    
          // Returns the Released CORS (ALL)
          mg_http_reply(c, 204, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: *\r\n\r\n",
                        "No Content");
    } 

    // // Handle OPTIONS requests for CORS preflight
    // if (mg_http_match_uri(hm, "*")) {
    //   printf("\n\nMethod: %.*s\n", (int)hm->method.len, hm->method.ptr);
    //   if (mg_match(hm->method, mg_str("OPTIONS"), NULL)) {
    //           printf("OPTIONS request\n\n");
    //           set_cors_headers(c);
    //           return; // Stop further processing of this preflight request
    //       }
    // }


    if (mg_match(hm->uri, mg_str("/upload"), NULL)) {
      printf("Upload request\n");
      // Serve file upload
      if (s_upload_dir == NULL) {
        mg_http_reply(c, 403, "", "Denied: file upload directory not set\n");
      } else {
        struct mg_http_part part;
        size_t pos = 0, total_bytes = 0, num_files = 0;
        while ((pos = mg_http_next_multipart(hm->body, pos, &part)) > 0) {
          char path[MG_PATH_MAX];
          MG_INFO(("Chunk name: [%.*s] filename: [%.*s] length: %lu bytes",
                   part.name.len, part.name.ptr, part.filename.len,
                   part.filename.ptr, part.body.len));
          mg_snprintf(path, sizeof(path), "%s/%.*s", s_upload_dir,
                      part.filename.len, part.filename.ptr);
          if (mg_path_is_sane(path)) {
            mg_file_write(&mg_fs_posix, path, part.body.ptr, part.body.len);
            total_bytes += part.body.len;
            num_files++;
          } else {
            MG_ERROR(("Rejecting dangerous path %s", path));
          }
        }
        mg_http_reply(c, 200, "", "Uploaded %lu files, %lu bytes\n", num_files,
                      total_bytes);
      }
    } 
    else if (mg_http_match_uri(hm, "/hls/*")) {
      printf("hm->uri: %.*s\n", (int)hm->uri.len, hm->uri.ptr);
      char file_path[256];
      snprintf(file_path, sizeof(file_path), "hls/%.*s", (int)hm->uri.len - 5, hm->uri.ptr + 5);

      FILE *fp = fopen(file_path, "wb");
      if (fp != NULL) {
          fwrite(hm->body.ptr, 1, hm->body.len, fp);
          fclose(fp);
          mg_http_reply(c, 200, "", "File stored\n");
      } 
      else {
          mg_http_reply(c, 500, "", "Could not store file\n");
      }
    }
    
    else {
      printf("s_root_dir: %s\n", s_root_dir);
      // Serve web root directory
      struct mg_http_serve_opts opts = {0};
      opts.root_dir = s_root_dir;
      opts.ssi_pattern = s_ssi_pattern;
      mg_http_serve_dir(c, hm, &opts);
      printf("2Pre-flight OPTIONS request received\n");



      // Returns the Released CORS (ALL)
      // mg_http_reply(c, 204, "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: *\r\n\r\n",
      //               "No Content");

      set_cors_headers(c);
    }

    // Log request
    MG_INFO(("%.*s %.*s %lu -> %.*s %lu", hm->method.len, hm->method.ptr,
             hm->uri.len, hm->uri.ptr, hm->body.len, 3, c->send.buf + 9,
             c->send.len));
  }
}

static void usage(const char *prog) {
  fprintf(stderr,
          "Mongoose v.%s\n"
          "Usage: %s OPTIONS\n"
          "  -H yes|no - enable traffic hexdump, default: '%s'\n"
          "  -S PAT    - SSI filename pattern, default: '%s'\n"
          "  -d DIR    - directory to serve, default: '%s'\n"
          "  -l ADDR   - listening address, default: '%s'\n"
          "  -u DIR    - file upload directory, default: unset\n"
          "  -v LEVEL  - debug level, from 0 to 4, default: %d\n",
          MG_VERSION, prog, s_enable_hexdump, s_ssi_pattern, s_root_dir,
          s_listening_address, s_debug_level);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  char path[MG_PATH_MAX] = ".";
  struct mg_mgr mgr;
  struct mg_connection *c;
  int i;

  // Parse command-line flags
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      s_root_dir = argv[++i];
    } else if (strcmp(argv[i], "-H") == 0) {
      s_enable_hexdump = argv[++i];
    } else if (strcmp(argv[i], "-S") == 0) {
      s_ssi_pattern = argv[++i];
    } else if (strcmp(argv[i], "-l") == 0) {
      s_listening_address = argv[++i];
    } else if (strcmp(argv[i], "-u") == 0) {
      s_upload_dir = argv[++i];
    } else if (strcmp(argv[i], "-v") == 0) {
      s_debug_level = atoi(argv[++i]);
    } else {
      usage(argv[0]);
    }
  }

  // Root directory must not contain double dots. Make it absolute
  // Do the conversion only if the root dir spec does not contain overrides
  if (strchr(s_root_dir, ',') == NULL) {
    realpath(s_root_dir, path);
    s_root_dir = path;
  }

  // Initialise stuff
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  mg_log_set(s_debug_level);
  mg_mgr_init(&mgr);
  if ((c = mg_http_listen(&mgr, s_listening_address, cb, &mgr)) == NULL) {
    MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT",
              s_listening_address));
    exit(EXIT_FAILURE);
  }
  if (mg_casecmp(s_enable_hexdump, "yes") == 0) c->is_hexdumping = 1;

  // Start infinite event loop
  MG_INFO(("Mongoose version : v%s", MG_VERSION));
  MG_INFO(("Listening on     : %s", s_listening_address));
  MG_INFO(("Web root         : [%s]", s_root_dir));
  MG_INFO(("Upload dir       : [%s]", s_upload_dir ? s_upload_dir : "unset"));
  while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  MG_INFO(("Exiting on signal %d", s_signo));
  return 0;
}
