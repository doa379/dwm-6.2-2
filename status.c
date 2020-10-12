/* See LICENSE file for license details.
 * Copyright 2017-2020 by doa379
 */

#include <stdbool.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
//#include <linux/input.h>
//include <fcntl.h>
#include <unistd.h>
#include <curl/curl.h>
#include <X11/Xlib.h>
#include "status.h"

#define LENGTH(X)		(sizeof X / sizeof X[0])
#define CPU	"/proc/cpuinfo"
#define MEM	"/proc/meminfo"
#define STAT "/proc/stat"
#define DISKSTAT "/proc/diskstats"
#define NET_ADAPTERS "/proc/net/dev"
#define WIRELESS "/proc/net/wireless"
#define ACPI_ACSTATE "/proc/acpi/ac_adapter/AC/state"
#define ACPI_BAT "/proc/acpi/battery"
#define SYS_PS "/sys/class/power_supply"
#define SYS_ACSTATE "/sys/class/power_supply/AC/online"
#define SND_CMD "fuser -v /dev/snd/* 2>&1 /dev/zero"
#define DEVICES "/proc/bus/input/devices"
//#define MAX_NFD 4
//#define DEVICE_TYPES "Keyboard", "keyboard", "Lid", "Sleep", "Power"
//#define DEVICE_TYPES "Lid",
#define kB			1024
#define mB			(kB * kB)
#define gB			(kB * mB)

typedef struct
{
  unsigned long user,
                nice,
                system,
                idle,
                iowait,
                irq,
                softirq;
  unsigned processor;
  float perc, mhz;
} cpu_t;

typedef struct
{
  unsigned long total,
                free,
                available,
                buffers,
                cached,
                swapcached,
                swaptotal,
                swapfree;
  float perc, swap;
} mem_t;

typedef struct
{
  char DEVNAME[16];
  unsigned long rd_sec_or_wr_ios, wr_sec;
  const char *blkdev;
  float readkbs, writekbs;
} diskstats_t;

typedef struct
{
  char IFNAME[8];
  unsigned long RXbytes, TXbytes;
  const char *netif;
  float downkbytes, upkbytes;
} net_t;

typedef struct
{
  char SSID[16];
  struct iwreq wreq;
  int sockfd;
} ssid_t;

typedef struct
{
  char IFNAME[8];
  float link, level, noise;
  ssid_t ssid;
  net_t *net;
} wireless_t;

typedef struct
{
  CURL *handle;
  char BUFFER[64], PREV[64];
} ip_t;

typedef struct
{
  unsigned capacity,
           remaining,
           rate, 
           perc;
  char BAT[8], STATEFILE[48], state;
} battery_t;

typedef struct
{
  unsigned NBAT, total_perc;
  battery_t BATTERY[MAX_BATTERIES];
} batteries_t;
/*
typedef struct
{
  unsigned NFD, count;
  char DEV[MAX_NFD][32];
  struct pollfd PFD[MAX_NFD];
} device_t;
*/
cpu_t cpu;
mem_t mem;
diskstats_t DISKSTATS[LENGTH(BLKDEV)];
net_t NET[LENGTH(NETIF)];
wireless_t WLAN[LENGTH(NETIF)];
ip_t ip;
bool ac_state;
batteries_t batteries;
char SND[16], TIME[32];
//device_t device;
static unsigned char interval = UPDATE_INTV;
//const char *SEARCH_TERMS[] = { DEVICE_TYPES };

static void read_file(void *data, void (*cb)(), const char FILENAME[])
{
  FILE *fp = fopen(FILENAME, "r");
  if (!fp)
    return;

  char LINE[STRLEN];
  while (fgets(LINE, sizeof LINE, fp))
    cb(data, LINE);

  fclose(fp);
}

static void read_dir(void *data, void (*cb)(), const char DIRNAME[])
{
  DIR *dp = opendir(DIRNAME);
  if (!dp)
    return;

  struct dirent *d;
  while ((d = readdir(dp)))
    if (strcmp(d->d_name, ".") && strcmp(d->d_name, ".."))
      cb(data, d->d_name);

  closedir(dp);
}

