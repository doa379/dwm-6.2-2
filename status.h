#define	STRLEN 1024
/* Update interval in sec */
#define UPDATE_INTV_ON_BATTERY 30
#define UPDATE_INTV	5
#define MAX_BATTERIES 2
#define SUSPEND_THRESHOLD_PERC 5
/* #define PROC_ACPI */
#define SLEEP_CMD "sudo /usr/local/bin/zzz M"
#define SUSPEND_CMD "sudo /usr/local/bin/zzz D"
#define SWITCHDISPLAY_CMD "switchdisplay"
#define VOLUMEMUTE_CMD "vol 0"
#define VOLUMEUP_CMD "vol +"
#define VOLUMEDOWN_CMD "vol -"
#define BRIGHTNESSUP_CMD "sudo /usr/local/bin/backlight +"
#define BRIGHTNESSDOWN_CMD "sudo /usr/local/bin/backlight -"
#define LOCKALL_CMD "lockall"
#define UP_ARROW                "\u2b06"
#define DOWN_ARROW		          "\u2b07"
#define UP_TRI                  "\u25b4"
#define DOWN_TRI                "\u25be"
#define RIGHT_ARROW		          "\u27a1"
#define FULL_SPACE		          "\u2000"
#define SHORT_SPACE		          "\u2005"
#define HEAVY_HORIZONTAL        "\u2501"
#define HEAVY_VERTICAL		      "\u2503"
#define DOUBLE_VERTICAL		      "\u2551"
#define LF_THREE_EIGHTHS_BLOCK	"\u258d"
#define LIGHT_SHADE		          "\u2591"
#define MEDIUM_SHADE		        "\u2592"
#define DARK_SHADE		          "\u2593"
#define DASH_VERT               "\u250a"
#define UP                      UP_TRI
#define DOWN                    DOWN_TRI
#define SNDSYM                  "♬"
#define DELIM		                "|"

static const char *BLKDEV[] = { "nvme0n1", };
static const char *DIRECTORY[] = { "/", "/tmp" };
static const char *NETIF[] = { "eth0", "wlan0", "wlan1", "wlan2" };
static const char *IPLIST = "/tmp/iplist";
static const char *IPURL[] = {
	"http://whatismyip.akamai.com",
	"http://checkip.amazonaws.com",
	"http://ipinfo.io/ip",
	"http://ipecho.net/plain",
};

