#include "domain_sockets.h"
#include <stdarg.h>

static char output_name[MAX_USERNAME + 20];

static inline void log_message(const char *format, ...) {
  // Open file in append mode
  FILE *output_file = fopen(output_name, "a");
  if (!output_file) {
    perror("Error opening log file for writing");
    return;
  }

  // Create a va_list to handle variable arguments
  va_list args;
  va_start(args, format);

  // Use vfprintf to process variable arguments like log_message
  vfprintf(output_file, format, args);
  if (format[strlen(format) - 1] != '\n') {
    fprintf(output_file, "\n");
  }

  // Clean up the va_list and close the file
  va_end(args);
  fclose(output_file);
}

static inline void create_file(char *username) {
  // Create output file
  strncpy(output_name, username, MAX_USERNAME - 1);
  strncat(output_name, ".txt", 5);
  FILE *output_file = fopen(output_name, "w");
  if (!output_file) {
    perror("Failed to open log file");
    return;
  }
  fclose(output_file);
}
