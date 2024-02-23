/*
 * This is a file system benchmark which attempts to study bottlenecks -
 * it is named 'Bonnie' after Bonnie Raitt, who knows how to use one.
 *
 * Commentary on Bonnie's operations may be found at 
 * http://www.textuality.com/bonnie/intro.html
 *
 * COPYRIGHT NOTICE: 
 * Copyright (c) Tim Bray, 1990-1996.
 *
 * Everybody is hereby granted rights to use, copy, and modify this program, 
 *  provided only that this copyright notice and the disclaimer below
 *  are preserved without change.
 * DISCLAIMER:
 * This program is provided AS IS with no warranty of any kind, and
 * The author makes no representation with respect to the adequacy of this
 *  program for any particular purpose or with respect to its adequacy to 
 *  produce any particular result, and
 * The author shall not be liable for loss or damage arising out of
 *  the use of this program regardless of how sustained, and
 * In no event shall the author be liable for special, direct, indirect
 *  or consequential damage, loss, costs or fees or expenses of any
 *  nature or kind.
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#if defined(SysV)
#include <limits.h>
#include <sys/times.h>
#else
#include <sys/resource.h>
#endif

#include <string.h>
#include <stdlib.h>
#ifdef KERNEL_RTTHREAD
#include <rtthread.h>
#include <finsh.h>
#include "aic_osal.h"
#endif
#include "cpu_usage.h"

#define NO_FORK

#define BON_SPEED(size, type, scale)    \
            (((double) (size)) / (delta[(int)(type)][Elapsed] * (scale)) + 0.4)
#define BON_SPEED_INT(size, type)       (int)BON_SPEED(size, type, 1024.0)
#define BON_SPEED_DEC(size, type)       \
            ((int)(BON_SPEED(size, type, 1024.0) * 100) % 100)
#define BON_SEEK_SPEED_INT(size, type)  (int)BON_SPEED(size, type, 1.0)
#define BON_SEEK_SPEED_DEC(size, type)  \
            ((int)(BON_SPEED(size, type, 1.0) * 100) % 100)

#define IntSize (sizeof(int))

/*
 * N.B. in seeker_reports, CPU appears and Start/End time, but not Elapsed,
 *  so position 1 is re-used; icky data coupling.
 */
#define CPU (0)
#define Elapsed (1)
#define StartTime (1)
#define EndTime (2)
#define Seeks (4000)
#define UpdateSeek (10)
#define SeekProcCount (3)
#define Chunk (16384) /* Chunk needs to be smaller than BufSize */
#define BufSize (131072) /* BufSize can be increased or decreased within a certain range */
#define AutoTestTimes (10)

/* labels for the tests, used as an array index */
typedef enum
{
  Putc, ReWrite, FastWrite, Getc, FastRead, Lseek, TestCount
} tests_t;

/*
 * sizes for the blocks, used as an array index.
 * the added gear needs to be less than BufSize.
*/
typedef enum
{
  S_512B = 512, S_2KB = 2048, S_4KB = 4096, S_8KB = 8192,
  S_16KB = 16384, S_32KB = 32768, S_64KB = 65536
} blocksize_t;
#define BlockSizeTypeNum (7)

typedef struct
{
  int WriteBlock;
  int ReadBlock;
} speed_t;

typedef struct
{
  int argc;
  char **argv;
} bonnie_arg;

static double cpu_so_far();
#ifndef NO_FORK
static void   doseek(off_t where, int fd, int update);
#endif
static void   get_delta_t(tests_t test);
static int    newfile(char * name, int * fd, FILE * * stream, int create);

#define io_error(message) \
{ \
  printf("Bonnie: drastic I/O error (%s)\n", message); \
  return -1; \
}

#if defined(SysV)
/* System V wrappers for randomizers */
static long   random();
static void   srandom(int seed);
#else
long random(void)
{
    return (long)rt_tick_get();
}
#endif

static void   report(char * machine, off_t size);
static void   write_html(char * machine, off_t size);
static double time_so_far();
static void   timestamp();
static void   usage();
static void   auto_test(char *name, off_t size);

/* 
 * Housekeeping variables to build up timestamps for the tests;
 *  global to make it easy to keep track of the progress of time.
 * all of this could have been done with non-global variables,
 *  but the code is easier to read this way and I don't anticipate
 *  much software engineering down the road 
 */
