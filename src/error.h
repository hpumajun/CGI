/*
 *  printlog -- An application to securely tunnel IP networks
 *             over a single TCP/UDP port, with support for SSL/TLS-based
 *             session authentication and key exchange,
 *             packet encryption, packet authentication, and
 *             packet compression.
 *
 *  Copyright (C) 2002-2010 OpenVPN Technologies, Inc. <sales@openvpn.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
/* #define ABORT_ON_ERROR */

#define ERR_BUF_SIZE 9*1024


/*
 * Where should messages be printed before syslog is opened?
 * Not used if OPENVPN_DEBUG_COMMAND_LINE is defined.
 */
#define OPENVPN_MSG_FP   stdout
#define OPENVPN_ERROR_FP stderr


#define EAGAIN 11

/*
 * Exit status codes
 */

#define EXIT_STATUS_GOOD                    0
#define EXIT_STATUS_ERROR                   1
#define EXIT_STATUS_USAGE                   1
#define EXIT_STATUS_CANNOT_OPEN_DEBUG_FILE  1

/*
 * Special command line debugging mode.
 * If OPENVPN_DEBUG_COMMAND_LINE
 * is defined, contents of argc/argv will
 * be dumped to OPENVPN_DEBUG_FILE as well
 * as all other OpenVPN messages.
 */

/* #define OPENVPN_DEBUG_COMMAND_LINE */
#define OPENVPN_DEBUG_FILE  "log.log"

/* String and Error functions */

extern unsigned int x_debug_level;
extern int x_msg_line_num;

/* msg() flags */

#define M_DEBUG_LEVEL     (0x0F)     /* debug level mask */

#define M_FATAL           (1<<4)     /* exit program */
#define M_NONFATAL        (1<<5)     /* non-fatal error */
#define M_WARN            (1<<6)     /* call syslog with LOG_WARNING */
#define M_DEBUG           (1<<7)

#define M_ERRNO           (1<<8)     /* show errno description */
#define M_ERRNO_SOCK      (1<<9)     /* show socket errno description */
#define M_SSL             (1<<10)    /* show SSL error */
#define M_NOMUTE          (1<<11)        /* don't do mute processing */
#define M_NOPREFIX        (1<<12)        /* don't show date/time prefix */
#define M_USAGE_SMALL     (1<<13)        /* fatal options error, call usage_small */
#define M_MSG_VIRT_OUT    (1<<14)        /* output message through msg_status_output callback */
#define M_OPTERR          (1<<15)        /* print "Options error:" prefix */
#define M_NOLF            (1<<16)        /* don't print new line */
#define M_NOIPREFIX       (1<<17)        /* don't print instance prefix */

/* flag combinations which are frequently used */
#define M_ERR     (M_FATAL | M_ERRNO)
#define M_SOCKERR (M_FATAL | M_ERRNO_SOCK)
#define M_SSLERR  (M_FATAL | M_SSL)
#define M_USAGE   (M_USAGE_SMALL | M_NOPREFIX | M_OPTERR)
#define M_CLIENT  (M_MSG_VIRT_OUT | M_NOMUTE | M_NOIPREFIX)

/*
 * Mute levels are designed to avoid large numbers of
 * mostly similar messages clogging the log file.
 *
 * A mute level of 0 is always printed.
 */
#define MUTE_LEVEL_SHIFT 24
#define MUTE_LEVEL_MASK 0xFF

#define ENCODE_MUTE_LEVEL(mute_level) (((mute_level) & MUTE_LEVEL_MASK) << MUTE_LEVEL_SHIFT)
#define DECODE_MUTE_LEVEL(flags) (((flags) >> MUTE_LEVEL_SHIFT) & MUTE_LEVEL_MASK)

/*
 * log_level:  verbosity level n (--verb n) must be >= log_level to print.
 * mute_level: don't print more than n (--mute n) consecutive messages at
 *             a given mute level, or if 0 disable muting and print everything.
 *
 * Mask map:
 * Bits 0-3:   log level
 * Bits 4-23:  M_x flags
 * Bits 24-31: mute level
 */
#define LOGLEV(log_level, mute_level, other) ((log_level) | ENCODE_MUTE_LEVEL(mute_level) | other)

/*
 * If compiler supports variable arguments in macros, define
 * msg() as a macro for optimization win.
 */

/*
 * Debug level at and above where we
 * display time to microsecond resolution.
 */
#define DEBUG_LEVEL_USEC_TIME 4

/*
 * In non-server modes, delay n milliseconds after certain kinds
 * of non-fatal network errors to avoid a barrage of errors.
 *
 * To disable all delays, set to 0.
 */
#define P2P_ERROR_DELAY_MS 0

