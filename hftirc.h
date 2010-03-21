#ifndef HFTIRC_H
#define HFTIRC_H

/* Libs */
#include <ncurses.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <locale.h>

#include <libircclient.h>
#include <libirc_events.h>

/* Macro */
#define HFTIRC_VERSION    "0.01"
#define BUFSIZE           8192
#define MAXBUF            64
#define BUFLINES          512
#define NICKLEN           24
#define CHANLEN           24
#define HOSTLEN           128
#define NSERV             32
#define HFTIRC_KEY_ENTER  10
#define MAINWIN_LINES     LINES - 2
#define DATELEN           (strlen(hftirc->date.str))
#define CONFPATH          "hftirc.conf"
#define B                 C('B')
#define U                 C('_')

#define C(c) ((c) & 037)
#define LEN(x) (sizeof(x)/sizeof(x[0]))
#define WARN(t,s) ui_print_buf(0, "%s: %s", t, s)
#define DSINPUT(i) for(; i && i[0] == ' '; ++i)
#define PRINTATTR(w, attr, s)  wattron(w, attr); waddstr(w, s); wattroff(w, attr);
#define NOSERVRET(r) if(!hftirc->conf.nserv)                     \
                     {                                           \
                          WARN("Error", "You're not connected"); \
                          return r;                              \
                     }

/* Structures */
typedef struct
{
     /* Ncurses windows */
     WINDOW *mainwin;
     WINDOW *inputwin;
     WINDOW *statuswin;
     WINDOW *topicwin;

     /* Input buffer struct */
     struct
     {
          wchar_t buffer[BUFSIZE];
          int pos;
          int cpos;
          int split;
          int spting;
     } ib;
} Ui;

/* Channel buffer */
typedef struct
{
     /* For ui use */
     char *buffer[BUFLINES];
     int bufpos, scrollpos, naming;

     /* For irc info */
     unsigned int sessid;
     char name[HOSTLEN];
     char *names;
     char topic[BUFSIZE];
     int act;
} ChanBuf;

/* Irc color struct */
typedef struct
{
     short color;
     unsigned int mask;
} IrcColor;

/* Date struct */
typedef struct
{
     struct tm *tm;
     time_t lt;
     char str[256];
} DateStruct;

/* Input struct */
typedef struct
{
     char *cmd;
     void (*func)(const char *arg);
} InputStruct;

/* Server information struct */
typedef struct
{
     char name[HOSTLEN];
     char adress[HOSTLEN];
     char password[128];
     int port;
     char nick[NICKLEN];
     char mode[NICKLEN];
     char username[256];
     char realname[256];
     char autojoin[128][CHANLEN];
     int nautojoin;
     unsigned int bname;
} ServInfo;

/* Config struct */
typedef struct
{
     char path[512];
     char datef[256];
     int nserv;
     ServInfo serv[NSERV];

} ConfStruct;

/* Global struct */
typedef struct
{
     int ft, running;
     int nbuf, selbuf, selses;
     irc_session_t *session[NSERV];
     irc_callbacks_t callbacks;
     ConfStruct conf;
     ChanBuf *cb;
     Ui *ui;
     DateStruct date;
} HFTIrc;


/* Prototypes */

/* config.c */
void config_parse(char *file);

/* ui.c */
void ui_init(void);
void ui_update_statuswin(void);
void ui_update_topicwin(void);
void ui_update_infowin(void);
void ui_manage_print_color(int i, char *str, int *mask);
void ui_print(WINDOW *w, char *str);
void ui_print_buf(int id, char *format, ...);
void ui_draw_buf(int id);
void ui_buf_new(const char *name, unsigned int id);
void ui_buf_close(int buf);
void ui_buf_set(int buf);
void ui_scroll_up(int buf);
void ui_scroll_down(int buf);
void ui_get_input(void);

/* irc.c */
void irc_init(void);
void irc_join(irc_session_t *session, const char *chan);

void irc_dump_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_numeric(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void irc_event_nick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_mode(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_part(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_quit(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_channel(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count);
void irc_event_privmsg(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_notice(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_topic(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_names(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_action(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_kick(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void irc_event_whois(irc_session_t *session, unsigned int event, const char *origin, const char **params, unsigned int count);
void irc_event_invite(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);

/* input.c */
void input_manage(const char *input);
void input_join(const char *input);
void input_nick(const char *input);
void input_quit(const char *input);
void input_help(const char *input);
void input_names(const char *input);
void input_topic(const char *input);
void input_part(const char *input);
void input_me(const char *input);
void input_msg(const char *input);
void input_kick(const char *input);
void input_whois(const char *input);
void input_query(const char *input);
void input_close(const char *input);
void input_raw(const char *input);
void input_umode(const char *input);
void input_serv(const char *input);
void input_redraw(const char *input);
void input_connect(const char *input);
void input_disconnect(const char *input);

/* util.c */
void update_date(void);
int find_bufid(unsigned id, const char *str);
int find_sessid(irc_session_t *session);

/* main.c */
void signal_handler(int signal);
void draw_logo(void);

/* Variables */
HFTIrc *hftirc;

#endif /* HFTIRC_H */