static int    basetime = 0;              /* when we started */
static double delta[(int) TestCount][2]; /* array of DeltaT values */
static double last_cpustamp = 0.0;       /* for computing delta-t */
static double last_timestamp = 0.0;      /* for computing delta-t */
static int    chars[256] = {0};
static int    buf[BufSize / IntSize] __ALIGNED(CACHE_LINE_SIZE) = {0};

int cmd_bonnie(
  int    argc,
  char **argv)
{
  int    bufindex = 0;
#ifndef NO_FORK
  int    child;
#endif
  char * dir;
  int    html = 0;
  int    fd;
#ifndef NO_FORK
  double first_start;
  double last_stop;
  int    lseek_count = 0;
#endif
  char * machine;
  char   name[128] = "";
  int    next;
#ifndef NO_FORK
  int    seek_control[2];
  int    seek_feedback[2];
  char   seek_tickets[Seeks + SeekProcCount];
  double seeker_report[3];
#endif
  off_t  size;
  FILE * stream = NULL;
  off_t  words;
  char *tmp = NULL;
  int   loop = 0, loops = 1;
  char auto_test_flag = 0;

  fd = -1;
#if 0
  basetime = (int) time((time_t *) NULL);
#else
  basetime = 0;
  basetime = (int) time_so_far();
#endif
  size = 1;
  dir = ".";
  machine = "";

  /* pick apart args */
  for (next = 1; next < argc; next++)
    if (strcmp(argv[next], "-d") == 0)
      dir = argv[++next];
    else if (strcmp(argv[next], "-s") == 0)
      size = atoi(argv[++next]);
    else if (strcmp(argv[next], "-m") == 0)
      machine = argv[++next];
    else if (strcmp(argv[next], "-l") == 0)
      loops = atoi(argv[++next]);
    else if (strcmp(argv[next], "-html") == 0)
      html = 1;
    else if (strcmp(argv[next], "-a") == 0)
      auto_test_flag = 1;
    else {
      usage();
      return -1;
    }

  if (size <= 0) {
    usage();
    return -1;
  }

  /* sanity check - 32-bit machines can't handle more than 2047 Mb */
  if (sizeof(off_t) <= 4 && size > 2047)
  {
    fprintf(stderr, "File too large for 32-bit machine, sorry\n");
    return 1;
  }

  if (strlen(machine))
    sprintf(name, "%s/Bonnie.%s.%d", dir, machine, basetime);
  else
    sprintf(name, "%s/Bonnie.%d", dir, basetime);

  /* size is in meg, rounded down to multiple of Chunk */
  size *= (1024 * 1024);
  size = Chunk * (size / Chunk);

  /* auto test - which use some block size to test read write */
  if (auto_test_flag == 1) {
    for (loop = 0; loop < loops; loop++)
    {
      fprintf(stderr, "Auto testing...(%d/%d)", loop + 1, loops);
      auto_test(name, size);
    }
    return 0;
  }

loop_again:
  fprintf(stderr, "%d/%d. Create file '%s', size: %ld MB\n",
          loop + 1, loops, name, size / 1024 / 1024);

  /* Fill up a file, writing it a char at a time with the stdio putc() call */
  fprintf(stderr, "Writing with putc()...");
  if (newfile(name, &fd, &stream, 1))
    io_error("open file");
  timestamp();
  for (words = 0; words < size; words++)
    if (putc(words & 0x7f, stream) == EOF)
      io_error("putc");
  
  /*
   * note that we always close the file before measuring time, in an
   *  effort to force as much of the I/O out as we can
   */
  if (fclose(stream) == -1)
    io_error("fclose after putc");
  get_delta_t(Putc);
  fprintf(stderr, "done. Elapsed %d.%03d\n", (int)delta[Putc][Elapsed],
          (int)(delta[Putc][Elapsed] * 1000) % 1000);

  /* Now read & rewrite it using block I/O.  Dirty one word in each block */
  if (newfile(name, &fd, &stream, 0))
    io_error("open file");
  if (lseek(fd, (off_t) 0, 0) == (off_t) -1)
    io_error("lseek(2) before rewrite");
  fprintf(stderr, "Rewriting one dirty char in each block...");
  timestamp();
  bufindex = 0;
  if ((words = read(fd, (char *) buf, Chunk)) == -1)
    io_error("rewrite read");
  while (words == Chunk)
  { /* while we can read a block */
    if (bufindex == Chunk / IntSize)
      bufindex = 0;
    buf[bufindex++]++;
    if (lseek(fd, (off_t) -words, 1) == -1)
      io_error("relative lseek(2)");
    if (write(fd, (char *) buf, words) == -1)
      io_error("re write(2)");
    if ((words = read(fd, (char *) buf, Chunk)) == -1)
      io_error("rwrite read");
  } /* while we can read a block */
  if (fclose(stream) == -1)
    io_error("close after rewrite");
  get_delta_t(ReWrite);
  fprintf(stderr, "done. Elapsed %d.%03d\n", (int)delta[ReWrite][Elapsed],
          (int)(delta[ReWrite][Elapsed] * 1000) % 1000);

  /* Write the whole file from scratch, again, with block I/O */
  if (newfile(name, &fd, &stream, 1))
    io_error("open file");
  fprintf(stderr, "Writing with block(%d)...", Chunk);
  tmp = (char *)buf;
  for (words = 0; words < Chunk; words++)
    tmp[words] = words & 0x7f;
  timestamp();
  for (words = bufindex = 0; words < (size / Chunk); words++)
  { /* for each word */
    if (bufindex == (Chunk / IntSize))
      bufindex = 0;
    buf[bufindex++]++;
    if (write(fd, (char *) buf, Chunk) == -1)
      io_error("write(2)");
  } /* for each word */
  if (fclose(stream) == -1)
    io_error("close after fast write");
  get_delta_t(FastWrite);
  fprintf(stderr, "done. Elapsed %d.%03d\n", (int)delta[FastWrite][Elapsed],
          (int)(delta[FastWrite][Elapsed] * 1000) % 1000);

  /* read them all back with getc() */
  if (newfile(name, &fd, &stream, 0))
    io_error("open file");
  for (words = 0; words < 256; words++)
    chars[words] = 0;
  fprintf(stderr, "Reading with getc()...");
  timestamp();
  for (words = 0; words < size; words++)
  { /* for each byte */
    if ((next = getc(stream)) == EOF)
      io_error("getc(3)");

    /* just to fool optimizers */
    chars[next]++;
  } /* for each byte */
  if (fclose(stream) == -1)
    io_error("fclose after getc");
  get_delta_t(Getc);
  fprintf(stderr, "done. Elapsed %d.%03d\n", (int)delta[Getc][Elapsed],
          (int)(delta[Getc][Elapsed] * 1000) % 1000);

  /* use the frequency count */
  for (words = 0; words < 256; words++)
    sprintf((char *) buf, "%d", chars[words]);

  /* Now suck it in, Chunk at a time, as fast as we can */
  if (newfile(name, &fd, &stream, 0))
    io_error("open file");
  if (lseek(fd, (off_t) 0, 0) == -1)
    io_error("lseek before read");
  fprintf(stderr, "Reading with block(%d)...", Chunk);
  timestamp();
  do
  { /* per block */
    if ((words = read(fd, (char *) buf, Chunk)) == -1)
      io_error("read(2)");
    chars[buf[abs(buf[0]) % (Chunk / IntSize)] & 0x7f]++;
  } /* per block */
  while (words);
  if (fclose(stream) == -1)
    io_error("close after read");
  get_delta_t(FastRead);
  fprintf(stderr, "done. Elapsed %d.%03d\n", (int)delta[FastRead][Elapsed],
          (int)(delta[FastRead][Elapsed] * 1000) % 1000);

  /* use the frequency count */
  for (words = 0; words < 256; words++)
    sprintf((char *) buf, "%d", chars[words]);

#ifndef NO_FORK
  /*
   * Now test random seeks; first, set up for communicating with children.
   * The object of the game is to do "Seeks" lseek() calls as quickly
   *  as possible.  So we'll farm them out among SeekProcCount processes.
   *  We'll control them by writing 1-byte tickets down a pipe which
   *  the children all read.  We write "Seeks" bytes with val 1, whichever
   *  child happens to get them does it and the right number of seeks get
   *  done.
   * The idea is that since the write() of the tickets is probably
   *  atomic, the parent process likely won't get scheduled while the
   *  children are seeking away.  If you draw a picture of the likely
   *  timelines for three children, it seems likely that the seeks will
   *  overlap very nicely with the process scheduling with the effect
   *  that there will *always* be a seek() outstanding on the file.
   * Question: should the file be opened *before* the fork, so that
   *  all the children are lseeking on the same underlying file object?
   */
  if (pipe(seek_feedback) == -1 || pipe(seek_control) == -1)
    io_error("pipe");
  for (next = 0; next < Seeks; next++)
    seek_tickets[next] = 1;
  for ( ; next < (Seeks + SeekProcCount); next++)
    seek_tickets[next] = 0;

  /* launch some parallel seek processes */
  for (next = 0; next < SeekProcCount; next++)
  { /* for each seek proc */
    if ((child = fork()) == -1)
      io_error("fork");
    else if (child == 0)
    { /* child process */

      /* set up and wait for the go-ahead */
      close(seek_feedback[0]);
      close(seek_control[1]);
      if (newfile(name, &fd, &stream, 0))
        io_error("open file");
#if defined(SysV)
      srandom(getpid());
#endif
      fprintf(stderr, "Seeker %d...", next + 1);

      /* wait for the go-ahead */
      if (read(seek_control[0], seek_tickets, 1) != 1)
	io_error("read ticket");
      timestamp();
      seeker_report[StartTime] = time_so_far();

      /* loop until we read a 0 ticket back from our parent */
      while(seek_tickets[0])
      { /* until Mom says stop */
        doseek((long) (random() % (size / Chunk)), fd,
	  ((lseek_count++ % UpdateSeek) == 0));
	if (read(seek_control[0], seek_tickets, 1) != 1)
	  io_error("read ticket");
      } /* until Mom says stop */
      if (close(fd) == -1)
        io_error("close after seek");

      /* report to parent */
      get_delta_t(Lseek);
      seeker_report[EndTime] = time_so_far();
      seeker_report[CPU] = delta[(int) Lseek][CPU];
      if (write(seek_feedback[1], seeker_report, sizeof(seeker_report))
          != sizeof(seeker_report))
        io_error("pipe write");
      return 0;
    } /* child process */
  } /* for each seek proc */
#endif

  /*
   * Back in the parent; in an effort to ensure the children get an even
   *  start, wait a few seconds for them to get scheduled, open their
   *  files & so on.
   */
#ifndef NO_FORK
  close(seek_feedback[1]);
  close(seek_control[0]);
  sleep(5);
  fprintf(stderr, "start 'em...");
  if (write(seek_control[1], seek_tickets, sizeof(seek_tickets)) 
      != sizeof(seek_tickets))
    io_error("write tickets");
  
  /* read back from children */
  for (next = 0; next < SeekProcCount; next++)
  { /* for each child */
    if (read(seek_feedback[0], (char *) seeker_report, sizeof(seeker_report))
        != sizeof(seeker_report))
      io_error("pipe read");

    /*
     * each child writes back its CPU, start & end times.  The elapsed time 
     *  to do all the seeks is the time the first child started until the 
     *  time the last child stopped
     */
    delta[(int) Lseek][CPU] += seeker_report[CPU];
    if (next == 0)
    { /* first time */
      first_start = seeker_report[StartTime];
      last_stop = seeker_report[EndTime];
    } /* first time */
    else
    { /* not first time */
      first_start = (first_start < seeker_report[StartTime]) ?
	first_start : seeker_report[StartTime]; 
      last_stop = (last_stop > seeker_report[EndTime]) ?
	last_stop : seeker_report[EndTime]; 
    } /* not first time */
    if (wait(&child) == -1)
      io_error("wait");
    fprintf(stderr, "done...");
  } /* for each child */
  fprintf(stderr, "\n");
  delta[(int) Lseek][Elapsed] = last_stop - first_start;
#endif

  if (html)
    write_html(machine, size);
  else
    report(machine, size);

  printf("\nBonnie test done\n\n");

  loop++;
  if (loop < loops)
    goto loop_again;
  return 0;
}