/*
 * Enable D_LOG_RW
 */
#define LOG_RW 1

/*
 * Debugging levels for various kinds
 * of output.
 */

#define M_VERB0              LOGLEV(0, 0, 0)         /* Messages displayed even at --verb 0 (fatal errors only) */

#define M_INFO               LOGLEV(1, 0, 0)         /* default informational messages */

#define D_LINK_ERRORS        LOGLEV(1, 1, M_NONFATAL)   /* show link errors from main event loop */
#define D_CRYPT_ERRORS       LOGLEV(1, 2, M_NONFATAL)   /* show errors from encrypt/decrypt */
#define D_TLS_ERRORS         LOGLEV(1, 3, M_NONFATAL)   /* show TLS control channel errors */
#define D_RESOLVE_ERRORS     LOGLEV(1, 4, M_NONFATAL)   /* show hostname resolve errors */
#define D_COMP_ERRORS        LOGLEV(1, 5, M_NONFATAL)   /* show compression errors */
#define D_REPLAY_ERRORS      LOGLEV(1, 6, M_NONFATAL)   /* show packet replay errors */
#define D_STREAM_ERRORS      LOGLEV(1, 7, M_NONFATAL)    /* TCP stream error requiring restart */
#define D_IMPORT_ERRORS      LOGLEV(1, 8, M_NONFATAL)    /* show server import option errors */
#define D_MULTI_ERRORS       LOGLEV(1, 9, M_NONFATAL)    /* show multi-client server errors */
#define D_EVENT_ERRORS       LOGLEV(1, 10, M_NONFATAL)   /* show event.[ch] errors */
#define D_PUSH_ERRORS        LOGLEV(1, 11, M_NONFATAL)   /* show push/pull errors */
#define D_PID_PERSIST        LOGLEV(1, 12, M_NONFATAL)   /* show packet_id persist errors */
#define D_FRAG_ERRORS        LOGLEV(1, 13, M_NONFATAL)   /* show fragmentation errors */
#define D_ALIGN_ERRORS       LOGLEV(1, 14, M_NONFATAL)   /* show bad struct alignments */

#define D_HANDSHAKE          LOGLEV(2, 20, 0)        /* show data & control channel handshakes */
#define D_MTU_INFO           LOGLEV(2, 21, 0)        /* show terse MTU info */
#define D_CLOSE              LOGLEV(2, 22, 0)        /* show socket and TUN/TAP close */
#define D_SHOW_OCC_HASH      LOGLEV(2, 23, 0)        /* show MD5 hash of option compatibility string */
#define D_PROXY              LOGLEV(2, 24, 0)        /* show http proxy control packets */
#define D_ARGV               LOGLEV(2, 25, 0)        /* show struct argv errors */

#define D_TLS_DEBUG_LOW      LOGLEV(3, 20, 0)        /* low frequency info from tls_session routines */
#define D_GREMLIN            LOGLEV(3, 30, 0)        /* show simulated outage info from gremlin module */
#define D_GENKEY             LOGLEV(3, 31, 0)        /* print message after key generation */
#define D_ROUTE              LOGLEV(3, 0,  0)        /* show routes added and deleted (don't mute) */
#define D_TUNTAP_INFO        LOGLEV(3, 32, 0)        /* show debugging info from TUN/TAP driver */
#define D_RESTART            LOGLEV(3, 33, 0)        /* show certain restart messages */
#define D_PUSH               LOGLEV(3, 34, 0)        /* show push/pull info */
#define D_IFCONFIG_POOL      LOGLEV(3, 35, 0)        /* show ifconfig pool info */
#define D_BACKTRACK          LOGLEV(3, 36, 0)        /* show replay backtracks */
#define D_AUTH               LOGLEV(3, 37, 0)        /* show user/pass auth info */
#define D_MULTI_LOW          LOGLEV(3, 38, 0)        /* show point-to-multipoint low-freq debug info */
#define D_PLUGIN             LOGLEV(3, 39, 0)        /* show plugin calls */
#define D_MANAGEMENT         LOGLEV(3, 40, 0)        /* show --management info */
#define D_SCHED_EXIT         LOGLEV(3, 41, 0)        /* show arming of scheduled exit */
#define D_ROUTE_QUOTA        LOGLEV(3, 42, 0)        /* show route quota exceeded messages */
#define D_OSBUF              LOGLEV(3, 43, 0)        /* show socket/tun/tap buffer sizes */
#define D_PS_PROXY           LOGLEV(3, 44, 0)        /* messages related to --port-share option */
#define D_PF_INFO            LOGLEV(3, 45, 0)        /* packet filter informational messages */

