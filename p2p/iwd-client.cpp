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

#include <iostream>
#include <gio/gio.h>
#include <memory>

#include "iwd-client.h"

#define IWD_BUS_TYPE G_BUS_TYPE_SYSTEM
#define IWD_SERVICE "net.connman.iwd"
#define IWD_P2P_INTERFACE "net.connman.iwd.p2p.Device"
#define IWD_P2P_PEER_INTERFACE "net.connman.iwd.p2p.Peer"
#define IWD_P2P_SERVICE_MANAGER_INTERFACE "net.connman.iwd.p2p.ServiceManager"
#define IWD_P2P_WFD_INTERFACE "net.connman.iwd.p2p.Display"
#define IWD_WSC_INTERFACE "net.connman.iwd.SimpleConfiguration"

namespace P2P {

void IWDClient::check_peer_ready(std::shared_ptr<IWDPeer> peer)
{
    if (!p2p_proxy_ || g_strcmp0(g_dbus_proxy_get_object_path(p2p_proxy_), peer->get_dev_path()))
        return;
    if (!peer->is_ready())
        return;

    peers_[peer->get_path()] = peer;
    non_wfd_peers_.erase(peer->get_path());

    if (observer_)
        observer_->on_peer_added(this, peer);

    peer->added();
}

void IWDClient::check_all_peers(void)
{
    const char *path = p2p_proxy_ ? g_dbus_proxy_get_object_path(p2p_proxy_) : NULL;
    std::map<std::string, std::shared_ptr<P2P::Peer>> removed;

    for (auto it = peers_.begin(); it != peers_.end();) {
        auto peer = std::static_pointer_cast<IWDPeer>(it->second);

        if (!g_strcmp0(path, peer->get_dev_path()) && peer->is_ready()) {
            ++it;
            continue;
        }

        removed[peer->get_path()] = peer;
        peers_.erase(it++);

        if (observer_)
            observer_->on_peer_removed(this, peer);

        peer->removed();
    }

    for (auto it = non_wfd_peers_.begin(); it != non_wfd_peers_.end();) {
        auto peer = std::static_pointer_cast<IWDPeer>(it->second);

        if (g_strcmp0(path, peer->get_dev_path()) || !peer->is_ready()) {
            ++it;
            continue;
        }

        peers_[peer->get_path()] = peer;
        non_wfd_peers_.erase(it++);

        if (observer_)
            observer_->on_peer_added(this, peer);

        peer->added();
    }

    non_wfd_peers_.insert(removed.begin(), removed.end());
}

void IWDClient::check_available()
{
    if (!enabled_ || !registered_)
        return;

    if (observer_)
        observer_->on_availability_changed(this);
}

void IWDClient::check_unavailable()
{
    if (enabled_ && registered_)
        return;

    if (observer_)
        observer_->on_availability_changed(this);
}

void IWDClient::iwd_properties_changed(GDBusProxy *proxy, GVariant *changed_properties,
                                       GStrv invalidate_properties, gpointer user_data)
{
    auto client = static_cast<IWDClient *>(user_data);
    gboolean new_bool;

    if (g_variant_lookup(changed_properties, "Enabled", "b", &new_bool)) {
        if (new_bool == client->enabled_)
            return;

        client->enabled_ = new_bool;

        if (new_bool) {
            std::cout << "P2P device is now enabled" << std::endl;
            client->check_available();
        } else {
            std::cout << "P2P device is now disabled" << std::endl;
            client->check_unavailable();
        }

        return;
    }
}

void IWDClient::set_enabled_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_variant_unref(g_dbus_proxy_call_finish(proxy, res, &error));
    if (error) {
        std::cout << "set enabled error " << error->message << std::endl;
        g_clear_error(&error);
    }
}

