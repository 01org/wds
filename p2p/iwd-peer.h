/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2020 Intel Corporation.
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

#include <gio/gio.h>
#include "peer.h"

#ifndef IWD_PEER_H_
#define IWD_PEER_H_

namespace P2P {

class IWDPeer : public Peer {
    public:
        IWDPeer(const char *path);
        virtual ~IWDPeer();

        void connect() override;
        void disconnect() override;

        bool is_available() const override { return connected_; }
        void set_peer_proxy(GDBusProxy *proxy);
        void set_wfd_proxy(GDBusProxy *proxy);
        void set_wsc_proxy(GDBusProxy *proxy) { g_set_object(&wsc_proxy_, proxy); }
        bool is_ready() const { return peer_proxy_ && wfd_proxy_ && wsc_proxy_; }
        const char *get_path() const { return path_; }
        const char *get_dev_path() const { return dev_path_; }
        void added(void);
        void removed(void);

    protected:
        static void connect_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void cancel_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void disconnect_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static const char *get_str_property(GVariant *value, const GVariantType *type, const char *name);
        static bool get_bool_property(GVariant *value, const char *name, bool *out);
        static bool get_uint16_property(GVariant *value, const char *name, int *out);
        static void peer_properties_changed_cb(GDBusProxy *proxy, GVariant *changed_properties,
                                               GStrv invalidate_properties, gpointer user_data);
        static void wfd_properties_changed_cb(GDBusProxy *proxy, GVariant *changed_properties,
                                              GStrv invalidate_properties, gpointer user_data);

        void local_host_update(const char *ifname);
        void dev_type_update(void);
        void check_available(void);
        void peer_property_update(const char *name, GVariant *value);
        void wfd_property_update(const char *name, GVariant *value);

        Parameters params_;
        GDBusProxy *peer_proxy_;
        GDBusProxy *wfd_proxy_;
        GDBusProxy *wsc_proxy_;
        char *path_;
        char *dev_path_;
        bool connected_;
        GCancellable *cancellable_;
};

}
#endif // IWD_PEER_H_