#ifdef KERNEL_RTTHREAD
static void bonnie_thread(
  void *arg)
{
  bonnie_arg para;
  memset(&para, 0, sizeof(bonnie_arg));
  memcpy(&para, arg, sizeof(bonnie_arg));

  fprintf(stderr, "bonnie_thread start!\n");
  cmd_bonnie(para.argc, para.argv);
  fprintf(stderr, "bonnie_thread finished!\n");
}

static void cmd_bonnie_rt(
  int    argc,
  char **argv)
{
  rt_thread_t thid;
  char thread_name[] = "bonnie_rt";
  bonnie_arg para;

  para.argc = argc;
  para.argv = argv;

  thid = aicos_thread_create(thread_name, 8192, 10, bonnie_thread, &para);
  if (thid == NULL) {
    fprintf(stderr, "Failed to create bonnie thread\n");
    return;
  }

  return;
}
#endif

#ifdef KERNEL_RTTHREAD
MSH_CMD_EXPORT_ALIAS(cmd_bonnie_rt, bonnie, File system benchmark);
#else
MSH_CMD_EXPORT_ALIAS(cmd_bonnie, bonnie, File system benchmark);
#endif

static int
block_readwrite_test(
  char *name,
  off_t  size,
  int  chunk_size,
  speed_t *speed)
{
  FILE * stream = NULL;
  int    fd = -1;
  off_t  words;
  char *tmp = NULL;
  int    bufindex = 0;

  /* Write the whole file from scratch, again, with block I/O */
  if (newfile(name, &fd, &stream, 1) < 0)
    io_error("open file");
  tmp = (char *)buf;
  for (words = 0; words < chunk_size; words++)
    tmp[words] = words & 0x7f;
  timestamp();
  for (words = bufindex = 0; words < (size / chunk_size); words++)
  { /* for each word */
    if (bufindex == (chunk_size / IntSize))
      bufindex = 0;
    buf[bufindex++]++;
    if (write(fd, (char *) buf, chunk_size) == -1)
      io_error("write(2)");
  } /* for each word */
  if (fclose(stream) == -1)
    io_error("close after fast write");
  get_delta_t(FastWrite);

  /* use the frequency count */
  for (words = 0; words < 256; words++)
    sprintf((char *) buf, "%d", chars[words]);

  /* Now suck it in, Chunk at a time, as fast as we can */
  if (newfile(name, &fd, &stream, 0) < 0)
    io_error("open file");
  if (lseek(fd, (off_t) 0, 0) == -1)
    io_error("lseek before read");
  timestamp();
  do
  { /* per block */
    if ((words = read(fd, (char *) buf, chunk_size)) == -1)
      io_error("read(2)");
    chars[buf[abs(buf[0]) % (chunk_size / IntSize)] & 0x7f]++;
  } /* per block */
  while (words);
  if (fclose(stream) == -1)
    io_error("close after read");
  get_delta_t(FastRead);

  /* use the frequency count */
  for (words = 0; words < 256; words++)
    sprintf((char *) buf, "%d", chars[words]);

  speed->WriteBlock = BON_SPEED_INT(size, FastWrite);
  speed->ReadBlock = BON_SPEED_INT(size, FastRead);

  return 0;
}

