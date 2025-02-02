#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "function.h"

#define RD 0
#define WR 1

void fct(); // Pour y voir un peu plus clair dans le main. Fonction exec par le
            // processus père.

void faire_arbre(int, int); // Notre fonction avec appel récursive pour créer
                            // notre arbre de processus.

int depth;       // Profondeur de notre arbre de processus.
int nb_proc = 2; // Nombre ttl de processus
int token;       // Notre jeton que l'on va faire passer dans nos processus.
int visite;      // Utilisée pour nos fichiers.

int running; // Pour la gestion de nos signaux

int *pid_fils; // Notre tableau qui va contenir tous les pid de nos processus
               // fils. Celui-ci sera utilisé dans pour propager nos signaux.
int SIZE;      // Taille de notre tableau de pid_fils

const char *nameMem = "/mes_pid";
int mem; // Ma mémoire partagée.

int tubeDesc1[2]; // Tube qui sert à la descente du token
int tubeDesc2[2]; // Second tube qui sert à la descente du token dans les procs
                  // X,0
int tubeMont1[2]; // Tube qui sert à la remontée du token
int tubeMont2[2]; // Tube qui va servir aux remontées dans les X,0

int main(int argc, char *argv[]) {

  running = 1;

  if (argc != 2 || atoi(argv[1]) < 2) {
    printf("Erreur : nombre d'arguments incorrects !\n");
    printf("Usage : ./prog [profondeur] (plus grand que 1)\n");
    exit(1);
  }

  // signal(SIGINT, handle_sigint);

  visite = 1;

  depth = atoi(argv[1]);

  for (int i = 3; i <= depth; i++)
    nb_proc = nb_proc + i;

  SIZE = sizeof(int) * (nb_proc + 1);
  pid_fils = malloc(SIZE);

  mem = shm_open(nameMem, O_CREAT | O_RDWR, 0666);
  if (mem == -1) {
    perror("shm_open");
    exit(0);
  }

/* Le warning de déclaration implicite m'énervait, il n'a pas lieu d'être donc
   je l'empêche d'apparaître J'ai fais la même chose pour kill() dans les
   fonctions handler.
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
  if (ftruncate(mem, SIZE) == -1) {
    perror("ftruncate");
    exit(0);
  };
#pragma GCC diagnostic pop

  pid_fils =
      (int *)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0);
  if (pid_fils == MAP_FAILED) {
    perror("mmap");
    exit(0);
  }

  pid_fils[0] = 1; // Variable pour garder compte d'où on en est dans notre
                   // tableau de PID.
  // Programme...
  token = 1;
  printf("Processus Père %d (1,0)\n", getpid());
  creerRepertoire(1, 0);
  creerFichier(1, 0, 1);

  fct();

  // Fin du programme
  printf("Valeur FINALE du token : %d\n", token);

  printf("Affichage de nos PID fils : \n");
  for (int i = 0; i < nb_proc; i++)
    printf("PID %d : %d / ", i, pid_fils[i]);
  printf("PID : %d\n", pid_fils[nb_proc]);

  munmap(pid_fils, SIZE);
  close(mem);
  shm_unlink(nameMem);

  exit(0);
}

void fct() {
  // Je crée D1 et M1
  pipe(tubeDesc1);
  pipe(tubeMont1);

  // J'écris dans mon tube D1 pour les 1,x
  write(tubeDesc1[WR], &token, sizeof(int));
  faire_arbre(1, 1);
  wait(NULL);
  close(tubeDesc1[RD]);
  close(tubeDesc1[WR]);

  // Je lis le retour des 1,x
  close(tubeMont1[WR]);
  read(tubeMont1[RD], &token, sizeof(int));
  creerFichier(1, 0, 2);
  close(tubeMont1[RD]);
  token++;

  close(tubeMont1[RD]);
  close(tubeMont1[WR]);

  // Je crée D2 et M2
  pipe(tubeDesc2);
  pipe(tubeMont2);

  // J'écris dans mon tube D2 pour 2,0
  write(tubeDesc2[WR], &token, sizeof(int));
  faire_arbre(2, 0);
  wait(NULL);
  close(tubeDesc2[WR]);
  close(tubeDesc2[RD]);

  // Je lis le retour de 2,0
  close(tubeMont2[WR]);
  read(tubeMont2[RD], &token, sizeof(int));
  creerFichier(1, 0, 3);
  close(tubeMont2[RD]);
  token++;
}

void faire_arbre(int x, int y) {

  if (x + y > depth) {
    return;
  }
  sleep(2);
  creerRepertoire(x, y);
  int pid = fork();

  if (pid < 0) {
    printf("Y'a un problème chef\n");
    return;

  } else if (pid == 0) {
    /* Proc fils */
    printf("Processus %d (%d,%d)\n", getpid(), x, y);
    /* Je récupère le pid du fils que je place dans mon tableau. */
    pid_fils[pid_fils[0]++] = getpid();
    ;

    /*
    Feuilles de l'abre, on différencie deux types de feuilles : les X,Y dont la
    somme == depth et la X,0 dont X = depth les X,Y lisent dans D1 et remontent
    dans M1, alors que X,0 lit et remonte dans D2 et M2
    */
    if (x + y == depth) {
      // printf("Je suis la feuille.\n");
      if (x == depth) {
        close(tubeDesc2[WR]);
        close(tubeMont2[RD]);
        read(tubeDesc2[RD], &token, sizeof(int));
        printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
        close(tubeDesc2[RD]);
        token++;

        write(tubeMont2[WR], &token, sizeof(int));
        close(tubeMont2[WR]);
      } else {
        close(tubeDesc1[WR]);
        close(tubeMont1[RD]);
        read(tubeDesc1[RD], &token, sizeof(int));
        printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
        close(tubeDesc1[RD]);
        token++;

        write(tubeMont1[WR], &token, sizeof(int));
        close(tubeMont1[WR]);
      }
      creerFichier(x, y, visite);
    }

    /*
    Croisement dans l'arbre, ceux-ci correspondent aux noeud X,0 (à l'exception
    du dernier où X = depth et est donc une feuille) Ceux-ci font beaucoup de
    choses :
        - Tout d'abord ils lisent dans D2,
        - Ensuite ils créent leur propre D1 et M1 pour leurs fils X,Y
        - Ils écrivent dans D1 et attendent la fin de leur processus fils pour
    lire dans M1
        - Puis ils descendent le token dans D2
        - Ils attendent la fin de leur procs fils pour lire M2
        - Enfin ils remontent le token dans M2
    */
    else if (y == 0 && x != depth) { // le x != depth n'est pas nécessaire, mais
                                     // je le garde comme mesure de sécurité.
      // printf("Je suis un croisement.\n");
      read(tubeDesc2[RD], &token, sizeof(int));
      creerFichier(x, y, visite++);
      printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
      token++;
      pipe(tubeDesc1);
      pipe(tubeMont1);

      write(tubeDesc1[WR], &token, sizeof(int));
      faire_arbre(x, y + 1);
      wait(NULL);
      close(tubeDesc1[RD]);
      close(tubeDesc1[WR]);
      close(tubeMont1[WR]);

      read(tubeMont1[RD], &token, sizeof(int));
      creerFichier(x, y, visite++);
      printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
      close(tubeMont1[RD]);
      token++;

      write(tubeDesc2[WR], &token, sizeof(int));
      faire_arbre(x + 1, 0);
      wait(NULL);
      close(tubeDesc2[RD]);
      close(tubeDesc2[WR]);

      read(tubeMont2[RD], &token, sizeof(int));
      creerFichier(x, y, visite++);
      printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
      token++;
      write(tubeMont2[WR], &token, sizeof(int));

      close(tubeMont2[RD]);
      close(tubeMont2[WR]);
    }
    /*
    Il s'agit ici de noeuds intermédaires. Ceux ci vont uniquement utiliser D1
    et M1. Ils vont lire D1, puis réécrire dedans la nouvelle valeur de token
    Ensuite ils vont attendre la fin de leur fils pour lire M1 et réécrire
    dedans.
    */
    else {
      // printf("Je suis un noeud intermédiaire.\n");

      read(tubeDesc1[RD], &token, sizeof(int));
      creerFichier(x, y, visite++);
      printf("Valeur du token reçu (%d,%d): %d\n", x, y, token);
      token++;

      write(tubeDesc1[WR], &token, sizeof(int));
      faire_arbre(x, y + 1);
      wait(NULL);

      read(tubeMont1[RD], &token, sizeof(int));
      creerFichier(x, y, visite++);
      token++;
      write(tubeMont1[WR], &token, sizeof(int));
    }

    exit(0);
  } else {
    // printf("Processus %d a crée le processus fils %d\n",getpid(), pid);
  }
}
