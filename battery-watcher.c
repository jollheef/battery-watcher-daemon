/**
 * @file   battery-watcher.c
 * @author Mikhail Klementyev jollheef<AT>riseup.net
 * @license GNU GPLv3
 * @date   March, 2015
 * @brief  Battery watcher
 *
 * Battery capacity watcher. Perform command on low battery capacity.
 */

#define IN
#define OUT

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>

#define SYS_CLASS_POWER_PATH ("/sys/class/power_supply/")

#define MIN_LOWEST_BATTERY_VALUE (7)
#define MAX_LOWEST_BATTERY_VALUE (90)
#define DEFAULT_LOWEST_BATTERY_VALUE (10)

#define MIN_SHUTDOWN_LOWEST_BATTERY_VALUE (5)

#define BATTERY_CHECK_TIMEOUT (10)

#define SHUTDOWN_COMMAND ("shutdown -h now")

typedef enum
  {
    FULL,
    DISCHARGING,
    CHARGING,
    UNKNOWN
  } battery_status_t;

typedef struct _battery_t
{
  /* Battery status */
  battery_status_t status;

  /* Capcity value in percents */
  uint32_t capacity;
} battery_t;

/* Flag up if need gracefully exit */
bool exitFlag = false;

/**
 * Parse battery status.
 *
 * @param s [in] -- line, what contain status string.
 * @return battery status value.
 */
battery_status_t
parseBatteryStatus (IN char* s)
{
  if (0 == strcmp (s, "Charging\n") )
    {
      return CHARGING;
    }
  else if (0 == strcmp (s, "Discharging\n") )
    {
      return DISCHARGING;
    }
  else if (0 == strcmp (s, "Full\n") )
    {
      return FULL;
    }
  else
    {
      return UNKNOWN;
    }
}

/**
 * Return status and current battery capacity (in percents).
 *
 * @param battery [in out] -- structure, describe battery.
 * @returm true if success, false otherwise.
 */
bool
getBatteryInfo (IN OUT battery_t* battery)
{
  struct dirent* ent;

  DIR* directory = opendir (SYS_CLASS_POWER_PATH);

  char* batteryDirectoryPath = calloc (1, PATH_MAX);

  if (errno)
    {
      return (false);
    }

  /* Find battery directory */
  while ( (ent = readdir (directory) ) != NULL)
    {
      /* Always use first battery (me use notebook) */
      if (strcasestr (ent->d_name, "BAT") && (DT_LNK == ent->d_type) )
        {
          sprintf (batteryDirectoryPath, "%s/%s/",
                   SYS_CLASS_POWER_PATH, ent->d_name);
          chdir (batteryDirectoryPath);

          size_t len = 0;
          char* line = calloc (1, 1024);

          /* Parse status */

          FILE* statusFile = fopen ("status", "r");

          if (errno)
            {
              return (false);
            }

          if (-1 == getline (&line, &len, statusFile) )
            {
              return (false);
            }
          else
            {
              battery->status = parseBatteryStatus (line);
            }

          fclose (statusFile);

          /* Parse capacity */

          FILE* capacityFile = fopen ("capacity", "r");

          if (errno)
            {
              return (false);
            }

          if (-1 == getline (&line, &len, capacityFile) )
            {
              return (false);
            }
          else
            {
              battery->capacity = atoi (line);
            }

          free (line);
          fclose (statusFile);
        }
    }

  /* Be nice cats, nice cats free memory */
  closedir (directory);
  free (batteryDirectoryPath);

  return (true);
}

/**
 * SIGTERM and SIGINT signal handler.
 * @param sig [in] -- signal code.
 */
void
exitGracefully (IN int sig)
{
  fprintf (stderr, "\nAttempting to exit gracefully...\n");

  exitFlag = true;
}

/**
 * Program entry point.
 * @param argc [in] -- command line arguments counter.
 * @param argv [in] -- pointer to command line arguments.
 * @return exit status.
 */
int
main (IN int argc, IN char* argv[])
{
  uint32_t lowestBatteryCapacity = DEFAULT_LOWEST_BATTERY_VALUE;

  char* command = SHUTDOWN_COMMAND;

  /* First arg -- minimal battery capacity for perform command */
  if (argc > 1)
    {
      lowestBatteryCapacity = atoi (argv[1]);
    }

  /* Normalize value */
  if (lowestBatteryCapacity > MAX_LOWEST_BATTERY_VALUE)
    {
      lowestBatteryCapacity = MAX_LOWEST_BATTERY_VALUE;
    }
  else if (lowestBatteryCapacity < MIN_LOWEST_BATTERY_VALUE)
    {
      lowestBatteryCapacity = MAX_LOWEST_BATTERY_VALUE;
    }
  
  /* Command, than performs if battery capacity lower than setted */
  if (argc > 2)
    {
      command = argv[2];
    }

  fprintf (stderr,
           "Usage: %s [0-100] COMMAND\n", argv[0]);

  fprintf (stderr,
           "Command '%s' on %d%% battery capacity\n",
           command,
           lowestBatteryCapacity);

  /* Set signal handlers for gracefully selfkill */
  signal (SIGTERM, exitGracefully);
  signal (SIGINT, exitGracefully);

  battery_t* battery = calloc (sizeof (struct _battery_t), 1);

  while (!exitFlag)
    {
      if (getBatteryInfo (battery) )
        {
          if (battery->capacity < lowestBatteryCapacity
              && battery->status != FULL
              && battery->status != CHARGING)
            {
              if (battery->capacity < MIN_SHUTDOWN_LOWEST_BATTERY_VALUE)
                {
                  /* If command fail (i.e. pm-suspend fail) */
                  system (SHUTDOWN_COMMAND);
                }
              else
                {
                  system (command);
                }
            }
        }
      else
        {
          perror ("Get battery info error");
        }

      sleep (BATTERY_CHECK_TIMEOUT);
    }

  exit (EXIT_SUCCESS);
}