static void
auto_test(
  char *name,
  off_t  size)
{
  int testtimes;
  int testtypenums;
  speed_t testspeed[BlockSizeTypeNum][AutoTestTimes];
  int testtypes[BlockSizeTypeNum] = {S_512B, S_2KB, S_4KB, S_8KB, S_16KB, S_32KB, S_64KB};
  int writeavg[BlockSizeTypeNum] = {0}, readavg[BlockSizeTypeNum] = {0};

  printf("Create file '%s' for test, size: %ld MB\n", name, size / 1024 / 1024);

  for (testtypenums = 0; testtypenums < BlockSizeTypeNum; testtypenums++) {
    for (testtimes = 0; testtimes < AutoTestTimes; testtimes++) {
        fprintf(stderr, "Writing and reading with block(%d)...(%d/%d)", testtypes[testtypenums], testtimes + 1, AutoTestTimes);
        fflush(stdout);
        if (block_readwrite_test(name, size, testtypes[testtypenums], &testspeed[testtypenums][testtimes]) < 0)
           return;
        writeavg[testtypenums] += testspeed[testtypenums][testtimes].WriteBlock;
        readavg[testtypenums] += testspeed[testtypenums][testtimes].ReadBlock;
        fprintf(stderr, "\r");
    }
    writeavg[testtypenums] /= AutoTestTimes;
    readavg[testtypenums] /= AutoTestTimes;
    fprintf(stderr, "\n");
  }

  printf("\nWriter Report\n");
  printf("times\t512B\t2KB\t4KB\t8KB\t16KB\t32KB\t64KB\t\n");
  for (testtimes = 0; testtimes < AutoTestTimes; testtimes++)
    printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", testtimes + 1, testspeed[0][testtimes].WriteBlock,
           testspeed[1][testtimes].WriteBlock, testspeed[2][testtimes].WriteBlock, testspeed[3][testtimes].WriteBlock,
           testspeed[4][testtimes].WriteBlock, testspeed[5][testtimes].WriteBlock, testspeed[6][testtimes].WriteBlock);

  printf("-------------------------------------------------------------\n");
  printf("average\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", writeavg[0], writeavg[1], writeavg[2],
         writeavg[3], writeavg[4], writeavg[5], writeavg[6]);

  printf("\nReader Report\n");
  printf("times\t512B\t2KB\t4KB\t8KB\t16KB\t32KB\t64KB\t\n");
  for (testtimes = 0; testtimes < AutoTestTimes; testtimes++)
    printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", testtimes + 1, testspeed[0][testtimes].ReadBlock,
           testspeed[1][testtimes].ReadBlock, testspeed[2][testtimes].ReadBlock, testspeed[3][testtimes].ReadBlock,
           testspeed[4][testtimes].ReadBlock, testspeed[5][testtimes].ReadBlock, testspeed[6][testtimes].ReadBlock);

  printf("-------------------------------------------------------------\n");
  printf("average\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", readavg[0], readavg[1], readavg[2],
         readavg[3], readavg[4], readavg[5], readavg[6]);

}

