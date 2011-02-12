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

void
dump_event(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     char buf[BUFSIZE] = { 0 };
     unsigned int i;

     for(i = 0; i < count; ++i)
     {
          if(i)
               strcat(buf, "|");

          strcat(buf, params[i]);
     }

     ui_print_buf(0, "[%s] *** (%s): %s", session->name, event, buf);

     return;
}

void
event_numeric(IrcSession *session, unsigned int event, const char *origin, const char **params, unsigned int count)
{
     char buf[BUFSIZE] = { 0 };
     char se[8] = { 0 };
     int i;

     sprintf(se, "%d", event);

     switch(event)
     {
          /* Basic message, just write it in status buffer */
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 250:
          case 251:
          case 252:
          case 253:
          case 254:
          case 255:
          case 265:
          case 266:
          case 372:
          case 375:
          case 123456:
               for(i = 1; i < count; ++i)
               {
                    strcat(buf, "|");

                    strcat(buf, params[i]);
               }

               ui_print_buf(0, "[%s] *** %s", session->name, buf + 1);
               break;

          /* Whois */
          case 275:
          case 301:
          case 307:
          case 311:
          case 312:
          case 313:
          case 317:
          case 318:
          case 319:
          case 320:
          case 330:
          case 378:
          case 671:
               event_whois(session, event, origin, params, count);
               break;

          /* Away */
          case 305:
          case 306:
               ui_print_buf(0, "[%s] *** %s", session->name, params[1]);
               break;

          /* List */
          case 321:
               ui_print_buf(0, "[%s] *** %s : %s", session->name, params[1], params[2]);
               break;
          case 322:
               ui_print_buf(0, "[%s] *** %s   %s : %s", session->name, params[1], params[2], params[3]);
               break;
          case 323:
               ui_print_buf(0, "[%s] *** %s", session->name, params[1]);
               break;

          /* Topic / Channel */
          case 332:
          case 333:
               event_topic(session, se, origin, params, count);
               break;
          case 328:
               ui_print_buf(find_bufid(session, params[1]),
                         "  *** Home page of %c%s%c: %s", B, params[1], B, params[2]);
               break;

          /* Names */
          case 353:
          case 366:
               event_names(session, se, origin, params, count);
               break;

          /* Identify */
          case 396:
               ui_print_buf(0, "[%s] *** %c%s%c(%s) %s", session->name, B, params[0], B, params[1], params[2]);
               break;

          /* Errors */
          case 401:
          case 403:
               ui_print_buf(find_bufid(session, params[1]), "[%s] *** %s", session->name, params[2]);
               break;
          case 263:
          case 402:
          case 404:
          case 412:
          case 421:
          case 437:
          case 461:
               ui_print_buf(0, "[%s] *** %s", session->name, params[2]);
               break;
          case 432:
          case 442:
          case 473:
               ui_print_buf(0, "[%s] *** %c%s%c: %s", session->name, B, params[1], B, params[2]);
               break;
          case 433:
               ui_print_buf(0, "[%s] *** Nickname is already in use", session->name);

               if(!strcmp(session->nick, params[1]))
               {
                   strcat(session->nick, "_");
                   irc_send_raw(session, "NICK %s", session->nick);
               }

               break;
          case 451:
               ui_print_buf(0, "[%s] *** You have not registered", session->name);
               break;
          case 482:
                ui_print_buf(hftirc->selbuf, "  *** <%s> You're not channel operator", params[1]);
               break;

          /* Redirection */
          case 470:
               ui_print_buf(0, "[%s] *** Channel %c%s%c linked on %c%s",
                         session->name, B, params[1], B, B, params[2]);

               if((i = find_bufid(session, params[1])))
                    strcpy(hftirc->cb[i].name, params[2]);

               break;

          case 479:
               ui_print_buf(0, "[%s] *** %c%s%c: %s", session->name, B, params[1], B, params[2]);

               if((i = find_bufid(session, params[1])))
                    ui_buf_close(i);

               ui_buf_set(0);
               break;

          /* End of MOTD, already managed by connect handle if motd is not received */
          case 376:
               /* Re-join every channel opened previously in the same session */
               if(session->motd_received)
               {
                    for(i = 0; i < hftirc->nbuf; ++i)
                         if(hftirc->cb[i].session == session
                                   && ISCHAN(hftirc->cb[i].name[0]))
                              irc_send_raw(session, "JOIN %s", hftirc->cb[i].name);
               }

               break;

          default:
               dump_event(session, se, origin, params, count);
               break;
     }

     return;
}

