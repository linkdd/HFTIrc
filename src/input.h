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

static const InputStruct input_struct[] =
{
     { "away",            input_away },
     { "buffer",          input_buffer },
     { "buffer_list",     input_buffer_list },
     { "buffer_prev",     input_buffer_prev},
     { "buffer_swap",     input_buffer_swap },
     { "clear",           input_clear },
     { "close",           input_close },
     { "color_theme",     input_color_theme },
     { "connect",         input_connect },
     { "ctcp",            input_ctcp },
     { "disconnect",      input_disconnect },
     { "help",            input_help },
     { "invite",          input_invite },
     { "join",            input_join },
     { "kick",            input_kick },
     { "me",              input_me },
     { "mode",            input_mode },
     { "msg",             input_msg },
     { "names",           input_names },
     { "nick",            input_nick },
     { "part",            input_part },
     { "query",           input_query },
     { "quit",            input_quit },
     { "raw",             input_raw },
     { "reconnect",       input_reconnect },
     { "redraw",          input_redraw },
     { "nicklist_scroll", input_nicklist_scroll },
     { "nicklist_toggle", input_nicklist_toggle },
     { "say",             input_say },
     { "scrollclear",     input_scrollclear },
     { "serv",            input_serv },
     { "server",          input_connect },
     { "topic",           input_topic },
     { "umode",           input_umode },
     { "whois",           input_whois },
     { "/",               input_say },
};


