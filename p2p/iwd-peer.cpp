/* vim: ts=4 et
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
#include <stdexcept>
#include <gio/gio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "iwd-peer.h"

namespace P2P {

IWDPeer::IWDPeer(const char *path):
    params_({}),
    peer_proxy_(NULL),
    wfd_proxy_(NULL),
    wsc_proxy_(NULL),
    dev_path_(NULL),
    connected_(false)
{
    observer_ = NULL;
    path_ = g_strdup(path);
    cancellable_ = g_cancellable_new();
}

IWDPeer::~IWDPeer()
{
    g_cancellable_cancel(cancellable_);
    g_clear_object(&cancellable_);
    observer_ = NULL;
    set_peer_proxy(NULL);
    set_wfd_proxy(NULL);
    set_wsc_proxy(NULL);
    g_free(path_);
    g_free(dev_path_);
}

void IWDPeer::connect_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_dbus_proxy_call_finish(proxy, res, &error);
    if (error) {
        std::cout << "Connect() error " << error->message << std::endl;
        g_clear_error(&error);
    } else
        std::cout << "Connect() success" << std::endl;
}

void IWDPeer::cancel_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_dbus_proxy_call_finish(proxy, res, &error);
    if (error) {
        std::cout << "Cancel() error " << error->message << std::endl;
        g_clear_error(&error);
    } else
        std::cout << "Cancel() success" << std::endl;
}

void IWDPeer::disconnect_cb(GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GDBusProxy *proxy = G_DBUS_PROXY(object);
    GError *error = NULL;

    g_dbus_proxy_call_finish(proxy, res, &error);
    if (error) {
        std::cout << "Disconnect() error " << error->message << std::endl;
        g_clear_error(&error);
    } else
        std::cout << "Disconnect() success" << std::endl;
}

void IWDPeer::connect()
{
    g_dbus_proxy_call(wsc_proxy_, "PushButton", NULL, G_DBUS_CALL_FLAGS_NONE,
                      120000, cancellable_, connect_cb, this);
}

void IWDPeer::disconnect()
{
    if (!connected_)
        g_dbus_proxy_call(wsc_proxy_, "Cancel", NULL, G_DBUS_CALL_FLAGS_NONE,
                          5000, cancellable_, cancel_cb, this);

    g_dbus_proxy_call(peer_proxy_, "Disconnect", NULL, G_DBUS_CALL_FLAGS_NONE,
                      5000, cancellable_, disconnect_cb, this);
}

const char *IWDPeer::get_str_property(GVariant *value, const GVariantType *type, const char *name)
{
    if (!value || !g_variant_is_of_type(value, type)) {
        std::cout << "property " << name << " type wrong" << std::endl;
        return NULL;
    }

    return g_variant_get_string(value, NULL);
}

bool IWDPeer::get_bool_property(GVariant *value, const char *name, bool *out)
{
    if (!value || !g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
        std::cout << "property " << name << " type wrong" << std::endl;
        return false;
    }

    *out = g_variant_get_boolean(value);
    return true;
}

bool IWDPeer::get_uint16_property(GVariant *value, const char *name, int *out)
{
    if (!value || !g_variant_is_of_type(value, G_VARIANT_TYPE_UINT16)) {
        std::cout << "property " << name << " type wrong" << std::endl;
        return false;
    }

    *out = g_variant_get_uint16(value);
    return true;
}

void IWDPeer::local_host_update(const char *ifname)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        std::cout << "can't get local IPs: " << strerror(errno) << std::endl;
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        void *addr;
        char str[INET_ADDRSTRLEN];

        if (strcmp(ifa->ifa_name, ifname))
            continue;
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;

        addr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
        inet_ntop(AF_INET, addr, str, INET_ADDRSTRLEN);
        local_host_ = std::string(str);
        return;
    }

    std::cout << "can't find local IP" << std::endl;
}

void IWDPeer::dev_type_update(void)
{
    if (params_.source)
        device_type_ = params_.sink ? DUAL_ROLE : SOURCE;
    else if (params_.sink)
        device_type_ = PRIMARY_SINK;
    else
        std::cout << "Device is neither a source nor a sink" << std::endl;
}

void IWDPeer::check_available(void)
{
    if (observer_ && connected_ && !local_host_.empty() && !remote_host_.empty())
        observer_->on_availability_changed(this);
}

void IWDPeer::peer_property_update(const char *name, GVariant *value)
{
    if (!strcmp(name, "Name")) {
        const char *str_val = get_str_property(value, G_VARIANT_TYPE_STRING, "Name");
        if (!str_val)
            return;

        name_changed(str_val);
    } else if (!strcmp(name, "Device")) {
        const char *str_val = get_str_property(value, G_VARIANT_TYPE_OBJECT_PATH, "Device");
        if (!str_val)
            return;

        g_free(dev_path_);
        dev_path_ = g_strdup(str_val);
    } else if (!strcmp(name, "Connected")) {
        bool was_available = is_available();
        bool bool_val;

        if (!get_bool_property(value, "Connected", &bool_val))
            return;

        connected_ = bool_val;

        if (was_available && !connected_) {
            local_host_.clear();
            remote_host_.clear();

            if (observer_)
                observer_->on_availability_changed(this);
        } else if (!was_available && connected_)
            check_available();
    } else if (!strcmp(name, "ConnectedInterface")) {
        bool was_available = is_available();
        const char *str_val = get_str_property(value, G_VARIANT_TYPE_STRING, "ConnectedInterface");
        if (!str_val)
            return;

        local_host_update(str_val);

        if (!was_available)
            check_available();
    } else if (!strcmp(name, "ConnectedIP")) {
        bool was_available = is_available();
        const char *str_val = get_str_property(value, G_VARIANT_TYPE_STRING, "ConnectedIp");
        if (!str_val)
            return;

        remote_host_ = std::string(str_val);

        if (!was_available)
            check_available();
    }
}

void IWDPeer::wfd_property_update(const char *name, GVariant *value)
{
    if (!strcmp(name, "Source")) {
        if (!get_bool_property(value, "Source", &params_.source))
            return;

        dev_type_update();
    } else if (!strcmp(name, "Sink")) {
        if (!get_bool_property(value, "Sink", &params_.sink))
            return;

        dev_type_update();
    } else if (!strcmp(name, "Port"))
        get_uint16_property(value, "Port", &rtsp_port_);
}

void IWDPeer::peer_properties_changed_cb(GDBusProxy *proxy, GVariant *changed_properties,
                                         GStrv invalidate_properties, gpointer user_data)
{
    auto peer = static_cast<IWDPeer *>(user_data);
    GVariantIter *iter;
    const char *key;
    GVariant *value;

    g_variant_get(changed_properties, "a{sv}", &iter);
    while (g_variant_iter_loop(iter, "{&sv}", &key, &value))
        peer->peer_property_update(key, value);
    g_variant_iter_free(iter);
}

void IWDPeer::set_peer_proxy(GDBusProxy *proxy)
{
    if (peer_proxy_)
        g_signal_handlers_disconnect_by_data(peer_proxy_, this);

    g_set_object(&peer_proxy_, proxy);

    if (peer_proxy_) {
        char **props = g_dbus_proxy_get_cached_property_names(peer_proxy_);

        g_signal_connect(peer_proxy_, "g-properties-changed",
                         G_CALLBACK(peer_properties_changed_cb), this);

        for (char **name = props; *name; name++) {
            GVariant *value = g_dbus_proxy_get_cached_property(peer_proxy_, *name);
            peer_property_update(*name, value);
	        g_variant_unref(value);
        }
        g_strfreev(props);

        if (name_.empty()) {
            std::cout << "peer name not found" << std::endl;
            g_clear_object(&peer_proxy_);
        } else if (!dev_path_) {
            std::cout << "peer device path not found" << std::endl;
            g_clear_object(&peer_proxy_);
        }
    }
}

void IWDPeer::wfd_properties_changed_cb(GDBusProxy *proxy, GVariant *changed_properties,
                                        GStrv invalidate_properties, gpointer user_data)
{
    auto peer = static_cast<IWDPeer *>(user_data);
    GVariantIter *iter;
    const char *key;
    GVariant *value;

    g_variant_get(changed_properties, "a{sv}", &iter);
    while (g_variant_iter_loop(iter, "{&sv}", &key, &value))
        peer->wfd_property_update(key, value);
    g_variant_iter_free(iter);
}

void IWDPeer::set_wfd_proxy(GDBusProxy *proxy)
{
    if (wfd_proxy_)
        g_signal_handlers_disconnect_by_data(wfd_proxy_, this);

    g_set_object(&wfd_proxy_, proxy);

    if (wfd_proxy_) {
        char **props = g_dbus_proxy_get_cached_property_names(wfd_proxy_);

        g_signal_connect(wfd_proxy_, "g-properties-changed",
                         G_CALLBACK(wfd_properties_changed_cb), this);

        for (char **name = props; *name; name++) {
            GVariant *value = g_dbus_proxy_get_cached_property(wfd_proxy_, *name);
            wfd_property_update(*name, value);
	        g_variant_unref(value);
        }
        g_strfreev(props);

        if (!params_.source && !params_.sink) {
            std::cout << "couldn't determine device type" << std::endl;
            g_clear_object(&wfd_proxy_);
        }
    }
}

void IWDPeer::added(void)
{
    if (observer_)
        observer_->on_initialized(this);
}

void IWDPeer::removed(void)
{
    observer_ = NULL;
}

}
