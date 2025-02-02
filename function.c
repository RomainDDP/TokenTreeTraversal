#include "function.h"

void creerRepertoire(int x, int y) {
  char tmp[4];
  sprintf(tmp, "%d,%d", x, y);
  mkdir(tmp, 0777);
}

double mesurerTemps(void) {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  return current_time.tv_usec / 10 ^ 3;
}

void creerFichier(int x, int y, int visite) {
  // Declaration of variables
  char name_file[20];
  char second_name_file[20];
  char time[50];
  char second_time[50];
  int file;
  int second_file;
  double value;
  char buf[255];
  int charlu;

  if (visite == 1) {
    sprintf(name_file, "./%d,%d/visite_%d", x, y, visite);
    file = open(name_file, O_RDWR | O_CREAT, 0777);
    sprintf(time, "%lf", mesurerTemps());
    write(file, &time, sizeof(double));
    close(file);
    visite++;
  } else {
    sprintf(name_file, "./%d,%d/visite_%d", x, y, visite);
    file = open(name_file, O_RDWR | O_CREAT, 0770);
    sprintf(second_name_file, "./%d,%d/visite_%d", x, y,
            (visite - (visite - 1)));
    second_file = open(second_name_file, O_RDWR | O_CREAT, 0777);

    while ((charlu = read(second_file, buf, 255)) != 0) {
      value = atof(buf);
    }

    sprintf(second_time, "%lf \n", mesurerTemps() - value);
    write(file, &second_time, sizeof(double));
    // write(file, "le pid du processus est %d", getpid());
    close(file);
    close(second_file);
  }
}
