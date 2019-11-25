#include "klbvfs.h"


#define ORIGVFS(p) ((sqlite3_vfs*)((p)->pAppData))

static int klbOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *outFlags);
static int klbDelete(sqlite3_vfs *pVfs, const char *zPath, int dirSync);
static int klbDelete(sqlite3_vfs *pVfs, const char *zPath, int dirSync);
static int klbAccess(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut);
static int klbFullPathname(sqlite3_vfs *pVfs, const char *zName, int nOut, char *zOut);
static void *klbDlOpen(sqlite3_vfs *pVfs, const char *zPath);
static void klbDlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg);
static void klbDlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg);
static void (*klbDlSym(sqlite3_vfs *pVfs, void *p, const char *zSym))(void);
static void klbDlClose(sqlite3_vfs *pVfs, void *pHandle);
static int klbRandomness(sqlite3_vfs *pVfs, int nByte, char *zBufOut);
static int klbSleep(sqlite3_vfs *pVfs, int nMicro);
static int klbCurrentTime(sqlite3_vfs *pVfs, double *pTimeOut);
static int klbGetLastError(sqlite3_vfs *pVfs, int a, char *b);
static int klbCurrentTimeInt64(sqlite3_vfs *pVfs, sqlite3_int64 *p);

static int klbClose(sqlite3_file*);
static int klbRead(sqlite3_file*, void*, int iAmt, sqlite3_int64 iOfst);
static int klbWrite(sqlite3_file*,const void*,int iAmt, sqlite3_int64 iOfst);
static int klbTruncate(sqlite3_file*, sqlite3_int64 size);
static int klbSync(sqlite3_file*, int flags);
static int klbFileSize(sqlite3_file*, sqlite3_int64 *pSize);
static int klbLock(sqlite3_file*, int);
static int klbUnlock(sqlite3_file*, int);
static int klbCheckReservedLock(sqlite3_file*, int *pResOut);
static int klbFileControl(sqlite3_file*, int op, void *pArg);
static int klbSectorSize(sqlite3_file*);
static int klbDeviceCharacteristics(sqlite3_file*);
static int klbShmMap(sqlite3_file*, int iPg, int pgsz, int, void volatile**);
static int klbShmLock(sqlite3_file*, int offset, int n, int flags);
static void klbShmBarrier(sqlite3_file*);
static int klbShmUnmap(sqlite3_file*, int deleteFlag);
static int klbFetch(sqlite3_file*, sqlite3_int64 iOfst, int iAmt, void **pp);
static int klbUnfetch(sqlite3_file*, sqlite3_int64 iOfst, void *p);

static char *str_replace(char *str, char *orig, char *rep);
static char** str_split(char* a_str, const char a_delim);
static uint CalculateKeyOffset(ulong Offset, uint key);



typedef struct KlbFile KlbFile;

struct KlbFile {
  sqlite3_file base;              /* IO methods */
  sqlite3_file *pReal;              /* Size of the file */
  uint key[3];
};


static sqlite3_vfs klb_vfs = {
  2,                           /* iVersion */
  0,                           /* szOsFile (set when registered) */
  1024,                        /* mxPathname */
  0,                           /* pNext */
  "klb_vfs",                    /* zName */
  0,                           /* pAppData (set when registered) */
  klbOpen,                     /* xOpen */
  klbDelete,                   /* xDelete */
  klbAccess,                   /* xAccess */
  klbFullPathname,             /* xFullPathname */
  klbDlOpen,                   /* xDlOpen */
  klbDlError,                  /* xDlError */
  klbDlSym,                    /* xDlSym */
  klbDlClose,                  /* xDlClose */
  klbRandomness,               /* xRandomness */
  klbSleep,                    /* xSleep */
  klbCurrentTime,              /* xCurrentTime */
  klbGetLastError,             /* xGetLastError */
  klbCurrentTimeInt64          /* xCurrentTimeInt64 */
};