static char *format_units(float val)
{ /* Function expects numeric to be in kB units */
  static char STRING[8];
  if (val * kB < kB)
    sprintf(STRING, "%.0fB", val * kB);
  else if (val * kB > kB - 1 && val * kB < mB)
    sprintf(STRING, "%.0fK", val);
  else if (val * kB > mB - 1 && val * kB < gB)
    sprintf(STRING, "%.1fM", val / kB);
  else if (val * kB > gB - 1)
    sprintf(STRING, "%.1fG", val / mB);

  return STRING;
}

static void tail(char LINE[], size_t size, const char FILENAME[], unsigned char n)
{
  FILE *fp = fopen(FILENAME, "r");
  if (!fp)
    return;

  char err;
  fseek(fp, -sizeof err, SEEK_END);
  unsigned i = 0;
  while (i < n && !(err = fseek(fp, -2 * sizeof err, SEEK_CUR)))
    if (fgetc(fp) == '\n')
      i++;

  if (err)
    fseek(fp, 0L, SEEK_SET);

  if (fgets(LINE, size, fp))
    LINE[strlen(LINE) - 1] = '\0';
  fclose(fp);
}
/*
static void parse_dev_cb(void *data, char LINE[])
{
  device_t *device = data;
  if (!strncmp("N:", LINE, 2))
  {
    for (unsigned i = 0; i < LENGTH(SEARCH_TERMS); i++)
      if (strstr(LINE, SEARCH_TERMS[i]))
        device->NFD++;
  }

  else if (!strncmp("H:", LINE, 2) && device->count < device->NFD)
  {
    char *p;
    if ((p = strstr(LINE, "event")))
    {
      char EV[7];
      sscanf(p, "%s", EV);
      sprintf(device->DEV[device->NFD - 1], "/dev/input/%s", EV);
      device->count++;
    }
  }
}

static void deinit_device(device_t *device)
{
  for (unsigned i = 0; i < device->NFD; i++)
    close(device->PFD[i].fd);
}

static void init_device(device_t *device)
{
  read_file(device, parse_dev_cb, DEVICES);
  for (unsigned i = 0; i < device->NFD; i++)
  {
    device->PFD[i].fd = open(device->DEV[i], O_RDONLY);
    device->PFD[i].events = POLLIN;
  }
}
*/
typedef struct
{
  char *node;
  const char *type;
  bool found;
} device_t;

static void parse_dev_cb(void *data, char LINE[])
{
  device_t *device = data;
  if (!strncmp("N:", LINE, 2) && strstr(LINE, device->type))
    device->found = 1;

  else if (!strncmp("H:", LINE, 2) && device->found)
  {
    char *p;
    if ((p = strstr(LINE, "event")))
    {
      char EV[8];
      sscanf(p, "%s", EV);
      sprintf(device->node, "/dev/input/%s", EV);
    }

    device->found = 0;
  }
}

void input_event_node(char NODE[], const char TYPE[])
{
  device_t device = { NODE, TYPE, 0 };
  read_file(&device, parse_dev_cb, DEVICES);
}

static void date(char TIME[], size_t size)
{
  time_t t = time(NULL);
  strftime(TIME , size, "%R %a %d %b", localtime(&t));
}

static void snd(char SND[])
{
  FILE *fp = popen(SND_CMD, "r");
  if (!fp)
    return;

  char STRING[128];
  if (fgets(STRING, 128, fp) && fgets(STRING, 128, fp))
    sscanf(STRING, "%*[^ ] %*[^ ] %*[^ ] %*[^ ] %s", SND);
  else SND[0] = '\0';
  pclose(fp);
}

static void battery_cb(void *data, const char STRING[])
{
  batteries_t *batteries = data;
  unsigned *NBAT = &batteries->NBAT;
  if (!strncmp("BAT", STRING, 3))
  {
    strcpy(batteries->BATTERY[*NBAT].BAT, STRING);
    (*NBAT)++;
  }
}

#ifdef PROC_ACPI
static void battery_state_cb(void *data, const char STRING[])
{
  battery_t *battery = data, tmp;
  if (sscanf(STRING, "charging state: %s", tmp.STATE))
    strcpy(battery->STATE, tmp.STATE);
  else if (sscanf(STRING, "present rate: %d", &tmp.rate))
    battery->rate = tmp.rate;
  else if (sscanf(STRING, "remaining capacity: %d", &tmp.remaining))
  {
    battery->remaining = tmp.remaining;
    battery->perc = (float) battery->remaining / battery->capacity * 100;
  }
}

