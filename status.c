#include <string.h>
#include <stdio.h>
#include "status.h"

static const char *BLKDEV[] = { BLKDEVS };
static const char *DIRECTORY[] = { DIRECTORIES };
static const char *NETIF[] = { NETIFS };
static unsigned char interval = UPDATE_INTV;
static unsigned battery_total_perc;

int status(char STRING[])
{
  sprintf(STRING, "%s%dW", PWRSYM, power(interval));
  print(STRING, "%s%dW", PWRSYM, power(interval));
  print(STRING, "%s%.0lf%% %.0fMHz", DELIM, cpu_perc(), cpu_mhz());
  print(STRING, "%s%.0f%% (%s)", DELIM, mem_perc(), format_units(mem_swap()));

  for (unsigned i = 0; i < LENGTH(BLKDEV); i++)
  {
    print(STRING, "%s%s ", DELIM, BLKDEV[i]);
    refresh_diskstats(i);
    print(STRING, "%s%s", UP, format_units(read_kbps(i, interval)));
    print(STRING, "%s%s", DOWN, format_units(write_kbps(i, interval)));
  }

  print(STRING, "%s", DELIM);
  for (unsigned i = 0; i < LENGTH(DIRECTORY); i++)
    print(STRING, "%s %u%% ", DIRECTORY[i], du_perc(DIRECTORY[i]));

  for (unsigned i = 0; i < LENGTH(NETIF); i++)
  {
    print(STRING, "%s%s ", DELIM, NETIF[i]);
    if (ssid(i))
      print(STRING, "%s %d%% ", ssid_string(i), wireless_link(i));

    refresh_netadapter(i);
    print(STRING, "%s%s", UP, format_units(tx_kbps(i, interval)));
    print(STRING, "(%s)", format_units(tx_total_kb(i)));
    print(STRING, "%s%s", DOWN, format_units(rx_kbps(i, interval)));
    print(STRING, "(%s)", format_units(rx_total_kb(i)));
  }

  refresh_publicip();
  if (!strcmp(prev_ip(), curr_ip()) || !strlen(prev_ip()))
    print(STRING, "%s%s", DELIM, curr_ip());
  else
    print(STRING, "%s%s%s%s", DELIM, prev_ip(), RIGHT_ARROW, curr_ip());

  refresh_ps();
  battery_total_perc = 0;
  for (unsigned i = 0; i < batteries_size(); i++)
  {
    refresh_battery(i);
    print(STRING, "%s%s %d%% %c", DELIM,
        battery_string(i),
        battery_perc(i),
        battery_state(i));
    battery_total_perc += battery_perc(i);
  }
  
  if ((float) battery_total_perc / batteries_size() < SUSPEND_THRESHOLD_PERC)
  {
    sprintf(STRING, "Battery < %d%%", SUSPEND_THRESHOLD_PERC);
    return -1;
  }

  for (unsigned i = 0; i < asound_cards_size(); i++)
  {
    refresh_asound_card(i);
    print(STRING, "%s%s%s%s%s",
        DELIM, SNDSYM, asound_card_p(i), MICSYM, asound_card_c(i));
  }
  print(STRING, "%s%s\n", DELIM, date());
  interval = ac() ? UPDATE_INTV_ON_BATTERY : UPDATE_INTV;
  return interval;
}