static void
write_html(
  char * machine,
  off_t  size)
{

  printf("<TR><TD>%s</TD><TD>%ld</TD>", machine, size / (1024 * 1024));
  printf("<TD>%d</TD><TD>%4.1f</TD><TD>%d</TD><TD>%4.1f</TD><TD>%d</TD><TD>%4.1f</TD>",
    (int) (((double) size) / (delta[(int) Putc][Elapsed] * 1024.0)),
    delta[(int) Putc][CPU] / delta[(int) Putc][Elapsed] * 100.0,
    (int) (((double) size) / (delta[(int) FastWrite][Elapsed] * 1024.0)),
    delta[(int) FastWrite][CPU] / delta[(int) FastWrite][Elapsed] * 100.0,
    (int) (((double) size) / (delta[(int) ReWrite][Elapsed] * 1024.0)),
    delta[(int) ReWrite][CPU] / delta[(int) ReWrite][Elapsed] * 100.0);
  printf("<TD>%d</TD><TD>%4.1f</TD><TD>%d</TD><TD>%4.1f</TD>",
    (int) (((double) size) / (delta[(int) Getc][Elapsed] * 1024.0)),
    delta[(int) Getc][CPU] / delta[(int) Getc][Elapsed] * 100.0,
    (int) (((double) size) / (delta[(int) FastRead][Elapsed] * 1024.0)),
    delta[(int) FastRead][CPU] / delta[(int) FastRead][Elapsed] * 100.0);
  printf("<TD>%5.1f</TD><TD>%4.1f</TD></TR>\n",
    ((double) Seeks) / delta[(int) Lseek][Elapsed],
    delta[(int) Lseek][CPU] / delta[(int) Lseek][Elapsed] * 100.0);
}

