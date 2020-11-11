#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include "status.h"

static const char *BLKDEV[] = { BLKDEVS };
static const char *DIRECTORY[] = { DIRECTORIES };
static const char *NETIF[] = { NETIFS };

static cpu_t cpu;
static mem_t mem;
static diskstats_t DISKSTATS[LENGTH(BLKDEV)];
static net_t NET[LENGTH(NETIF)];
static wireless_t WLAN[LENGTH(NETIF)];
static ip_t ip;
static bool ac_state;
static batteries_t batteries;
static char SND[16], TIME[32];
static powercaps_t powercaps;
static unsigned char interval = UPDATE_INTV;

static void print(char RESULT[], const char *format, ...)
{
  va_list args = { 0 };
  char STRING[64];
  va_start(args, format);
  vsnprintf(STRING, 64, format, args);
  va_end(args);
  strcat(RESULT, STRING);
}

void deinit_status(void)
{
  deinit_power(&powercaps);
  deinit_batteries(&batteries);
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
  init_power(&powercaps);
}

int status(char STRING[])
{
  sprintf(STRING, "%s%dW", PWRSYM, power(&powercaps, interval));
  read_file(&cpu, cpu_cb, CPU);
  read_file(&cpu, cpu_cb, STAT);
  read_file(&mem, mem_cb, MEM);
  print(STRING, "%s%.0f%% %.0fMHz", DELIM, cpu.perc, cpu.mhz);
  print(STRING, "%s%.0f%% (%s)", DELIM, mem.perc, format_units(mem.swap));

  for (unsigned i = 0; i < LENGTH(BLKDEV); i++)
  {
    read_file(&DISKSTATS[i], blkdev_cb, DISKSTAT);
    print(STRING, "%s%s ", DELIM, BLKDEV[i]);
    print(STRING, "%s%s", UP, format_units(read_kbps(&DISKSTATS[i], interval)));
    print(STRING, "%s%s", DOWN, format_units(write_kbps(&DISKSTATS[i], interval)));
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

    print(STRING, "%s%s", UP, format_units(up_kbps(&NET[i], interval)));
    print(STRING, "(%s)", format_units(NET[i].TXbytes / kB));
    print(STRING, "%s%s", DOWN, format_units(down_kbps(&NET[i], interval)));
    print(STRING, "(%s)", format_units(NET[i].RXbytes / kB));
 }

  public_ip(&ip);
  if (!strcmp(ip.PREV, ip.CURR) || !strlen(ip.PREV))
    print(STRING, "%s%s", DELIM, ip.CURR);
  else
    print(STRING, "%s%s%s%s", DELIM, ip.PREV, RIGHT_ARROW, ip.CURR);
#ifdef PROC_ACPI
  read_file(&ac_state, ac_cb, ACPI_ACSTATE);
#else
  read_file(&ac_state, ac_cb, SYS_ACSTATE);
#endif
  batteries.total_perc = 0;
  for (unsigned i = 0; i < batteries.size; i++)
  {
    read_file(&batteries.battery[i], battery_state_cb, batteries.battery[i].STATEFILE);
    print(STRING, "%s%s %d%% %c", DELIM, 
        batteries.battery[i].BAT, 
        batteries.battery[i].perc, 
        batteries.battery[i].state);
    batteries.total_perc += batteries.battery[i].perc;
  }

  if ((float) batteries.total_perc / batteries.size < SUSPEND_THRESHOLD_PERC)
  {
    sprintf(STRING, "Battery < %d%%", SUSPEND_THRESHOLD_PERC);
    return -1;
  }

  snd(SND);
  print(STRING, "%s%s%s", DELIM, SNDSYM, SND);
  date(TIME, sizeof TIME);
  print(STRING, "%s%s", DELIM, TIME);
  interval = ac_state ? UPDATE_INTV_ON_BATTERY : UPDATE_INTV;
  return interval;
}
