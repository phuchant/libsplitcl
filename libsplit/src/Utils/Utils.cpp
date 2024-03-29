#include <Utils/Utils.h>

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/time.h>

namespace libsplit {

  size_t file_size(const char *filename)
  {
    struct stat sb;

    if (stat(filename, &sb) < 0) {
      perror("stat");
      abort();
    }
    return sb.st_size;
  }

  unsigned char *binary_load(const char *filename, size_t *size)
  {
    FILE *f;
    unsigned char *b;
    size_t r;

    *size = file_size(filename);
    b = (unsigned char *) malloc(*size);
    if (!b) {
      perror("malloc");
      exit(1);
    }
    f = fopen(filename, "r");
    if (f == NULL) {
      perror("fopen");
      exit(1);
    }
    r = fread(b, *size, 1, f);
    if (r != 1) {
      printf("binary load error\n");
      perror("fread");
      exit(1);
    }

    fclose(f);

    return b;
  }

  char *file_load(const char *filename) {
    FILE *f;
    char *b;
    size_t s;
    size_t r;

    s = file_size (filename);
    b = (char *) malloc (s+1);
    if (!b) {
      perror ("malloc");
      exit (1);
    }
    f = fopen (filename, "r");
    if (f == NULL) {
      perror ("fopen");
      exit (1);
    }
    r = fread (b, s, 1, f);
    if (r != 1) {
      printf("file load error %s\n", filename);

      perror ("fread");
      exit (1);
    }
    b[s] = '\0';

    fclose(f);

    return b;
  }

  double get_time()
  {
    struct timezone Tzp;
    struct timeval Tp;
    int stat;
    stat = gettimeofday (&Tp, &Tzp);
    if (stat != 0) printf("Error return from gettimeofday: %d",stat);
    return(Tp.tv_sec + Tp.tv_usec*1.0e-6);
  }

}