static void battery_info_cb(void *data, const char STRING[])
{
  battery_t *battery = data, tmp;
  if (sscanf(STRING, "design capacity: %d", &tmp.capacity))
    battery->capacity = tmp.capacity;
}

static void init_batteries(batteries_t *batteries)
{
  batteries->NBAT = 0;
  read_dir(batteries, battery_cb, ACPI_BAT); 
  for (unsigned i = 0; i < batteries->NBAT; i++)
  {
    char INFOFILE[32], *bat = batteries->BATTERY[i].BAT;
    sprintf(INFOFILE, "%s/%s/info", ACPI_BAT, bat);
    read_file(&batteries->BATTERY[i], battery_info_cb, INFOFILE);
    sprintf(batteries->BATTERY[i].STATEFILE, "%s/%s/state", ACPI_BAT, bat);
  }
}

static void ac_cb(void *data, const char STRING[])
{
  bool *ac_state = data;
  if (!strcmp("state: on-line", STRING))
    *ac_state = 1;
  else
    *ac_online = 0;
}
#else
static void battery_state_cb(void *data, const char STRING[])
{
  battery_t *battery = data, tmp;
  if (sscanf(STRING, "POWER_SUPPLY_STATUS=%c", &tmp.state))
    battery->state = tmp.state > 96 ? tmp.state - 32 : tmp.state;
  else if (sscanf(STRING, "POWER_SUPPLY_CURRENT_NOW=%ud", &tmp.rate))
    battery->rate = tmp.rate;
  else if (sscanf(STRING, "POWER_SUPPLY_CAPACITY=%ud", &tmp.perc))
    battery->perc = tmp.perc;
}

static void init_batteries(batteries_t *batteries)
{
  batteries->NBAT = 0;
  read_dir(batteries, battery_cb, SYS_PS); 
  for (unsigned i = 0; i < batteries->NBAT; i++)
  {
    char *bat = batteries->BATTERY[i].BAT;
    sprintf(batteries->BATTERY[i].STATEFILE, "%s/%s/uevent", SYS_PS, bat);
  }
}

static void ac_cb(void *data, const char STRING[])
{
  bool *ac_state = data;
  unsigned val;
  sscanf(STRING, "%ud", &val);
  *ac_state = !val;
}
#endif
static void public_ip(ip_t *ip)
{
  char SWAP[64];
  strcpy(SWAP, ip->BUFFER);
  if (curl_easy_perform(ip->handle) != CURLE_OK)
  {
    strcpy(ip->BUFFER, "No IP");
    return;
  }
  else if (strcmp(ip->BUFFER, ip->PREV))
  {
    FILE *fp = fopen(IPLIST, "a+");
    if (!fp)
      return;

    fprintf(fp, "%s\n", ip->BUFFER);
    fclose(fp);
    strcpy(ip->PREV, SWAP);
  }
}

static bool init_ssid(ssid_t *ssid, const char NETIF[])
{
  memset(&ssid->wreq, 0, sizeof ssid->wreq);
  strcpy(ssid->wreq.ifr_name, NETIF);
  if ((ssid->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) > -1)
    return 1;

  return 0;
}

static void ssid(ssid_t *ssid)
{
  memset(ssid->SSID, 0, sizeof ssid->SSID);
  ssid->wreq.u.essid.pointer = ssid->SSID;
  ssid->wreq.u.essid.length = sizeof ssid->SSID;
  ioctl(ssid->sockfd, SIOCGIWESSID, &ssid->wreq);
}

static void wireless_cb(void *data, const char LINE[])
{
  wireless_t *wireless = data, tmp;
  sscanf(LINE, "%s %*[^ ] %f %f %f", tmp.IFNAME, &tmp.link, &tmp.level, &tmp.noise);
  if (strncmp(tmp.IFNAME, wireless->net->netif, strlen(wireless->net->netif)) == 0)
  {
    wireless->link = tmp.link;
    wireless->level = tmp.level;
    wireless->noise = tmp.noise;
  }
}