#define D_SHOW_PARMS         LOGLEV(4, 50, 0)        /* show all parameters on program initiation */
#define D_SHOW_OCC           LOGLEV(4, 51, 0)        /* show options compatibility string */
#define D_LOW                LOGLEV(4, 52, 0)        /* miscellaneous low-frequency debug info */
#define D_DHCP_OPT           LOGLEV(4, 53, 0)        /* show DHCP options binary string */
#define D_MBUF               LOGLEV(4, 54, 0)        /* mbuf.[ch] routines */
#define D_PACKET_TRUNC_ERR   LOGLEV(4, 55, 0)        /* PACKET_TRUNCATION_CHECK */
#define D_PF_DROPPED         LOGLEV(4, 56, 0)        /* packet filter dropped a packet */
#define D_MULTI_DROPPED      LOGLEV(4, 57, 0)        /* show point-to-multipoint packet drops */

#define D_LOG_RW             LOGLEV(5, 0,  0)        /* Print 'R' or 'W' to stdout for read/write */

#define D_LINK_RW            LOGLEV(6, 60, M_DEBUG)  /* show TCP/UDP reads/writes (terse) */
#define D_TUN_RW             LOGLEV(6, 60, M_DEBUG)  /* show TUN/TAP reads/writes */
#define D_TAP_WIN32_DEBUG    LOGLEV(6, 60, M_DEBUG)  /* show TAP-Win32 driver debug info */

#define D_SHOW_KEYS          LOGLEV(7, 70, M_DEBUG)  /* show data channel encryption keys */
#define D_SHOW_KEY_SOURCE    LOGLEV(7, 70, M_DEBUG)  /* show data channel key source entropy */
#define D_REL_LOW            LOGLEV(7, 70, M_DEBUG)  /* show low frequency info from reliable layer */
#define D_FRAG_DEBUG         LOGLEV(7, 70, M_DEBUG)  /* show fragment debugging info */
#define D_WIN32_IO_LOW       LOGLEV(7, 70, M_DEBUG)  /* low freq win32 I/O debugging info */
#define D_MTU_DEBUG          LOGLEV(7, 70, M_DEBUG)  /* show MTU debugging info */
#define D_PID_DEBUG_LOW      LOGLEV(7, 70, M_DEBUG)  /* show low-freq packet-id debugging info */
#define D_MULTI_DEBUG        LOGLEV(7, 70, M_DEBUG)  /* show medium-freq multi debugging info */
#define D_MSS                LOGLEV(7, 70, M_DEBUG)  /* show MSS adjustments */
#define D_COMP_LOW           LOGLEV(7, 70, M_DEBUG)  /* show adaptive compression state changes */
#define D_CONNECTION_LIST    LOGLEV(7, 70, M_DEBUG)  /* show <connection> list info */
#define D_SCRIPT             LOGLEV(7, 70, M_DEBUG)  /* show parms & env vars passed to scripts */
#define D_SHOW_NET           LOGLEV(7, 70, M_DEBUG)  /* show routing table and adapter list */
#define D_ROUTE_DEBUG        LOGLEV(7, 70, M_DEBUG)  /* show verbose route.[ch] output */
#define D_TLS_STATE_ERRORS   LOGLEV(7, 70, M_DEBUG)  /* no TLS state for client */
#define D_SEMAPHORE_LOW      LOGLEV(7, 70, M_DEBUG)  /* show Win32 semaphore waits (low freq) */
#define D_SEMAPHORE          LOGLEV(7, 70, M_DEBUG)  /* show Win32 semaphore waits */
#define D_TEST_FILE          LOGLEV(7, 70, M_DEBUG)  /* show test_file() calls */
#define D_MANAGEMENT_DEBUG   LOGLEV(3, 70, M_DEBUG)  /* show --management debug info */
#define D_PLUGIN_DEBUG       LOGLEV(7, 70, M_DEBUG)  /* show verbose plugin calls */
#define D_SOCKET_DEBUG       LOGLEV(7, 70, M_DEBUG)  /* show socket.[ch] debugging info */
#define D_SHOW_PKCS11        LOGLEV(7, 70, M_DEBUG)  /* show PKCS#11 actions */
#define D_ALIGN_DEBUG        LOGLEV(7, 70, M_DEBUG)  /* show verbose struct alignment info */
#define D_PACKET_TRUNC_DEBUG LOGLEV(7, 70, M_DEBUG)  /* PACKET_TRUNCATION_CHECK verbose */
#define D_PING               LOGLEV(7, 70, M_DEBUG)  /* PING send/receive messages */
#define D_PS_PROXY_DEBUG     LOGLEV(7, 70, M_DEBUG)  /* port share proxy debug */
#define D_AUTO_USERID        LOGLEV(7, 70, M_DEBUG)  /* AUTO_USERID debugging */
#define D_TLS_KEYSELECT      LOGLEV(7, 70, M_DEBUG)  /* show information on key selection for data channel */
#define D_ARGV_PARSE_CMD     LOGLEV(7, 70, M_DEBUG)  /* show parse_line() errors in argv_printf %sc */
#define D_CRYPTO_DEBUG       LOGLEV(7, 70, M_DEBUG)  /* show detailed info from crypto.c routines */
#define D_PF_DROPPED_BCAST   LOGLEV(7, 71, M_DEBUG)  /* packet filter dropped a broadcast packet */
#define D_PF_DEBUG           LOGLEV(7, 72, M_DEBUG)  /* packet filter debugging, must also define PF_DEBUG in pf.h */

