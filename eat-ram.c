/* Eat RAM to trigger low memory/OOM */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>


#define MAX_CHUNK_COUNT (256)
static char* chunks[MAX_CHUNK_COUNT] = {0};

#define MIN_CHUNK_SIZE (4096)

#define KILOBYTE (1024)
#define MEGABYTE (1048576)
#define GIGABYTE (1073741824)


static void help(const char* argv0)
{
  fprintf(stderr, "Usage: %s [-l LIMIT] [-p PROGRESS]\n", argv0);
  exit(EXIT_FAILURE);
}


static long parse_size(const char* str)
{
  char* end = NULL;
  long value = strtol(str, &end, 0);
  if (value < 0)
  {
    fputs("Size must be positive\n", stderr);
    exit(EXIT_FAILURE);
  }
  switch (end[0])
  {
    case '\0': break;
    case 'k': case 'K': value *= KILOBYTE; break;
    case 'm': case 'M': value *= MEGABYTE; break;
    case 'g': case 'G': value *= GIGABYTE; break;
    default:
      fputs("Unknown size suffix\n", stderr);
      exit(EXIT_FAILURE);
  }
  return value;
}

static const char* format_size(long value)
{
  static char buffer[64]; // Static buffer to allow returning address and avoid too much stack use
  memset(buffer, 0, sizeof(buffer));
  if (value >= GIGABYTE)
    snprintf(buffer, sizeof(buffer) - 1, "%1.1fGB", (double)value / (double)GIGABYTE);
  else if (value >= MEGABYTE)
    snprintf(buffer, sizeof(buffer) - 1, "%1.1fMB", (double)value / (double)MEGABYTE);
  else if (value >= KILOBYTE)
    snprintf(buffer, sizeof(buffer) - 1, "%1.1fkB", (double)value / (double)KILOBYTE);
  else
    snprintf(buffer, sizeof(buffer) - 1, "%1.1fB", (double)value);
  return buffer;
}

static sig_atomic_t got_signal = 0;
static void sig_handler(int sig)
{
  got_signal = sig;
}


int main(int argc, char** argv)
{
  long limit = 0, chunk, allocated = 0, progress = -1, last_progress = 0;
  int opt;
  unsigned chunk_count = 0;
  char* p = NULL;
  struct sigaction action = {
    .sa_handler = sig_handler,
    .sa_flags = SA_RESETHAND
  };

  while ((opt = getopt(argc, argv, "l:p:")) != -1)
  {
    switch (opt)
    {
      case 'l':
        limit = parse_size(optarg);
        break;
      case 'p':
        progress = parse_size(optarg);
        break;
      default:
        help(argv[0]);
        break;
    }
  }

  chunk = limit ? limit / MAX_CHUNK_COUNT : MEGABYTE;
  chunk = (chunk < MIN_CHUNK_SIZE) ? MIN_CHUNK_SIZE : chunk;

  if (progress == -1)
  {
    progress = limit ? limit / 4 : GIGABYTE;
  }

  printf("Allocating using chunks of: %s\n", format_size(chunk));

  while (limit == 0 || (allocated < limit))
  {
    errno = 0;
    p = malloc(chunk);
    if (p == NULL)
    {
      perror("malloc");
      if (chunk >= (MIN_CHUNK_SIZE * 2))
      {
        chunk /= 2;
        printf("Attempting to continue with chunks of: %s\n", format_size(chunk));
        continue;
      }
      if (limit)
        fprintf(stderr, "Failed to reach limit, allocated: %s\n", format_size(allocated));
      else
        fprintf(stderr, "Allocated %s\n", format_size(allocated));
      break;
    }

    if (mlock(p, chunk) != 0)
    {
      perror("mlock");
      free(p);
      if (chunk >= (MIN_CHUNK_SIZE * 2))
      {
        chunk /= 2;
        printf("Attempting to continue with chunks of: %s\n", format_size(chunk));
        continue;
      }
      if (limit)
        fprintf(stderr, "Failed to reach limit, allocated: %s\n", format_size(allocated));
      else
        fprintf(stderr, "Allocated %s\n", format_size(allocated));
      break;
    }

    allocated += chunk;
    memset(p, 0x55, chunk);
    if (chunk_count < MAX_CHUNK_COUNT)
    {
      chunks[chunk_count++] = p;
    }
    if ((progress > 0) && (allocated >= last_progress + progress))
    {
      last_progress = allocated;
      printf("Allocated: %s\n", format_size(allocated));
    }
  }

  puts("Finished allocation, terminate to release memory");

  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  pause();

  printf("\nSignal %d, releasing\n", got_signal);

  while (chunk_count)
  {
    free(chunks[--chunk_count]);
  }

  puts("Finished");
  exit(EXIT_SUCCESS);
}