void
event_nick(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[NICKLEN] = { 0 };
     NickStruct *ns;

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     for(i = 0; i < hftirc->nbuf; ++i)
          for(ns = hftirc->cb[i].nickhead; ns; ns = ns->next)
               if(hftirc->cb[i].session == session && ns->nick && !strcmp(nick, ns->nick))
               {
                    hftirc->cb[i].umask |= (UNickSortMask | UNickListMask);

                    if(!strcmp(nick, session->nick))
                         strcpy(session->nick, params[0]);

                    ui_print_buf(i, "  *** %s is now %c%s", nick, B, params[0]);
                    strcpy(ns->nick, params[0]);
               }

     for(i = 0; i < hftirc->nbuf; ++i)
          if(!strcmp(nick, hftirc->cb[i].name) && session == hftirc->cb[i].session)
          {
               strcpy(hftirc->cb[i].name, params[0]);
               hftirc->cb[i].umask |= (UNickSortMask | UNickListMask);
          }

     return;
}

void
event_mode(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, b;
     char nick[NICKLEN] = { 0 };
     char nicks[BUFSIZE] = { 0 };
     char r[2] = { params[1][0], params[1][1] };
     NickStruct *ns;

     if(strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);
     else
          strcpy(nick, origin);

     /* User mode */
     if(count == 1)
     {
          ui_print_buf(0, "[%s] *** User mode of %c%s%c : [%s]", session->name, B, nick, B, params[0]);

          session->mode = strdup(params[0]);

          return;
     }

     for(i = 2; i < count; ++i)
     {
          strcat(nicks, " ");
          strcat(nicks, params[i]);
     }

     b = find_bufid(session, params[0]);

     for(ns = hftirc->cb[b].nickhead; ns; ns = ns->next)
          if(!strcasecmp(nicks + 1, ns->nick))
          {
               switch(r[1])
               {
                    case 'o':
                         ns->rang = (r[0] == '+') ? '@' : '\0';
                         break;
                    case 'v':
                         ns->rang = (r[0] == '+') ? '+' : '\0';
                         break;
                    case 'h':
                         ns->rang = (r[0] == '+') ? '%' : '\0';
                         break;
                    default:
                         break;
               }

               break;
          }

     ui_print_buf(b, "  *** Mode %c%s%c [%s %s] set by %c%s",
          B, params[0], B, params[1], nicks + 1, B, nick);

     hftirc->cb[b].umask |= UNickListMask;

     return;
}

void
event_connect(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;

     /* 123456 for write on buffer with default parse (event_numeric) */
     event_numeric(session, 123456, origin, params, count);

     hftirc->selsession = session;

     /* Find session by name */
     for(i = 0; strcmp(session->name, hftirc->conf.serv[i].name); ++i);

     for(j = 0; j < hftirc->conf.serv[i].nautojoin; ++j)
          input_join(hftirc->conf.serv[i].autojoin[j]);

     return;
}