#define D_HANDSHAKE_VERBOSE  LOGLEV(8, 70, M_DEBUG)  /* show detailed description of each handshake */
#define D_TLS_DEBUG_MED      LOGLEV(8, 70, M_DEBUG)  /* limited info from tls_session routines */
#define D_INTERVAL           LOGLEV(8, 70, M_DEBUG)  /* show interval.h debugging info */
#define D_SCHEDULER          LOGLEV(8, 70, M_DEBUG)  /* show scheduler debugging info */
#define D_GREMLIN_VERBOSE    LOGLEV(8, 70, M_DEBUG)  /* show verbose info from gremlin module */
#define D_REL_DEBUG          LOGLEV(8, 70, M_DEBUG)  /* show detailed info from reliable routines */
#define D_EVENT_WAIT         LOGLEV(8, 70, M_DEBUG)  /* show detailed info from event waits */
#define D_MULTI_TCP          LOGLEV(8, 70, M_DEBUG)  /* show debug info from mtcp.c */

#define D_TLS_DEBUG          LOGLEV(9, 70, M_DEBUG)  /* show detailed info from TLS routines */
#define D_COMP               LOGLEV(9, 70, M_DEBUG)  /* show compression info */
#define D_READ_WRITE         LOGLEV(9, 70, M_DEBUG)  /* show all tun/tcp/udp reads/writes/opens */
#define D_PACKET_CONTENT     LOGLEV(9, 70, M_DEBUG)  /* show before/after encryption packet content */
#define D_TLS_NO_SEND_KEY    LOGLEV(9, 70, M_DEBUG)  /* show when no data channel send-key exists */
#define D_PID_DEBUG          LOGLEV(9, 70, M_DEBUG)  /* show packet-id debugging info */
#define D_PID_PERSIST_DEBUG  LOGLEV(9, 70, M_DEBUG)  /* show packet-id persist debugging info */
#define D_LINK_RW_VERBOSE    LOGLEV(9, 70, M_DEBUG)  /* show link reads/writes with greater verbosity */
#define D_STREAM_DEBUG       LOGLEV(9, 70, M_DEBUG)  /* show TCP stream debug info */
#define D_WIN32_IO           LOGLEV(9, 70, M_DEBUG)  /* win32 I/O debugging info */
#define D_PKCS11_DEBUG       LOGLEV(9, 70, M_DEBUG)  /* show PKCS#11 debugging */

#define D_SHAPER_DEBUG       LOGLEV(10, 70, M_DEBUG) /* show traffic shaper info */

#define D_REGISTRY           LOGLEV(11, 70, M_DEBUG) /* win32 registry debugging info */
#define D_OPENSSL_LOCK       LOGLEV(11, 70, M_DEBUG) /* show OpenSSL locks */

/*#define D_THREAD_DEBUG       LOGLEV(4, 70, M_DEBUG)*/  /* show pthread debug information */


int dont_mute (unsigned int flags); /* check muting filter */

#define MSG_TEST(flags) (unlikely((((unsigned int)flags) & M_DEBUG_LEVEL) <= x_debug_level))

# define msg x_msg
# define dmsg x_msg

void x_msg (const unsigned int flags, const char *format, ...)
#ifdef __GNUC__
    __attribute__ ((format (printf, 2, 3)))
#endif
    ; /* should be called via msg above */


void error_reset (void);



#define SDL_CONSTRAIN (1<<0)
int set_debug_level (const int level, const unsigned int flags);

int set_mute_cutoff (const int cutoff);

int get_debug_level (void);
int get_mute_cutoff (void);

/* Fatal logic errors */
#define ASSERT(x) do { if (!(x)) assert_failed(__FILE__, __FUNCTION__, __LINE__); } while (false)

void assert_failed (const char *filename, const char *funcname, int line);



/* Inline functions */

static inline int check_debug_level (unsigned int level)
{
  return (level & M_DEBUG_LEVEL) <= x_debug_level;
}


#endif

