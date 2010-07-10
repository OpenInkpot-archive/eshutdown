/*
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
 * Copyright © 2009,2010 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <libintl.h>
#include <err.h>
#include <locale.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Con.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include <libkeys.h>
#include <libeoi_themes.h>
#include <libeoi_utils.h>
#include <libeoi_dialog.h>

#define POWER "Power"

Ecore_Evas *main_win;

void
exit_all(void *param)
{
    ecore_main_loop_quit();
}

typedef struct {
    char *msg;
    int size;
} client_data_t;

static void
_shutdown()
{
    system("poweroff");
}

static void
key_handler(void *data, Evas * evas, Evas_Object * obj, void *event_info)
{
    const char *action = keys_lookup_by_event((keys_t *) data, "default",
                                              (Evas_Event_Key_Up *)
                                              event_info);

    if (action && !strcmp(action, "Shutdown"))
        _shutdown();
    else if (action && !strcmp(action, "Close"))
        ecore_evas_hide(main_win);
}


static Eina_Bool
_client_add(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Add *e = ev;
    client_data_t *msg = malloc(sizeof(client_data_t));
    msg->msg = strdup("");
    msg->size = 0;
    ecore_con_client_data_set(e->client, msg);
    return 0;
}

static Eina_Bool
_client_del(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Del *e = ev;
    client_data_t *msg = ecore_con_client_data_get(e->client);

    /* Handle */
    if (strlen(POWER) == msg->size && !strncmp(POWER, msg->msg, msg->size)) {
        if (ecore_evas_visibility_get(main_win))
            ecore_evas_raise(main_win);
        else {
            ecore_evas_show(main_win);
        }
    }

    free(msg->msg);
    free(msg);
    return 0;
}

static Eina_Bool
_client_data(void *param, int ev_type, void *ev)
{
    Ecore_Con_Event_Client_Data *e = ev;
    client_data_t *msg = ecore_con_client_data_get(e->client);
    msg->msg = realloc(msg->msg, msg->size + e->size);
    memcpy(msg->msg + msg->size, e->data, e->size);
    msg->size += e->size;
    return 0;
}

int
main(int argc, char **argv)
{
    if (!ecore_x_init(NULL))
        errx(1, "Unable to initialize Ecore_X, maybe DISPLAY is not set");
    if (!ecore_con_init())
        errx(1, "Unable to initialize Ecore_Con\n");
    if (!ecore_evas_init())
        errx(1, "Unable to initialize Ecore_Evas\n");
    if (!edje_init())
        errx(1, "Unable to initialize Edje\n");

    setlocale(LC_ALL, "");
    textdomain("eshutdown");

    keys_t *keys = keys_alloc("eshutdown");

    ecore_con_server_add(ECORE_CON_LOCAL_SYSTEM, "eshutdown", 0, NULL);

    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, _client_add, NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, _client_data,
                            NULL);
    ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, _client_del, NULL);

    ecore_x_io_error_handler_set(exit_all, NULL);

    main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
    ecore_evas_borderless_set(main_win, 0);
    ecore_evas_title_set(main_win, "eshutdown");
    ecore_evas_name_class_set(main_win, "eshutdown", "eshutdown");

    Evas *main_canvas = ecore_evas_get(main_win);

    Evas_Object *edje
        = eoi_create_themed_edje(main_canvas, "eshutdown", "eshutdown");

    Evas_Object *dlg = eoi_dialog_create("dlg", edje);
    eoi_dialog_title_set(dlg, gettext("Power Off"));
    evas_object_move(dlg, 0, 0);
    evas_object_resize(dlg, 600, 800);
    ecore_evas_object_associate(main_win, dlg, 0);

    evas_object_focus_set(edje, 1);
    evas_object_event_callback_add(edje, EVAS_CALLBACK_KEY_UP,
                                   &key_handler, keys);

    char *t = xasprintf("%s<br><br>%s",
                        gettext("Power off - press \"OK\""),
                        gettext("Cancel - press \"C\""));
    edje_object_part_text_set(edje, "eshutdown/text", t);
    free(t);

    Evas_Object *icon = eoi_create_themed_edje(main_canvas, "eshutdown", "icon");
    edje_object_part_swallow(dlg, "icon", icon);

    evas_object_show(dlg);

    ecore_main_loop_begin();

    keys_free(keys);

    ecore_evas_shutdown();
    edje_shutdown();
    ecore_con_shutdown();
    ecore_x_shutdown();

    return 0;
}
