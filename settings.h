// Target IP and port of E3DC RSCP. Default port is 5033
#define SERVER_IP       ""
#define SERVER_PORT     5033

// Usename and passwort of E3DC webinterface - required to obtain additional data
#define E3DC_USER       ""
#define E3DC_PASSWORD   ""

// Password defined within E3DC device to access data
#define AES_PASSWORD    ""

// Location where the json data should be stored
#define TARGET_FILE     "/mnt/RAMDisk/e3dc.json"

// Number of used trackers - 1 default | 2 with max. 2 strings
#define PVI_TRACKER     1

// Seconds to wait until every fetch of data. Minimum is 1 (second)
#define FETCH_INTERVAL  1
