#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/reboot.h>
#include <libintl.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Con.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include <libkeys.h>

#ifndef DATADIR
#define DATADIR "."
#endif

#define POWER "Power"

Ecore_Evas *main_win;

void exit_all(void* param) { ecore_main_loop_quit(); }

static void die(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

typedef struct
{
    char* msg;
    int size;
} client_data_t;

static void shutdown()
{
	printf("shutdown\n");
	sync();
	system("poweroff");
}

static void
key_handler(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
    const char* action = keys_lookup_by_event((keys_t*)data, "default",
                                              (Evas_Event_Key_Up*)event_info);

    if(action && !strcmp(action, "Shutdown"))
        shutdown();
    else if(action && !strcmp(action, "Close"))
        ecore_evas_hide(main_win);
}


static int _client_add(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Add* e = ev;
    client_data_t* msg = malloc(sizeof(client_data_t));
    msg->msg = strdup("");
    msg->size = 0;
    ecore_con_client_data_set(e->client, msg);
    return 0;
}

static int _client_del(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Del* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);

    /* Handle */
	if(strlen(POWER) == msg->size && !strncmp(POWER, msg->msg, msg->size))
		if(ecore_evas_visibility_get(main_win))
			ecore_evas_raise(main_win);
		else {
			ecore_evas_show(main_win);
		}

    //printf(": %.*s(%d)\n", msg->size, msg->msg, msg->size);


    free(msg->msg);
    free(msg);
    return 0;
}

static int _client_data(void* param, int ev_type, void* ev)
{
    Ecore_Con_Event_Client_Data* e = ev;
    client_data_t* msg = ecore_con_client_data_get(e->client);
    msg->msg = realloc(msg->msg, msg->size + e->size);
    memcpy(msg->msg + msg->size, e->data, e->size);
    msg->size += e->size;
    return 0;
}

static void main_win_resize_handler(Ecore_Evas* main_win)
{
	ecore_evas_hide(main_win);
	int w, h;
	Evas* canvas = ecore_evas_get(main_win);
	evas_output_size_get(canvas, &w, &h);

	Evas_Object* edje = evas_object_name_find(canvas, "edje");
	evas_object_resize(edje, w, h);
	ecore_evas_show(main_win);
}

int main(int argc, char **argv)
{
	if(!evas_init())
		die("Unable to initialize Evas\n");
	if(!ecore_init())
		die("Unable to initialize Ecore\n");
	if(!ecore_con_init())
		die("Unable to initialize Ecore_Con\n");
	if(!ecore_evas_init())
		die("Unable to initialize Ecore_Evas\n");
	if(!edje_init())
		die("Unable to initialize Edje\n");

	setlocale(LC_ALL, "");
	textdomain("eshutdown");

    keys_t* keys = keys_alloc("eshutdown");

	ecore_con_server_add(ECORE_CON_LOCAL_USER, "eshutdown", 0, NULL);

	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_ADD, _client_add, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DATA, _client_data, NULL);
	ecore_event_handler_add(ECORE_CON_EVENT_CLIENT_DEL, _client_del, NULL);

	ecore_x_io_error_handler_set(exit_all, NULL);

	main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
	ecore_evas_borderless_set(main_win, 0);
	ecore_evas_title_set(main_win, "eshutdown");
	ecore_evas_name_class_set(main_win, "eshutdown", "eshutdown");

	ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

	Evas *main_canvas = ecore_evas_get(main_win);

	Evas_Object *edje = edje_object_add(main_canvas);
	evas_object_name_set(edje, "edje");
	edje_object_file_set(edje, DATADIR "/eshutdown/themes/eshutdown.edj", "eshutdown");
	evas_object_move(edje, 0, 0);
	evas_object_resize(edje, 600, 800);
	evas_object_show(edje);
	evas_object_focus_set(edje, 1);
	evas_object_event_callback_add(edje, EVAS_CALLBACK_KEY_UP, &key_handler, keys);

	edje_object_part_text_set(edje, "eshutdown/title", gettext("Power Off"));
	char *t;
	asprintf(&t, "%s<br><br>%s",
			gettext("Power off - press \"OK\""),
			gettext("Cancel - press \"C\""));
	edje_object_part_text_set(edje, "eshutdown/text", t);
	free(t);

//	ecore_evas_show(main_win);
	ecore_main_loop_begin();

	edje_shutdown();
	ecore_evas_shutdown();
	ecore_con_shutdown();
	ecore_shutdown();
	evas_shutdown();

    keys_free(keys);

	return 0;
}
