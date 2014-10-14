/*
 * This file is part of wysiwidi
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * Contact: Alexander Kanavin <alex.kanavin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */


#include <glib.h>
#include <iostream>
#include <glib-unix.h>

#include "mirac-gst.hpp"

static gboolean _sig_handler (gpointer data_ptr)
{
    GMainLoop *ml = (GMainLoop *) data_ptr;

    g_main_loop_quit(ml);

    return G_SOURCE_CONTINUE;
}


int main (int argc, char *argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    GMainLoop* ml = NULL;
    
    gchar* wfd_device_option = NULL;
    gchar* wfd_stream_option = NULL;
    gchar* hostname_option = NULL;
    gint port = 0;
    
    GOptionEntry main_entries[] =
    {
        { "device", 0, 0, G_OPTION_ARG_STRING, &wfd_device_option, "Specify WFD device type: source or sink", "(source|sink)"},
        { "stream", 0, 0, G_OPTION_ARG_STRING, &wfd_stream_option, "Specify WFD stream type: audio, video or both", "(audio|video|both)"},
        { "hostname", 0, 0, G_OPTION_ARG_STRING, &hostname_option, "Specify optional hostname or ip address to stream to or listen on", "host"},
        { "port", 0, 0, G_OPTION_ARG_INT, &port, "Specify UDP port number to stream to or listen on", "port"},
        { NULL }
    };

    context = g_option_context_new ("- WFD source/sink demo application\n\nExample:\ngst-test --device=source --stream=both --hostname=127.0.0.1 --port=5000\ngst-test --device=sink --stream=both --port=5000");
    g_option_context_add_main_entries (context, main_entries, NULL);
    
   if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        g_option_context_free(context);
        exit (1);
    }
    g_option_context_free(context);

    wfd_device_t wfd_device = WFD_UNKNOWN_DEVICE;
    if (g_strcmp0(wfd_device_option, "source") == 0)
        wfd_device = WFD_SOURCE;
    else if (g_strcmp0(wfd_device_option, "sink") == 0)
        wfd_device = WFD_SINK;
    
    wfd_stream_t wfd_stream = WFD_UNKNOWN_STREAM;
    if (g_strcmp0(wfd_stream_option, "audio") == 0)
        wfd_stream = WFD_AUDIO;
    else if (g_strcmp0(wfd_stream_option, "video") == 0)
        wfd_stream = WFD_VIDEO;
    else if (g_strcmp0(wfd_stream_option, "both") == 0)
        wfd_stream = WFD_BOTH;

    std::string hostname;
    if (hostname_option)
        hostname = hostname_option;

    g_free(wfd_device_option);
    g_free(wfd_stream_option);
    g_free(hostname_option);

    gst_init (&argc, &argv);
    
    MiracGst gst_pipeline(wfd_device, wfd_stream, hostname, port);
    
    ml = g_main_loop_new(NULL, TRUE);
    g_unix_signal_add(SIGINT, _sig_handler, ml);
    g_unix_signal_add(SIGTERM, _sig_handler, ml);

    g_main_loop_run(ml);

    g_main_loop_unref(ml);
    
    return 0;
}

  