static void net_cb(void *data, const char LINE[])
{
  net_t *net = data, tmp;
  sscanf(LINE, "%s %lu %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ] %lu %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ] %*[^ ]",
      tmp.IFNAME,
      &tmp.RXbytes,
      &tmp.TXbytes);

  if (strncmp(tmp.IFNAME, net->netif, strlen(net->netif)) == 0)
  {
    tmp.RXbytes /= kB;
    tmp.TXbytes /= kB;
    net->downkbytes = (tmp.RXbytes - net->RXbytes) / interval;
    net->upkbytes = (tmp.TXbytes - net->TXbytes) / interval;
    net->RXbytes = tmp.RXbytes;
    net->TXbytes = tmp.TXbytes;
  }
}

static unsigned wireless_link(wireless_t *wireless)
{
  return wireless->link / 70. * 100;
}

static void init_net(net_t NET[], wireless_t WLAN[])
{
  for (unsigned i = 0; i < LENGTH(NETIF); i++)
  {
    NET[i].netif = NETIF[i];
    WLAN[i].net = &NET[i];
    while(!init_ssid(&WLAN[i].ssid, NETIF[i]));
    read_file(&NET[i], net_cb, NET_ADAPTERS);
    read_file(&WLAN[i], wireless_cb, WIRELESS);
  }
}

static unsigned du_perc(const char DIRECTORY[])
{
  struct statvfs fs;
  if (statvfs(DIRECTORY, &fs) < 0)
    return 0;

  return (1 - ((float) fs.f_bfree / (float) fs.f_blocks)) * 100;
}

static void blkdev_cb(void *data, const char LINE[])
{
  diskstats_t *diskstats = data, tmp;
  sscanf(LINE, " %*[^ ] %*[^ ] %s %*[^ ] %*[^ ] %lu %*[^ ] %*[^ ] %*[^ ] %lu %*[^ ] %*[^ ] %*[^ ] %*[^ ]",
      tmp.DEVNAME,
      &tmp.rd_sec_or_wr_ios,
      &tmp.wr_sec);

  if (strcmp(tmp.DEVNAME, diskstats->blkdev) == 0)
  {
    diskstats->readkbs = (tmp.rd_sec_or_wr_ios - diskstats->rd_sec_or_wr_ios) / 2. / interval;
    diskstats->writekbs = (tmp.wr_sec - diskstats->wr_sec) / 2. / interval;
    diskstats->rd_sec_or_wr_ios =  tmp.rd_sec_or_wr_ios;
    diskstats->wr_sec = tmp.wr_sec;
  }
}

static void init_diskstats(diskstats_t DISKSTATS[])
{
  for (unsigned i = 0; i < LENGTH(BLKDEV); i++)
  {
    DISKSTATS[i].blkdev = BLKDEV[i];
    read_file(&DISKSTATS[i], blkdev_cb, DISKSTAT);
  }
}

static void mem_cb(void *data, const char LINE[])
{
  mem_t *mem = data;
  sscanf(LINE, "MemTotal: %lu", &mem->total);
  sscanf(LINE, "MemFree: %lu", &mem->free);
  sscanf(LINE, "MemAvailable: %lu", &mem->available);
  sscanf(LINE, "Buffers: %lu", &mem->buffers);
  sscanf(LINE, "Cached: %lu", &mem->cached);
  sscanf(LINE, "SwapCached: %lu", &mem->swapcached);
  sscanf(LINE, "SwapTotal: %lu", &mem->swaptotal);
  sscanf(LINE, "SwapFree: %lu", &mem->swapfree);
  mem->perc = (float) (mem->total - mem->free - mem->buffers - mem->cached) / mem->total * 100;
  mem->swap = mem->swaptotal - mem->swapfree - mem->swapcached;
}

