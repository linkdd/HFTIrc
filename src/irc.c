/*
 * Copyright (c) 2010 Martin Duquesnoy <xorg62@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "hftirc.h"

IrcSession*
irc_session(void)
{
     IrcSession *s;

     s = calloc(1, sizeof(IrcSession));

     s->sock = -1;
     s->connected = 0;

     return s;
}

int
irc_connect(IrcSession *s,
          const char *server,
          unsigned short port,
          const char *password,
          const char *nick,
          const char *username,
          const char *realname)
{
     struct hostent *hp;
     struct sockaddr_in a;

     if (!server || !nick)
          return 1;

     s->username = (username) ? strdup(username) : NULL;
     s->password = (password) ? strdup(password) : NULL;
     s->realname = (realname) ? strdup(realname) : NULL;

     s->nick   = strdup(nick);
     s->server = strdup(server);

     if(!(hp = gethostbyname(server)))
          return 1;

     a.sin_family = AF_INET;
     a.sin_port   = htons(s->port ? s->port : 6667);

     memcpy(&a.sin_addr, hp->h_addr_list[0], (size_t) hp->h_length);

     /* Create socket */
     if((s->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
          return 1;

     /* connect to the server */
     if(connect(s->sock, (const struct sockaddr*) &a, sizeof(a)) < 0)
          return 1;

     /* Identify */
    irc_send_raw(s, "PASS %s", password);
    irc_send_raw(s, "NICK %s", nick);
    irc_send_raw(s, "USER %s localhost %s :%s", username, server, realname);

    s->connected = 1;

    return 0;
}

void
irc_disconnect(IrcSession *s)
{
     if(s->sock >= 0)
          close(s->sock);

     s->sock = -1;

     s->connected = 0;

     return;
}

int
irc_run_process(IrcSession *session, fd_set *inset, fd_set *outset)
{
     int length, offset;
     unsigned int amount;

     if(session->sock < 0 || !session->connected)
          return 1;

     /* Read time */
     if(FD_ISSET(session->sock, inset))
     {
          amount = (sizeof(session->inbuf) - 1) - session->inoffset;

          if((length = recv(session->sock, session->inbuf + session->inoffset, amount, 0)) <= 0)
               return 1;

          session->inoffset += length;

          /* Process the incoming data */
          for(; ((offset = irc_findcrlf(session->inbuf, session->inoffset)) > 0); session->inoffset -= offset)
          {
               irc_manage_event(session, offset - 2);

               if(session->inoffset - offset > 0)
                    memmove(session->inbuf, session->inbuf + offset, session->inoffset - offset);
          }
     }
     /* Send time */
     if(FD_ISSET(session->sock, outset))
     {
          if((length = send(session->sock, session->outbuf, session->outoffset, 0)) <= 0)
               return 1;

          if(session->outoffset - length > 0)
               memmove (session->outbuf, session->outbuf + length, session->outoffset - length);

          session->outoffset -= length;
     }

     return 0;
}

int
irc_add_select_descriptors(IrcSession *session, fd_set *inset, fd_set *outset, int *maxfd)
{
     if(session->sock < 0 || !session->connected)
          return 1;

     if(session->inoffset < (sizeof (session->inbuf) - 1))
          FD_SET(session->sock, inset);

     if(irc_findcrlf(session->outbuf, session->outoffset) > 0)
          FD_SET(session->sock, outset);

     if(*maxfd < session->sock)
          *maxfd = session->sock;

     return 0;
}

int
irc_send_raw(IrcSession *session, const char *format, ...)
{
     char buf[BUFSIZE];
     va_list va_alist;

     va_start(va_alist, format);
     vsnprintf(buf, sizeof(buf), format, va_alist);
     va_end(va_alist);

     if((strlen(buf) + 2) >= (sizeof(session->outbuf) - session->outoffset))
          return 1;

     strcpy(session->outbuf + session->outoffset, buf);

     session->outoffset += strlen(buf);
     session->outbuf[session->outoffset++] = '\r';
     session->outbuf[session->outoffset++] = '\n';

     return 0;
}

void
irc_parse_in(char *buf,
          const char *prefix,
          const char *command,
          const char **params,
          int *code,
          int *paramindex)
{
     char *p = buf, *s = NULL;

     /* Parse prefix */
     if(buf[0] == ':')
     {
          for(; *p && *p != ' '; ++p);
          *p++ = '\0';
          strcpy((char *)prefix, buf + 1);
     }

     /* Parse command */
     if(isdigit(p[0]) && isdigit(p[1]) && isdigit(p[2]))
     {
          p[3] = '\0';
          *code = atoi (p);
          p += 4;
     }
     else
     {
          for(s = p; *p && *p != ' '; ++p);
          *p++ = '\0';
          strcpy((char *)command, s);
     }

     /* Parse params */
     for(;*p && *paramindex < 10; *p++ = '\0')
     {
          if(*p == ':')
          {
               params[(*paramindex)++] = p + 1;
               break;
          }

          for(s = p; *p && *p != ' '; ++p);

          params[(*paramindex)++] = s;

          if(!*p)
               break;
     }

     return;
}

void
irc_manage_event(IrcSession *session, int process_length)
{
     char buf[BUFSIZE], ctcp_buf[128];
     const char command[BUFSIZE] = { 0 };
     const char prefix[BUFSIZE] =  { 0 };
     const char *params[11];
     int code = 0, paramindex = 0;
     unsigned int msglen;

     if(process_length > sizeof(buf))
          return;

     memcpy(buf, session->inbuf, process_length);
     buf[process_length] = '\0';

     memset((char *)params, 0, sizeof(params));

     /* Parse socket */
     irc_parse_in(buf, prefix, command, params, &code, &paramindex);

     /* Handle auto PING/PONG */
     if(strlen(command) && !strcmp(command, "PING") && params[0])
     {
          irc_send_raw(session, "PONG %s", params[0]);
          return;
     }

     /* Numerical */
     if(code)
     {
          if((code == 376 || code == 422) && !session->motd_received)
          {
               session->motd_received = 1;
               event_connect(session, "CONNECT", prefix, params, paramindex);
          }

          event_numeric(session, code, prefix, params, paramindex);
     }
     else
     {
          /* Quit */
          if(!strcmp(command, "QUIT"))
               event_quit(session, command, prefix, params, paramindex);

          /* Join */
          else if(!strcmp(command, "JOIN"))
               event_join(session, command, prefix, params, paramindex);

          /* Part */
          else if(!strcmp(command, "PART"))
               event_part(session, command, prefix, params, paramindex);

          /* Invite */
          else if(!strcmp(command, "INVITE"))
               event_invite(session, command, prefix, params, paramindex);

          /* Topic */
          else if(!strcmp (command, "TOPIC"))
               event_topic(session, command, prefix, params, paramindex);

          /* Kick */
          else if (!strcmp(command, "KICK"))
               event_kick(session, command, prefix, params, paramindex);

          /* Nick */
          else if(!strcmp(command, "NICK"))
          {
               if(!strncmp(prefix, session->nick, sizeof(session->nick)) && paramindex > 0)
               {
                    free(session->nick);
                    session->nick = strdup(params[0]);
               }

               event_nick(session, command, prefix, params, paramindex);
          }

          /* Mode / User mode */
          else if(!strcmp(command, "MODE"))
          {
               /* User mode  case */
               if(paramindex > 0 && !strcmp(params[0], session->nick))
               {
                    params[0] = params[1];
                    paramindex = 1;
               }

               event_mode(session, command, prefix, params, paramindex);
          }

          /* Privmsg */
          else if(!strcmp(command, "PRIVMSG"))
          {
               if(paramindex > 1)
               {
                    msglen = strlen(params[1]);

                    /* CTCP request */
                    if(params[1][0] == 0x01 && params[1][msglen - 1] == 0x01)
                    {
                         msglen -= 2;

                         if(msglen > sizeof(ctcp_buf) - 1)
                              msglen = sizeof(ctcp_buf) - 1;

                         memcpy(ctcp_buf, params[1] + 1, msglen);
                         ctcp_buf[msglen] = '\0';

                         if(strstr(ctcp_buf, "ACTION ") == ctcp_buf)
                         {
                              params[1] = ctcp_buf + strlen("ACTION ");
                              paramindex = 2;
                              event_action(session, "ACTION", prefix, params, paramindex);
                         }
                         else
                         {
                              params[0] = ctcp_buf;
                              paramindex = 1;
                              event_ctcp(session, "CTCP", prefix, params, paramindex);
                         }
                    }
                    /* Private message */
                    else if(!strcmp(params[0], session->nick))
                         event_privmsg(session, command, prefix, params, paramindex);
                    /* Channel message */
                    else
                         event_channel(session, command, prefix, params, paramindex);
               }

          }

          /* Notice */
          else if(!strcmp(command, "NOTICE"))
          {
               msglen = strlen(params[1]);

               /* CTCP request */
               if(paramindex > 1 && params[1][0] == 0x01 && params[1][msglen-1] == 0x01)
               {
                    msglen -= 2;

                    if(msglen > sizeof(ctcp_buf) - 1)
                         msglen = sizeof(ctcp_buf) - 1;

                    memcpy(ctcp_buf, params[1] + 1, msglen);
                    ctcp_buf[msglen] = '\0';

                    params[0] = ctcp_buf;
                    paramindex = 1;

                    dump_event(session, "CTCP", prefix, params, paramindex);
               }
               /* Normal notice */
               else
                    event_notice(session, command, prefix, params, paramindex);
          }

          /* Unknown */
          else
               dump_event(session, command, prefix, params, paramindex);
     }

     return;
}

int
irc_findcrlf(const char *buf, int length)
{
     int offset = 0;

     for(; offset < (length - 1); ++offset)
          if(buf[offset] == '\r' && buf[offset + 1] == '\n')
               return offset + 2;

     return 0;
}

void
irc_init(void)
{
     int i;

     hftirc->selses = 0;

     /* Connection to conf servers */
     for(i = 0; i < hftirc->conf.nserv; ++i)
     {
          hftirc->session[i] = irc_session();
          if(irc_connect(hftirc->session[i],
                         hftirc->conf.serv[i].adress,
                         hftirc->conf.serv[i].port,
                         hftirc->conf.serv[i].password,
                         hftirc->conf.serv[i].nick,
                         hftirc->conf.serv[i].username,
                         hftirc->conf.serv[i].realname))
               ui_print_buf(0, "Error: Can't connect to %s", hftirc->conf.serv[i].adress);
     }

     return;
}

void
irc_join(IrcSession *session, const char *chan)
{
     int s;

     s = find_sessid(session);

     ui_buf_new(chan, s);

     ui_buf_set(hftirc->nbuf - 1);

     return;
}