void IWDClient::interface_added(GDBusObjectManager *object_manager, GDBusObject *object, GDBusInterface *interface, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);
    GDBusProxy *proxy;
    const char *iface_name, *path;

    proxy = G_DBUS_PROXY(interface);
    iface_name = g_dbus_proxy_get_interface_name(proxy);
    path = g_dbus_proxy_get_object_path(proxy);

    if (!g_strcmp0(iface_name, IWD_P2P_INTERFACE)) {
        if (client->p2p_proxy_)
            return;

        client->p2p_proxy_ = G_DBUS_PROXY(g_object_ref(proxy));
        g_signal_connect(client->p2p_proxy_, "g-properties-changed",
                         G_CALLBACK(iwd_properties_changed), data_ptr);
        client->check_all_peers();

        GVariant *value = g_dbus_proxy_get_cached_property(client->p2p_proxy_, "Enabled");
        if (!value) {
            std::cout << "can't get the Enabled property" << std::endl;
            return;
        }

        if (!g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
            std::cout << "Enabled property type wrong" << std::endl;
            g_variant_unref(value);
            return;
        }

        if (g_variant_get_boolean(value)) {
            client->enabled_ = true;
            client->check_available();
            g_variant_unref(value);
            return;
        }

        g_variant_unref(value);
        g_dbus_proxy_call(client->p2p_proxy_, "org.freedesktop.DBus.Properties.Set",
                          g_variant_new("(ssv)", IWD_P2P_INTERFACE,
                                        "Enabled", g_variant_new("b", true)),
                          G_DBUS_CALL_FLAGS_NONE, 2000, client->cancellable_,
                          set_enabled_cb, data_ptr);
        return;
    }

    if (    !g_strcmp0(iface_name, IWD_P2P_PEER_INTERFACE) ||
            !g_strcmp0(iface_name, IWD_P2P_WFD_INTERFACE) ||
            !g_strcmp0(iface_name, IWD_WSC_INTERFACE)) {
        if (client->peers_.find(path) != client->peers_.end())
            return;	/* Already have this peer */

        if (client->non_wfd_peers_.find(path) == client->non_wfd_peers_.end())
            client->non_wfd_peers_[path] = std::make_shared<IWDPeer>(path);

        auto peer = std::static_pointer_cast<IWDPeer>(client->non_wfd_peers_[path]);
        if (!g_strcmp0(iface_name, IWD_P2P_PEER_INTERFACE))
            peer->set_peer_proxy(proxy);
        else if (!g_strcmp0(iface_name, IWD_P2P_WFD_INTERFACE))
            peer->set_wfd_proxy(proxy);
        else
            peer->set_wsc_proxy(proxy);

        client->check_peer_ready(peer);
        return;
    }
}

void IWDClient::interface_removed(GDBusObjectManager *object_manager, GDBusObject *object, GDBusInterface *interface, gpointer user_data)
{
    auto client = static_cast<IWDClient *>(user_data);
    GDBusProxy *proxy;
    const char *iface_name, *path;

    proxy = G_DBUS_PROXY(interface);
    iface_name = g_dbus_proxy_get_interface_name(proxy);
    path = g_dbus_proxy_get_object_path(proxy);

    if (!g_strcmp0(iface_name, IWD_P2P_INTERFACE)) {
        if (proxy != client->p2p_proxy_)
            return;

        g_signal_handlers_disconnect_by_data(client->p2p_proxy_, user_data);
        g_clear_object(&client->p2p_proxy_);
        client->enabled_ = false;
        client->check_unavailable();
        client->check_all_peers();
        return;
    }

    if (    !g_strcmp0(iface_name, IWD_P2P_PEER_INTERFACE) ||
            !g_strcmp0(iface_name, IWD_P2P_WFD_INTERFACE) ||
            !g_strcmp0(iface_name, IWD_WSC_INTERFACE)) {
        std::shared_ptr<IWDPeer> peer;

        if (client->peers_.find(path) != client->peers_.end()) {
            peer = std::static_pointer_cast<IWDPeer>(client->peers_[path]);
            client->peers_.erase(path);
            client->non_wfd_peers_[path] = peer;

            if (client->observer_)
                client->observer_->on_peer_removed(client, peer);

            peer->removed();
        } else if (client->non_wfd_peers_.find(path) == client->non_wfd_peers_.end())
            peer = std::static_pointer_cast<IWDPeer>(client->non_wfd_peers_[path]);
        else
            return;

        if (!g_strcmp0(iface_name, IWD_P2P_PEER_INTERFACE))
            peer->set_peer_proxy(NULL);
        else if (!g_strcmp0(iface_name, IWD_P2P_WFD_INTERFACE))
            peer->set_wfd_proxy(NULL);
        else
            peer->set_wsc_proxy(NULL);

        /* TODO: erase once no interfaces left */
        return;
    }
}