static void cpu_cb(void *data, const char LINE[])
{
  cpu_t *cpu = data, tmp;

  if (sscanf(LINE, "cpu%*[^0-9] %lu %lu %lu %lu %lu %lu %lu",
        &tmp.user,
        &tmp.nice,
        &tmp.system,
        &tmp.idle,
        &tmp.iowait,
        &tmp.irq,
        &tmp.softirq) == 7)
  {
    cpu->perc = (double) (tmp.user + tmp.nice + tmp.system + tmp.irq + tmp.softirq
        - cpu->user - cpu->nice - cpu->system - cpu->irq - cpu->softirq) /
      (double) (tmp.user + tmp.nice + tmp.system + tmp.idle + tmp.iowait + tmp.irq + tmp.softirq
          - cpu->user - cpu->nice - cpu->system - cpu->idle - cpu->iowait - cpu->irq - cpu->softirq) * 100.;

    cpu->user = tmp.user;
    cpu->nice = tmp.nice;
    cpu->system = tmp.system;
    cpu->idle = tmp.idle;
    cpu->iowait = tmp.iowait;
    cpu->irq = tmp.irq;
    cpu->softirq = tmp.softirq;
  }

  else if (sscanf(LINE, "processor : %ud", &tmp.processor) == 1)
    cpu->processor = tmp.processor;
  else if (sscanf(LINE, "cpu MHz : %f", &tmp.mhz) == 1)
    /* Calculate the rolling average wrt number of cores */
    cpu->mhz = (cpu->mhz * (cpu->processor) + tmp.mhz) / (cpu->processor + 1);
}

static size_t writefunc(void *ptr, size_t size, size_t nmemb, void *data)
{
  ip_t *ip = data;
  size_t S = size * nmemb;
  if (S < sizeof ip->BUFFER)
  {
    memset(ip->BUFFER, 0, S);
    memcpy(ip->BUFFER, ptr, S);
    ip->BUFFER[S] = '\0';
  }
  else ip->BUFFER[0] = '\0';

  return S;
}

static void deinit_curl(ip_t *ip)
{
  curl_easy_cleanup(ip->handle);
}