static const sqlite3_io_methods mem_io_methods = {
  3,                              /* iVersion */
  klbClose,                      /* xClose */
  klbRead,                       /* xRead */
  klbWrite,                      /* xWrite */
  klbTruncate,                   /* xTruncate */
  klbSync,                       /* xSync */
  klbFileSize,                   /* xFileSize */
  klbLock,                       /* xLock */
  klbUnlock,                     /* xUnlock */
  klbCheckReservedLock,          /* xCheckReservedLock */
  klbFileControl,                /* xFileControl */
  klbSectorSize,                 /* xSectorSize */
  klbDeviceCharacteristics,      /* xDeviceCharacteristics */
  klbShmMap,                     /* xShmMap */
  klbShmLock,                    /* xShmLock */
  klbShmBarrier,                 /* xShmBarrier */
  klbShmUnmap,                   /* xShmUnmap */
  klbFetch,                      /* xFetch */
  klbUnfetch                     /* xUnfetch */
};





static int klbOpen(sqlite3_vfs *pVfs, const char *name, sqlite3_file *file, int flags, int *outFlags)
{



  KlbFile *p = (KlbFile *)file;
  p->pReal = (sqlite3_file *)&p[1];

  char cwd[PATH_MAX];
  char filePath[PATH_MAX];

  getcwd(cwd, sizeof(cwd));

  char currentDirectory[PATH_MAX];
  sprintf(currentDirectory,"%s/",cwd);

  char* output = str_replace((char*)name, currentDirectory, "");


  char** underScoreTokens;
  char** dotTokens;

  underScoreTokens = str_split(output, '_');
  if(!*(underScoreTokens + 1))
  {
    return SQLITE_INTERNAL;
  }

  dotTokens = str_split(*(underScoreTokens), '.');
  if(!*(dotTokens + 2))
  {
    return SQLITE_INTERNAL;
  }

  for (int i = 0; *(dotTokens + i); i++)
  {
      p->key[i] = atol(*(dotTokens + i));
      free(*(dotTokens + i));
  }

  int retCode = ORIGVFS(pVfs)->xOpen(ORIGVFS(pVfs), *(underScoreTokens + 1), p->pReal, flags, outFlags);
  if( p->pReal->pMethods )
  {
    file->pMethods = &mem_io_methods;
  }

  return retCode;
}

static int klbDelete(sqlite3_vfs *pVfs, const char *zPath, int dirSync){
  return ORIGVFS(pVfs)->xDelete(ORIGVFS(pVfs), zPath, dirSync);
}


static int klbAccess(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut){
  return ORIGVFS(pVfs)->xAccess(ORIGVFS(pVfs), zName, flags, pResOut);
}

static int klbFullPathname(sqlite3_vfs *pVfs, const char *zName, int nOut, char *zOut){
  return ORIGVFS(pVfs)->xFullPathname(ORIGVFS(pVfs), zName, nOut, zOut);
}

static void *klbDlOpen(sqlite3_vfs *pVfs, const char *zPath){
  return ORIGVFS(pVfs)->xDlOpen(ORIGVFS(pVfs), zPath);
}

static void klbDlError(sqlite3_vfs *pVfs, int nByte, char *zErrMsg){
  ORIGVFS(pVfs)->xDlError(ORIGVFS(pVfs), nByte, zErrMsg);
}

static void (*klbDlSym(sqlite3_vfs *pVfs, void *p, const char *zSym))(void){
  return ORIGVFS(pVfs)->xDlSym(ORIGVFS(pVfs), p, zSym);
}

static void klbDlClose(sqlite3_vfs *pVfs, void *pHandle){
  ORIGVFS(pVfs)->xDlClose(ORIGVFS(pVfs), pHandle);
}

static int klbRandomness(sqlite3_vfs *pVfs, int nByte, char *zBufOut){
  return ORIGVFS(pVfs)->xRandomness(ORIGVFS(pVfs), nByte, zBufOut);
}

static int klbSleep(sqlite3_vfs *pVfs, int nMicro){
  return ORIGVFS(pVfs)->xSleep(ORIGVFS(pVfs), nMicro);
}

static int klbCurrentTime(sqlite3_vfs *pVfs, double *pTimeOut){
  return ORIGVFS(pVfs)->xCurrentTime(ORIGVFS(pVfs), pTimeOut);
}

static int klbGetLastError(sqlite3_vfs *pVfs, int a, char *b){
  return ORIGVFS(pVfs)->xGetLastError(ORIGVFS(pVfs), a, b);
}

static int klbCurrentTimeInt64(sqlite3_vfs *pVfs, sqlite3_int64 *p){
  return ORIGVFS(pVfs)->xCurrentTimeInt64(ORIGVFS(pVfs), p);
}


