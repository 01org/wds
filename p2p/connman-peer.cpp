/*
 * This file is part of wysiwidi
 *
 * Copyright (C) 2014 Intel Corporation.
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

#include "connman-peer.h"

namespace P2P {

/* static C callback */
void Peer::proxy_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    auto client = reinterpret_cast<Peer*> (data_ptr);
    client->proxy_cb (res);
}

/* static C callback */
void Peer::proxy_signal_cb (GDBusProxy *proxy, const char *sender, const char *signal, GVariant *params, gpointer data_ptr)
{
	GVariant *property;
	char *name;
    auto peer = reinterpret_cast<Peer*> (data_ptr);

    if (g_strcmp0(signal, "PropertyChanged") != 0)
        return;

    g_variant_get (params, "(sv)", &name, &property);

	if (g_strcmp0(name, "State") == 0) {
	    const char *state = g_variant_get_string (property, NULL); 

		peer->state_changed (g_strcmp0 (state, "ready") == 0);
	} else if (g_strcmp0(name, "IPv4") == 0) {
		GVariantIter *ips;
		GVariant *spec_val;
		char *name;

		g_variant_get (property, "a{sv}", &ips);
        while (g_variant_iter_loop (ips, "{sv}", &name, &spec_val)) {
            if (g_strcmp0 (name, "Remote") == 0) {
				peer->remote_ip_changed (g_variant_get_string (spec_val, NULL));
			} else if (g_strcmp0 (name, "Local") == 0) {
				peer->local_ip_changed (g_variant_get_string (spec_val, NULL));
			}
			
		}
		g_variant_iter_free (ips);
	}
}

/* static C callback */
void Peer::connect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GError *error = NULL;
    GDBusProxy *proxy = G_DBUS_PROXY (object);

    g_dbus_proxy_call_finish (proxy, res, &error);
    if (error) {
        std::cout << "connect error " << error->message << std::endl;
        g_clear_error (&error);
        return;
    }

    std::cout << "* connected "<< std::endl;
}

/* static C callback */
void Peer::disconnect_cb (GObject *object, GAsyncResult *res, gpointer data_ptr)
{
    GError *error = NULL;
    GDBusProxy *proxy = G_DBUS_PROXY (object);

    g_dbus_proxy_call_finish (proxy, res, &error);
    if (error) {
        std::cout << "disconnect error " << error->message << std::endl;
        g_clear_error (&error);
        return;
    }

    std::cout << "* disconnected "<< std::endl;
}

void Peer::proxy_cb (GAsyncResult *result)
{
    GError *error = NULL;

    proxy_ = g_dbus_proxy_new_for_bus_finish(result, &error);
    if (error) {
        std::cout << "Peer proxy error "<< std::endl;
        g_clear_error (&error);
        return;
    }

    g_signal_connect (proxy_, "g-signal",
                      G_CALLBACK (Peer::proxy_signal_cb), this);

    /* TODO check the ip address in case it's up to date already */

	if (observer_)
		observer_->on_initialized(this);
}

void Peer::remote_ip_changed (const char *ip)
{
	std::string new_ip(ip);

	if (new_ip.compare (remote_host_) == 0)
		return;

	auto was_available = is_available();
	remote_host_ = new_ip;

	if (!observer_)
		return;

	if (was_available != is_available())
		observer_->on_availability_changed(this);
}

void Peer::local_ip_changed (const char *ip)
{
	std::string new_ip(ip);

	if (new_ip.compare (local_host_) == 0)
		return;

	auto was_available = is_available();
	local_host_ = new_ip;

	if (!observer_)
		return;

	if (was_available != is_available())
		observer_->on_availability_changed(this);
}

void Peer::state_changed (bool ready)
{
	if (ready_ == ready)
		return;
	
	auto was_available = is_available();
	ready_ = ready;

	if (!observer_)
		return;

	if (was_available != is_available())
		observer_->on_availability_changed(this);
}

Peer::Peer(const std::string& object_path, std::shared_ptr<P2P::InformationElement> ie):
    observer_(NULL),
    ie_(ie)
{
    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                              G_DBUS_PROXY_FLAGS_NONE,
                              NULL,
                              "net.connman",
                              object_path.c_str(),
                              "net.connman.Peer",
                              NULL,
                              Peer::proxy_cb,
                              this);
}

void Peer::connect()
{
    g_dbus_proxy_call (proxy_,
                       "Connect",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       60 * 1000, // is 1 minute too long?
                       NULL,
                       Peer::connect_cb,
                       this);
}

void Peer::disconnect()
{
    g_dbus_proxy_call (proxy_,
                       "Disconnect",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       Peer::disconnect_cb,
                       this);
}


Peer::~Peer()
{
    if (proxy_)
        g_clear_object (&proxy_);
}

}