static void init_curl(ip_t *ip)
{
  curl_global_init(CURL_GLOBAL_ALL);
  ip->handle = curl_easy_init();
  curl_easy_setopt(ip->handle, CURLOPT_URL, IPURL[0]);
  curl_easy_setopt(ip->handle, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(ip->handle, CURLOPT_WRITEDATA, ip);
  curl_easy_setopt(ip->handle, CURLOPT_TIMEOUT_MS, 250L);
}

static void deinit_ip(ip_t *ip)
{
  deinit_curl(ip);
}

static void init_ip(ip_t *ip)
{
  init_curl(ip);
  ip->BUFFER[0] = '\0';
  tail(ip->PREV, sizeof ip->PREV, IPLIST, 1);
}

void deinit_status(void)
{
  //deinit_device(&device);
  deinit_ip(&ip);
}
void init_status(void)
{
  setlocale(LC_ALL, "");
  setbuf(stdout, NULL);
  init_diskstats(DISKSTATS);
  init_net(NET, WLAN);
  init_ip(&ip);
  init_batteries(&batteries);
  /*
  init_device(&device);
  struct input_event evt;
  time_t init, now;
  float time_diff;
  unsigned poll_interval;
  */
}

static void print(char RESULT[], const char *format, ...)
{
  va_list args = { 0 };
  char STRING[64];
  va_start(args, format);
  vsnprintf(STRING, 64, format, args);
  va_end(args);
  strcat(RESULT, STRING);
}

int status(Display *dpy, char STRING[])
{
  read_file(&cpu, cpu_cb, CPU);
  read_file(&cpu, cpu_cb, STAT);
  read_file(&mem, mem_cb, MEM);
  sprintf(STRING, "%.0f%% %.0fMHz", cpu.perc, cpu.mhz);
  print(STRING, "%s%.0f%% (%s)", DELIM, mem.perc, format_units(mem.swap));

  for (unsigned i = 0; i < LENGTH(BLKDEV); i++)
  {
    read_file(&DISKSTATS[i], blkdev_cb, DISKSTAT);
    print(STRING, "%s%s ", DELIM, BLKDEV[i]);
    print(STRING, "%s%s", UP, format_units(DISKSTATS[i].readkbs));
    print(STRING, "%s%s", DOWN, format_units(DISKSTATS[i].writekbs));
  }

  print(STRING, "%s", DELIM);
  for (unsigned i = 0; i < LENGTH(DIRECTORY); i++)
    print(STRING, "%s %u%% ", DIRECTORY[i], du_perc(DIRECTORY[i]));

  for (unsigned i = 0; i < LENGTH(NETIF); i++)
  {
    read_file(&NET[i], net_cb, NET_ADAPTERS);
    read_file(&WLAN[i], wireless_cb, WIRELESS);
    print(STRING, "%s%s ", DELIM, NET[i].netif);
    ssid(&WLAN[i].ssid);
    if (strlen(WLAN[i].ssid.SSID))
      print(STRING, "%s %d%% ", WLAN[i].ssid.SSID, wireless_link(&WLAN[i]));

    print(STRING, "%s%s", UP, format_units(NET[i].upkbytes));
    print(STRING, "(%s)", format_units(NET[i].TXbytes));
    print(STRING, "%s%s", DOWN, format_units(NET[i].downkbytes));
    print(STRING, "(%s)", format_units(NET[i].RXbytes));
  }

  public_ip(&ip);
  if (!strcmp(ip.PREV, ip.BUFFER) || !strlen(ip.PREV))
    print(STRING, "%s%s", DELIM, ip.BUFFER);
  else
    print(STRING, "%s%s%s%s", DELIM, ip.PREV, RIGHT_ARROW, ip.BUFFER);
#ifdef PROC_ACPI
  read_file(&ac_state, ac_cb, ACPI_ACSTATE);
#else
  read_file(&ac_state, ac_cb, SYS_ACSTATE);
#endif
  batteries.total_perc = 0;
  for (unsigned i = 0; i < batteries.NBAT; i++)
  {
    read_file(&batteries.BATTERY[i], battery_state_cb, batteries.BATTERY[i].STATEFILE);
    print(STRING, "%s%s %d%% %c", DELIM, 
        batteries.BATTERY[i].BAT, 
        batteries.BATTERY[i].perc, 
        batteries.BATTERY[i].state);
    batteries.total_perc += batteries.BATTERY[i].perc;
  }

  if ((float) batteries.total_perc / batteries.NBAT < SUSPEND_THRESHOLD_PERC)
  {
    sprintf(STRING, "Battery < %d%%", SUSPEND_THRESHOLD_PERC);
    XStoreName(dpy, DefaultRootWindow(dpy), STRING);
    return -1;
  }

  snd(SND);
  print(STRING, "%s%s%s", DELIM, SNDSYM, SND);
  date(TIME, sizeof TIME);
  print(STRING, "%s%s", DELIM, TIME);
  XStoreName(dpy, DefaultRootWindow(dpy), STRING);
  interval = ac_state ? UPDATE_INTV_ON_BATTERY : UPDATE_INTV;
  return interval;
  /*
  poll_interval = interval;
  time(&init);
poll_resume:
  poll(device.PFD, device.NFD, poll_interval * 1000);
  for (unsigned i = 0; i < device.NFD; i++)
    if (device.PFD[i].revents & POLLIN && 
        read(device.PFD[i].fd, &evt, sizeof evt) > 0)
    {
      if (!evt.value);
      else if (evt.type == EV_SW && evt.code == SW_LID)
        system(LOCKALL_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_BRIGHTNESSUP)
        system(BRIGHTNESSUP_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_BRIGHTNESSDOWN)
        system(BRIGHTNESSDOWN_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_SWITCHVIDEOMODE)
        system(SWITCHDISPLAY_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_MUTE)
        system(VOLUMEMUTE_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_VOLUMEUP)
        system(VOLUMEUP_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_VOLUMEDOWN)
        system(VOLUMEDOWN_CMD);
      else if (evt.type == EV_KEY && evt.code == KEY_SLEEP)
      {
        system(SLEEP_CMD);
        system(LOCKALL_CMD);
      }
      else if (evt.type == EV_KEY && evt.code == KEY_POWER)
      {
        system(SUSPEND_CMD);
        system(LOCKALL_CMD);
      }

      time(&now);
      time_diff = difftime(now, init);
      if (time_diff < interval)
      {
        poll_interval = interval - time_diff;
        goto poll_resume;
      }
    }
    */
}