static void
report(
  char * machine,
  off_t  size)
{
  printf("------------- ");
  printf(
    "--------Sequential Write-------- ---Sequential Read--- --Random--\n");
  printf("              ");
  printf(
    "-Per Char- --Block--- -Rewrite-- -Per Char- --Block--- --Seeks---\n");
  printf("------------- ----- ---- ----- ---- ----- ---- ----- ---- ----- ---- ----- ----\n");
  printf("Machine    MB ");
  printf("K/sec %%CPU K/sec %%CPU K/sec %%CPU K/sec %%CPU K/sec ");
  printf("%%CPU  /sec %%CPU\n");
  printf("-------- ---- ----- ---- ----- ---- ----- ---- ----- ---- ----- ---- ----- ----\n");

  printf("%-8.8s %4ld ", machine, size / (1024 * 1024));
  printf("%5d %4d %5d %4d %5d %4d ",
    BON_SPEED_INT(size, Putc), (int)delta[(int) Putc][CPU],
    BON_SPEED_INT(size, FastWrite), (int)delta[(int) FastWrite][CPU],
    BON_SPEED_INT(size, ReWrite), (int)delta[(int) ReWrite][CPU]);
  printf("%5d %4d %5d %4d ",
    BON_SPEED_INT(size, Getc), (int)delta[(int) Getc][CPU],
    BON_SPEED_INT(size, FastRead), (int)delta[(int) FastRead][CPU]);
  printf("%5d %4d\n",
#ifdef NO_FORK
    0, 0);
#else
    BON_SEEK_SPEED_INT(Seeks, Lseek), (int)delta[(int) Lseek][CPU]);
