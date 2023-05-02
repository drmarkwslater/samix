#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

void *SharedMemMake(char *nom, size_t taille, int *id);
void *SharedMemMaster(size_t taille, char *dir, char *nom, int *ident);
void *SharedMemLink(char *nom, int id);
void *SharedMemAdrs(char *nom, size_t taille, int *id);
int SharedMemUnlk(char *nom, void *adrs);
int SharedMemKill(char *nom, void *adrs, int id);
void SharedMemRaz(char *nom, int id);
void SharedMemRazAll(int depart, int arrivee);
void *SharedMemLoad(char *nom, int *id, char *path);

#endif
