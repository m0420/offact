/* Copyright (C) 2024 OffAct Contributors

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.  */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "notify.h"


/* ------------------------------------------------------------------ */
/* PS5 system notification request struct (standard PS5 scene layout)  */
/* ------------------------------------------------------------------ */

typedef struct SceNotificationRequest
{
    int   type;         /* 0 = standard toast                         */
    int   unk1;         /* -1                                         */
    int   unk2;         /* 0                                          */
    int   use_icon_uri; /* 0 = use default icon                       */
    char  icon_uri[1024];
    char  message[1024];
    int   unk3;         /* 0                                          */
    char  unk4[12];
} SceNotificationRequest;

/* Provided by libkernel; no header needed, extern is sufficient. */
extern int sceKernelSendNotificationRequest(int dev,
                                            SceNotificationRequest *req,
                                            size_t size,
                                            int blocking);


/* ------------------------------------------------------------------ */
/* Internal helper                                                      */
/* ------------------------------------------------------------------ */

static void send_notification(const char *msg)
{
    SceNotificationRequest req;
    memset(&req, 0, sizeof(req));
    req.type         =  0;
    req.unk1         = -1;
    req.unk2         =  0;
    req.use_icon_uri =  0;
    req.unk3         =  0;

    snprintf(req.message, sizeof(req.message), "%s", msg);

    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}


/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

/**
 * Debug toast – prefixed with "[DBG]", appears as normal PS5 notification.
 **/
void notify_dbg(const char *fmt, ...)
{
    char buf[1024];
    char msg[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    snprintf(msg, sizeof(msg), "[DBG] %s", buf);
    send_notification(msg);
}


/**
 * Error toast – prefixed with "[ERR]" to clearly flag unexpected flow.
 **/
void notify_err(const char *fmt, ...)
{
    char buf[1024];
    char msg[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    snprintf(msg, sizeof(msg), "[ERR] %s", buf);
    send_notification(msg);
}


/* Local Variables: */
/* tab-width: 8 */
/* c-basic-offset: 4 */
/* End: */