#endif
}

static int
newfile(
  char *   name,
  int *    fd,
  FILE * * stream,
  int      create)
{
  if (create)
  { /* create from scratch */
    *fd = open(name, O_RDWR | O_CREAT);
  } /* create from scratch */
  else
    *fd = open(name, O_RDWR);

  if (*fd == -1)
    io_error(name);
  *stream = fdopen(*fd, "r+");
  if (*stream == NULL)
    io_error("fdopen");

  return 0;
}

static void
usage()
{
  fprintf(stderr,
    "usage: Bonnie [-d scratch-dir] [-s size-in-Mb] [-html] [-m machine-label] [-l loop times]\n \
            Bonnie -a [-d scratch-dir] [-s size-in-Mb] [-m machine-label] [-l loop times]\n");
}

static void
timestamp()
{
  last_timestamp = time_so_far();
  last_cpustamp = cpu_so_far();
}

static void 
get_delta_t(test)
  tests_t test;
{
  int which = (int) test;

  delta[which][Elapsed] = time_so_far() - last_timestamp;
  delta[which][CPU] = cpu_so_far() - last_cpustamp;
}

static double 
cpu_so_far()
{
#if defined(SysV)
  struct tms tms;

  if (times(&tms) == -1)
    io_error("times");
  return ((double) tms.tms_utime) / ((double) sysconf(_SC_CLK_TCK)) +
    ((double) tms.tms_stime) / ((double) sysconf(_SC_CLK_TCK));

#elif defined(LPKG_USING_CPU_USAGE)
  return (double)cpu_load_average();
#else
  struct rusage rusage;

  getrusage(RUSAGE_SELF, &rusage);
  return
    ((double) rusage.ru_utime.tv_sec) +
      (((double) rusage.ru_utime.tv_usec) / 1000000.0) +
        ((double) rusage.ru_stime.tv_sec) +
          (((double) rusage.ru_stime.tv_usec) / 1000000.0);
#endif
}

static double
time_so_far()
{
#if defined(SysV)
  int        val;
  struct tms tms;

  if ((val = times(&tms)) == -1)
    io_error("times");

  return ((double) val) / ((double) sysconf(_SC_CLK_TCK));

#elif defined(KERNEL_RTTHREAD)
  struct timespec now = {0};

  clock_gettime(CLOCK_REALTIME, &now);
  return (double)(now.tv_sec - basetime) + (double)now.tv_nsec / 1000000000;
#else
  struct timeval tp;

  if (gettimeofday(&tp, (struct timezone *) NULL) == -1)
    io_error("gettimeofday");
  return ((double) (tp.tv_sec - basetime)) +
    (((double) tp.tv_usec) / 1000000.0);
#endif
}

#ifndef NO_FORK
/*
 * Do a typical-of-something random I/O.  Any serious application that
 *  has a random I/O bottleneck is going to be smart enough to operate
 *  in a page mode, and not stupidly pull individual words out at
 *  odd offsets.  
 * The 'where' argument is used as a chunk number
 * To keep the cache from getting too clever, some pages must be updated.  
 *  However an application that updated each of many random pages that 
 *  it looked at is hard to imagine.  
 * However, it would be wrong to put the update percentage in as a
 *  parameter - the effect is too nonlinear.
 */
static void 
doseek(
  off_t where,
  int   fd,
  int   update)
{
  int   buf[Chunk / IntSize];
  off_t probe;
  off_t size;

  probe = where * Chunk;
  if (lseek(fd, probe, 0) != probe)
    io_error("lseek in doseek");
  if ((size = read(fd, (char *) buf, Chunk)) == -1)
    io_error("read in doseek");

  /* every so often, update a block */
  if (update)
  { /* update this block */

    /* touch a word */
    buf[((int) random() % (size/IntSize - 2)) + 1]--;
    if (lseek(fd, (long) probe, 0) != probe)
      io_error("lseek in doseek update");
    if (write(fd, (char *) buf, size) == -1)
      io_error("write in doseek");
  } /* update this block */
}
#endif

#if defined(SysV)
static char randseed[32];

static void
srandom(int seed)
{
  sprintf(randseed, "%06d", seed);
}

static long
random()
{
  return nrand48(randseed);
}
#endif

