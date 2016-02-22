/*
 * Author: Kaushik, Koneru
 * Email: konerukaushik@gmail.com
 */

#ifndef __ROUTER_H__
#define __ROUTER_H__
#include <math.h>

/* Macros to Connect to Router */
#define ROUTER_PORT 5999

#define IP_PROTOCOL  0
#define ERROR       -1

/* Macros to Make life easy */
#define LOG(s) printf(s"\n");

#define TRUE    1
#define FALSE   0

#define Y               1200 /* Send 30 Packets from Host A to B */

#define PERCENT_TO_DROP 0.5

#define PKT_TO_DROP     (int)((Y * PERCENT_TO_DROP)/100 ) 

#define TIMEOUT_SEC     4 /* Keep Timeout for 4 sec */

#endif