void
event_join(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int j, i = 0;
     char nick[NICKLEN] = { 0 };
     NickStruct *ns;

     i = find_bufid(session, params[0]);

     if(origin && strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     if(!strcmp(nick, session->nick))
     {
          /* Check if the channel isn't already present on buffers */
          if(i != 0)
               ui_buf_set(i);
          /* Else, create a buffer */
          else
          {
               irc_join(session, params[0]);
               i = hftirc->nbuf - 1;
          }
     }

     ui_print_buf(i, colorstr("3", "  ->>>> %c%s%c (%s) has joined %c%s",
               B, nick, B, origin + strlen(nick) + 1, B, params[0]));

     ns = nickstruct_set(nick);

     nick_attach(i, ns);

     return;
}


void
event_part(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[NICKLEN] = { 0 };
     NickStruct *ns;

     irc_send_raw(session, "MODE %s +i", session->nick);

     i = find_bufid(session, params[0]);

     if(origin && strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     for(ns = hftirc->cb[i].nickhead; ns; ns = ns->next)
          if(ns->nick && strlen(ns->nick) && !strcmp(ns->nick, nick))
          {
               nick_detach(i, ns);
               free(ns);
          }

     ui_print_buf(i, colorstr("5", "  <<<<- %s (%s) has left %c%s%c [%s]",
               nick, origin + strlen(nick) + 1, B, params[0], B, (params[1] ? params[1] : "")));

     return;
}

void
event_quit(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[NICKLEN] = { 0 };
     NickStruct *ns;

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     for(i = 0; i < hftirc->nbuf; ++i)
          for(ns = hftirc->cb[i].nickhead; ns; ns = ns->next)
          {
               if(hftirc->cb[i].session == session && strlen(ns->nick) && !strcmp(nick, ns->nick))
               {
                    ui_print_buf(i, colorstr("4", "  <<<<- %s (%s) has quit [%s]", nick, origin + strlen(nick) + 1, params[0]));
                    nick_detach(i, ns);
                    free(ns);
                    break;
               }
          }

     return;
}

void
event_channel(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char r, nick[NICKLEN] = { 0 }, color[4] = { 0 };
     NickStruct *ns;

     i = find_bufid(session, params[0]);

     /* If the message is not from an old buffer, init a new one. */
     if(i > hftirc->nbuf)
     {
          ui_buf_new(params[0], session);
          i = hftirc->nbuf - 1;
     }

     if(origin && strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     /* Find nick in linked list of chan to find rang */
     for(ns = hftirc->cb[i].nickhead; ns; ns = ns->next)
          if(!strcasecmp(nick, ns->nick))
          {
               r = ns->rang;
               break;
          }

     if(hftirc->conf.serv && strstr(params[1], session->nick))
     {
          if(hftirc->conf.bell)
               putchar('\a');

          /* 8 -> Yellow for HL, see struct irccolor ui.c */
          strcpy(color, "8");
     }
     /* Enable nick_color if there is no hl -> no colors conflicts */
     else
          strcpy(nick, nick_color(nick));

     if(!ns->rang)
          ui_print_buf(i, colorstr(color, "<%s> %s", nick, params[1]));
     else
          ui_print_buf(i, colorstr(color, "<%c%c%c%s> %s", B, r, B, nick, params[1]));

     return;
}

void
event_privmsg(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[NICKLEN] = { 0 };
     NickStruct *ns;

     if(origin && strchr(origin, '!'))
          for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

     /* If the message is not from an old buffer, init a new one. */
     if(!(i = find_bufid(session, nick)))
     {
          ui_buf_new(nick, session);
          i = hftirc->nbuf - 1;
          ns = nickstruct_set(nick);
          nick_attach(i, ns);
     }

     ui_print_buf(i, "<%s> %s", nick, params[1]);

     if(hftirc->conf.bell)
          putchar('\a');

     return;
}

void
event_notice(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int j;
     char nick[NICKLEN] = { 0 };

     if(origin && strchr(origin, '!'))
          for(nick[0] = ' ', j = 0; origin[j] != '!'; nick[j + 1] = origin[j], ++j);

     ui_print_buf(0, "[%s] ***%s (%s)- %s", session->name, nick,
               ((origin + strlen(nick)) ? origin + strlen(nick) : nick), params[1]);

     return;
}

void
event_topic(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i, j;
     char nick[NICKLEN] = { 0 };

     i = find_bufid(session, params[((event[0] == '3') ? 1 : 0)]);

     if(!strcmp(event, "333"))
     {
          if(strchr(params[2], '!'))
               for(j = 0; params[2][j] != '!'; nick[j] = params[2][j], ++j);
          else
               strcpy(nick, params[2]);

          ui_print_buf(i, "  *** Set by %c%s%c (%s)", B, nick, B, params[3]);
     }
     else if(!strcmp(event, "332"))
     {
          ui_print_buf(i, "  *** Topic of %c%s%c: %s", B, params[1], B, params[2]);
          strcpy(hftirc->cb[i].topic, params[2]);
          hftirc->cb[i].umask |= UTopicMask;
     }
     else
     {
          if(origin && strchr(origin, '!'))
               for(j = 0; origin[j] != '!'; nick[j] = origin[j], ++j);

          ui_print_buf(i, "  *** New topic of %c%s%c set by %c%s%c: %s", B, params[0], B, B, nick, B, params[1]);

          strcpy(hftirc->cb[i].topic, params[1]);
          hftirc->cb[i].umask |= UTopicMask;
     }

     return;
}

void
event_names(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int s;
     unsigned int cn = 0;
     char *p, *str = " ";
     NickStruct *ns;

     s = find_bufid(session, params[1]);

     if(!strcmp(event, "366"))
     {
          nick_sort_abc(s);

          for(ns = hftirc->cb[s].nickhead; ns; ns = ns->next, ++cn)
               asprintf(&str, "%s %c%c%c%s", str, B, (ns->rang) ? ns->rang : ' ', B, ns->nick);

          ui_print_buf(s, "  *** Users of %c%s%c: %c%d%c nick(s)", B, params[1], B, B, cn, B);
          ui_print_buf(s, "%c[%c", B, B);
          ui_print_buf(s, " %s",  str);
          ui_print_buf(s, "%c]%c", B, B);

          hftirc->cb[s].naming = 0;

          free(str);
     }
     else
     {
          s = find_bufid(session, params[2]);

          /* Empty the list */
          if(!hftirc->cb[s].naming)
          {
               /* Free the entire tail queue. */
               for(ns = hftirc->cb[s].nickhead; ns; ns = ns->next)
               {
                    nick_detach(s, ns);
                    free(ns);
               }
          }

          p = strtok((char *)params[3], " ");

          while(p)
          {
               ns = nickstruct_set(p);

               nick_attach(s, ns);

               p = strtok(NULL, " ");
          }

          ++hftirc->cb[s].naming;

          ui_update_nicklistwin();
     }

     return;
}

void
event_action(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[NICKLEN] = { 0 };

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     if(!(i = find_bufid(session, params[0])))
          i = find_bufid(session, nick);

     ui_print_buf(i, " %c* %s%c %s", B, nick, B, params[1]);

     if(hftirc->conf.bell && hftirc->conf.serv && strstr(params[1], session->nick))
          putchar('\a');

     return;
}

void
event_kick(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char ornick[NICKLEN] = { 0 };

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; ornick[i] = origin[i], ++i);

     i = find_bufid(session, params[0]);

     ui_print_buf(i, "  *** %c%s%c kicked by %s from %c%s%c [%s]",
               B, params[1], B, ornick, B, params[0], B, params[2]);

     return;
}

void
event_whois(IrcSession *session, unsigned int event, const char *origin, const char **params, unsigned int count)
{
     int b = 0;
     char *n = session->name;

     b = find_bufid(session, params[1]);

     switch(event)
     {
          /* Whois operator/registered/securingconnection */
          case 275:
          case 307:
          case 313:
          case 320:
          case 671:
               ui_print_buf(b, "[%s] ***           %s: %s", n, params[1], params[2]);
               break;

          /* Whois user */
          case 311:
               ui_print_buf(b, "[%s] *** %c%s%c (%s@%s)", n,  B, params[1], B, params[2], params[3]);
               ui_print_buf(b, "[%s] *** IRCNAME:  %s", n, params[5]);
               break;

          /* Whois server */
          case 312:
               ui_print_buf(b, "[%s] *** SERVER:   %s (%s)", n, params[2], params[3]);
               break;

          /* Whois away */
          case 301:
               /* When whois */
               ui_print_buf(b, "[%s] *** AWAY:     %s", n,  params[2]);
               break;

          /* Whois idle */
          case 317:
               ui_print_buf(b, "[%s] *** IDLE:     seconds idle: %s signon time: %s", n, params[2], params[3]);
               break;

          /* End of whois */
          case 318:
               ui_print_buf(b, "[%s] *** %s", n, params[2]);
               break;

          /* Whois channel */
          case 319:
               ui_print_buf(b, "[%s] *** CHANNELS: %s", n, params[2]);
               break;

          /* Whois account */
          case 330:
               ui_print_buf(b, "[%s] ***           %s: %s %s", n, params[1], params[3], params[2]);
               break;
     }

     return;
}

void
event_invite(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[NICKLEN] = { 0 };

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     ui_print_buf(0, "[%s] *** You've been invited by %c%s%c to %c%s", session->name, B, nick, B, B, params[1]);

     return;
}

void
event_ctcp(IrcSession *session, const char *event, const char *origin, const char **params, unsigned int count)
{
     int i;
     char nick[NICKLEN] = { 0 };
     char reply[256] = { 0 };
     struct utsname un;

     if(origin && strchr(origin, '!'))
          for(i = 0; origin[i] != '!'; nick[i] = origin[i], ++i);

     i = find_bufid(session, nick);

     ui_print_buf(i, "[%s] *** %c%s%c (%s) CTCP request: %c%s%c",
               session->name, B, nick, B, origin + strlen(nick) + 1, B, params[0], B);

     if(!strcasecmp(params[0], "VERSION"))
     {
          uname(&un);

          sprintf(reply, "%s HFTirc "HFTIRC_VERSION" - on %s %s", params[0], un.sysname, un.machine);
          irc_send_raw(session, "NOTICE %s :\x01%s\x01", nick, reply);
     }

     return;
}
