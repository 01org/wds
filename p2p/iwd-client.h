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

#ifndef IWD_CLIENT_H_
#define IWD_CLIENT_H_

#include <memory>
#include <gio/gio.h>

#include "client.h"
#include "iwd-peer.h"

namespace P2P {

class IWDClient : public Client {
    public:
        IWDClient(const Parameters &params, Observer *observer = NULL);
        virtual ~IWDClient();

        void set_parameters(const Parameters &params) override;

        bool is_available() const override { return enabled_ && registered_; }

        void scan() override;

    protected:
        void check_peer_ready(std::shared_ptr<IWDPeer> peer);
        void check_all_peers(void);
        void check_available(void);
        void check_unavailable(void);
        void iwd_cleanup(void);

        static void set_enabled_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void register_display_svc_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);
        static void interface_added(GDBusObjectManager *object_manager, GDBusObject *object, GDBusInterface *interface, gpointer data_ptr);
        static void interface_removed(GDBusObjectManager *object_manager, GDBusObject *object, GDBusInterface *interface, gpointer user_data);
        static void object_added(GDBusObjectManager *object_manager, GDBusObject *object, gpointer user_data);
        static void object_removed(GDBusObjectManager *object_manager, GDBusObject *object, gpointer user_data);
        static void new_object_manager_cb(GObject *object, GAsyncResult *result, gpointer data_ptr);
        static void iwd_properties_changed(GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidate_properties, gpointer user_data);
        static void iwd_appeared_cb(GDBusConnection *connection, const char *name, const char *owner, gpointer data_ptr);
        static void iwd_disappeared_cb(GDBusConnection *connection, const char *name, gpointer data_ptr);
        static gboolean disc_timeout_cb(gpointer data_ptr);
        static void disc_request_cb(GObject *object, GAsyncResult *res, gpointer data_ptr);

        Parameters params_;
        std::map<std::string, std::shared_ptr<P2P::Peer>> non_wfd_peers_;
        GDBusProxy *p2p_proxy_;
        GDBusProxy *service_manager_proxy_;
        GDBusObjectManager *object_manager_;
        bool enabled_;
        bool registered_;
        uint iwd_watcher_;
        GCancellable *cancellable_;
        guint disc_timeout_;
};

}
#endif // IWD_CLIENT_H_