void IWDClient::object_added(GDBusObjectManager *object_manager, GDBusObject *object, gpointer user_data)
{
    auto client = static_cast<IWDClient *>(user_data);
    GList *interfaces, *iter;

    interfaces = g_dbus_object_get_interfaces(object);

    for (iter = interfaces; iter; iter = iter->next)
        interface_added(NULL, object, G_DBUS_INTERFACE (iter->data), client);

    g_list_free_full(interfaces, g_object_unref);
}

void IWDClient::object_removed(GDBusObjectManager *object_manager, GDBusObject *object, gpointer user_data)
{
    auto client = static_cast<IWDClient *>(user_data);
    const char *path = g_dbus_object_get_object_path(object);

    if (!g_strcmp0(path, g_dbus_proxy_get_object_path(client->p2p_proxy_))) {
        g_signal_handlers_disconnect_by_data(client->p2p_proxy_, user_data);
        g_clear_object(&client->p2p_proxy_);
        client->enabled_ = false;
        client->check_unavailable();
        client->check_all_peers();
        return;
    }

    if (client->peers_.find(path) != client->peers_.end()) {
        auto peer = std::static_pointer_cast<IWDPeer>(client->peers_[path]);
        client->peers_.erase(path);

        if (client->observer_)
            client->observer_->on_peer_removed(client, peer);

        peer->removed();
        return;
    }

    if (client->non_wfd_peers_.find(path) != client->non_wfd_peers_.end())
        client->non_wfd_peers_.erase(path);
}

void IWDClient::register_display_svc_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_variant_unref(g_dbus_proxy_call_finish(proxy, res, &error));
    if (error) {
        std::cout << "register error " << error->message << std::endl;
        g_clear_error(&error);
        return;
    }

    client->registered_ = true;
    client->check_available();
}

void IWDClient::new_object_manager_cb(GObject *object, GAsyncResult *result, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);
    GError *error = NULL;
    GList *objects, *iter;
    GVariantBuilder params_builder;

    client->object_manager_ = g_dbus_object_manager_client_new_for_bus_finish(result, &error);
    if (!client->object_manager_) {
        std::cout << "g_dbus_object_manager_client_new_for_bus error: " << error->message << std::endl;
        g_clear_error(&error);
        return;
    }

    g_signal_connect(client->object_manager_, "object-added",
                     G_CALLBACK(object_added), data_ptr);
    g_signal_connect(client->object_manager_, "object-removed",
                     G_CALLBACK(object_removed), data_ptr);
    g_signal_connect(client->object_manager_, "interface-added",
                     G_CALLBACK(interface_added), data_ptr);
    g_signal_connect(client->object_manager_, "interface-removed",
                     G_CALLBACK(interface_removed), data_ptr);

    client->service_manager_proxy_ = G_DBUS_PROXY(g_dbus_object_manager_get_interface(client->object_manager_, "/net/connman/iwd", IWD_P2P_SERVICE_MANAGER_INTERFACE));

    g_variant_builder_init(&params_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&params_builder, "{sv}", "Source", g_variant_new("b", client->params_.source));
    g_variant_builder_add(&params_builder, "{sv}", "Sink", g_variant_new("b", client->params_.sink));
    g_variant_builder_add(&params_builder, "{sv}", "Port", g_variant_new("q", client->params_.session_management_control_port));

    g_dbus_proxy_call(client->service_manager_proxy_, "RegisterDisplayService",
                      g_variant_new("(a{sv})", &params_builder), G_DBUS_CALL_FLAGS_NONE, 2000,
                      client->cancellable_, register_display_svc_cb, data_ptr);

    objects = g_dbus_object_manager_get_objects(client->object_manager_);

    for (iter = objects; iter; iter = iter->next)
        client->object_added(client->object_manager_, G_DBUS_OBJECT(iter->data), data_ptr);

    g_list_free_full(objects, g_object_unref);
}