int klbvfs_register()
{
  sqlite3_vfs *root;

  if ((root = sqlite3_vfs_find(NULL)) == NULL)
	{
		return SQLITE_NOTFOUND;
	}

  klb_vfs.szOsFile = 1000;
  klb_vfs.pAppData = root;

	return sqlite3_vfs_register(&klb_vfs, 1);

}

static uint CalculateKeyOffset(ulong Offset, uint key)
{
  uint seed1 = SEED_1;
  uint seed2 = SEED_2;
  ulong offset = Offset;
  uint a14 = 1;
  uint a15 = 0;
  do
  {
    if((offset & 1) > 0)
    {
      a15 += a14 * seed2;
      a14 *= seed1;
    }
    offset = offset >> 1;
    seed2 +=  seed1 * seed2;
    seed1 *= seed1;
  } while(offset > 0);
  return a15 + a14 * key;
}


static int klbClose(sqlite3_file* file)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xClose(p->pReal);
}



static int klbRead(sqlite3_file* file, void* zBuf, int iAmt, sqlite3_int64 iOfst)
{
  KlbFile *p = (KlbFile *)file;
  int readResult = p->pReal->pMethods->xRead(p->pReal, zBuf, iAmt, iOfst);

  uint key[3] = {0,0,0};

  for(int i = 0; i < 3; i++)
  {
    if(iOfst > 0)
    {
      key[i] = CalculateKeyOffset(iOfst, p->key[i]);
    }
    else
    {
      key[i] = p->key[i];
    }
  }

  if( iAmt > 0 )
  {
    uint bufferIndex = 0;
    do
    {
      --iAmt;
      *((char *)zBuf + bufferIndex) = *(char*)(zBuf + bufferIndex) ^ ((char)((key[1] ^ key[0] ^ key[2]) >> 24));
      bufferIndex++;
      key[1] = key[1] * SEED_1 + SEED_2;
      key[0] = key[0] * SEED_1 + SEED_2;
      key[2] = key[2] * SEED_1 + SEED_2;
    } while( iAmt > 0 );
  }

  return readResult;
}
static int klbWrite(sqlite3_file* file,const void* zBuf,int iAmt, sqlite3_int64 iOfst)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xWrite(p->pReal, zBuf, iAmt, iOfst);
}

static int klbTruncate(sqlite3_file* file, sqlite3_int64 size)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xTruncate(p->pReal, size);
}
static int klbSync(sqlite3_file* file, int flags)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xSync(p->pReal, flags);
}
static int klbFileSize(sqlite3_file* file, sqlite3_int64 *pSize)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xFileSize(p->pReal, pSize);
}
static int klbLock(sqlite3_file* file, int eLock)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xLock(p->pReal, eLock);
}
static int klbUnlock(sqlite3_file* file, int eLock)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xUnlock(p->pReal, eLock);
}
static int klbCheckReservedLock(sqlite3_file* file, int *pResOut)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xCheckReservedLock(p->pReal, pResOut);
}
static int klbFileControl(sqlite3_file* file, int op, void *pArg)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xFileControl(p->pReal, op, pArg);
}
static int klbSectorSize(sqlite3_file* file)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xSectorSize(p->pReal);
}
static int klbDeviceCharacteristics(sqlite3_file* file)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xDeviceCharacteristics(p->pReal);
}

static int klbShmMap(sqlite3_file* file, int iRegion, int szRegion, int isWrite, void volatile** pp)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xShmMap(p->pReal, iRegion, szRegion, isWrite, pp);
}

static int klbShmLock(sqlite3_file* file, int offset, int n, int flags)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xShmLock(p->pReal, offset, n, flags);
}

static void klbShmBarrier(sqlite3_file* file)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xShmBarrier(p->pReal);
}

static int klbShmUnmap(sqlite3_file* file, int deleteFlag)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xShmUnmap(p->pReal, deleteFlag);
}

static int klbFetch(sqlite3_file* file, sqlite3_int64 iOfst, int iAmt, void **pp)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xFetch(p->pReal, iOfst, iAmt, pp);
}

static int klbUnfetch(sqlite3_file* file, sqlite3_int64 iOfst, void *pp)
{
  KlbFile *p = (KlbFile *)file;
  return p->pReal->pMethods->xUnfetch(p->pReal, iOfst, pp);
}

char *str_replace(char *str, char *orig, char *rep)
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))
    return str;

  strncpy(buffer, str, p-str);
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }
    count += last_comma < (a_str + strlen(a_str) - 1);
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