void IWDClient::iwd_appeared_cb(GDBusConnection *connection, const char *name, const char *owner, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);

    std::cout << "IWD appeared" << std::endl;

    if (client->object_manager_)
        return;

    g_dbus_object_manager_client_new_for_bus(IWD_BUS_TYPE,
                                             G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                             IWD_SERVICE, "/", NULL, NULL, NULL,
                                             client->cancellable_,
                                             new_object_manager_cb, data_ptr);
}

/* Cleanup actions common to IWD disconnecting and us disconnecting */
void IWDClient::iwd_cleanup(void)
{
    for (auto it = peers_.begin(); it != peers_.end(); ++it)
        static_cast<IWDPeer *>(it->second.get())->removed();

    peers_.clear();
    non_wfd_peers_.clear();

    g_cancellable_cancel(cancellable_);
    g_clear_object(&cancellable_);

    if (object_manager_) {
        g_signal_handlers_disconnect_by_data(object_manager_, this);
        g_clear_object(&object_manager_);
    }

    if (p2p_proxy_)
        g_clear_object(&p2p_proxy_);

    if (service_manager_proxy_)
        g_clear_object(&service_manager_proxy_);

    if (disc_timeout_)
        g_clear_handle_id(&disc_timeout_, g_source_remove);

    enabled_ = false;
    registered_ = false;
}

void IWDClient::iwd_disappeared_cb(GDBusConnection *connection, const char *name, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);
    bool was_available = client->is_available();

    std::cout << "IWD disappeared" << std::endl;

    if (client->observer_)
        for (auto it = client->peers_.begin(); it != client->peers_.end(); ++it)
            client->observer_->on_peer_removed(client, it->second);

    client->iwd_cleanup();
    client->cancellable_ = g_cancellable_new();

    if (was_available && client->observer_)
        client->observer_->on_availability_changed(client);
}

IWDClient::IWDClient(const Parameters &params, Observer *observer):
    Client(observer),
    params_(params),
    p2p_proxy_(NULL),
    service_manager_proxy_(NULL),
    object_manager_(NULL),
    enabled_(false),
    registered_(false),
    iwd_watcher_(0),
    disc_timeout_(0)
{
    cancellable_ = g_cancellable_new();
    iwd_watcher_ = g_bus_watch_name(G_BUS_TYPE_SYSTEM, IWD_SERVICE,
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    iwd_appeared_cb, iwd_disappeared_cb,
                                    this, NULL);
}

IWDClient::~IWDClient()
{
    if (registered_) {
        g_dbus_proxy_call(service_manager_proxy_, "UnregisterDisplayService",
                          NULL, G_DBUS_CALL_FLAGS_NONE, 2000, NULL, NULL, NULL);
        registered_ = false;
    }

    iwd_cleanup();

    if (iwd_watcher_ != 0) {
        g_bus_unwatch_name(iwd_watcher_);
        iwd_watcher_ = 0;
    }
}

void IWDClient::set_parameters(const Parameters &params)
{
    params_ = params;
    /* TODO: re-register? */
}

gboolean IWDClient::disc_timeout_cb(gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);

    std::cout << "discovery finished" << std::endl;
    g_dbus_proxy_call(client->p2p_proxy_, "ReleaseDiscovery",
                      NULL, G_DBUS_CALL_FLAGS_NONE, 2000,
                      client->cancellable_, NULL, NULL);
    client->disc_timeout_ = 0;
    return FALSE;
}

void IWDClient::disc_request_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    auto client = static_cast<IWDClient *>(data_ptr);
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_variant_unref(g_dbus_proxy_call_finish(proxy, res, &error));
    if (error) {
        std::cout << "RequestDiscovery error " << error->message << std::endl;
        g_clear_error(&error);
        return;
    }

    client->disc_timeout_ = g_timeout_add_seconds(20, disc_timeout_cb, data_ptr);
}

void IWDClient::scan()
{
    g_return_if_fail(is_available());

    g_dbus_proxy_call(p2p_proxy_, "RequestDiscovery",
                      NULL, G_DBUS_CALL_FLAGS_NONE, 2000,
                      cancellable_, disc_request_cb, this);
}

}
