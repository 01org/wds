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
#include "information-element.h"

#ifndef PEER_H_
#define PEER_H_

namespace P2P {

struct Parameters {
    bool source;
    bool sink;
    uint16_t session_management_control_port;
};

class Peer {
    public:
        class Observer {
            public:
                virtual void on_availability_changed(Peer *peer) {}
                virtual void on_initialized(Peer *peer) {}

            protected:
                virtual ~Observer() {}
        };

        virtual void set_observer(Observer* observer) {
            observer_ = observer;
        }

        /* TODO add error handling for these -- maybe through observer.on_error? */
        virtual void connect() = 0;
        virtual void disconnect() = 0;

        const P2P::DeviceType device_type() const { return device_type_; }
        const std::string& name() const { return name_; }
        const std::string& remote_host() const { return remote_host_; }
        const int remote_port() const { return rtsp_port_; }
        const std::string& local_host() const { return local_host_; }
        virtual bool is_available() const = 0;

    protected:
        virtual void ips_changed (const char *remote, const char *local)
        {
            if (g_strcmp0 (remote, remote_host_.c_str()) == 0 &&
                g_strcmp0 (local, local_host_.c_str()) == 0)
                return;

            auto was_available = is_available();

            if (g_strcmp0 (remote, "0.0.0.0") == 0)
                remote_host_.clear();
            else
                remote_host_ = std::string(remote);

            if (g_strcmp0 (local, "0.0.0.0") == 0)
                local_host_.clear();
            else
                local_host_ = std::string(local);

            if (!observer_)
                return;

            if (was_available != is_available())
                observer_->on_availability_changed(this);
        }

        virtual void name_changed (const char *name)
        {
            if (g_strcmp0 (name, name_.c_str()) == 0)
                return;

            name_ = std::string (name);
        }

        Observer *observer_;
        P2P::DeviceType device_type_;
        std::string name_;
        std::string remote_host_;
        std::string local_host_;
        int rtsp_port_;
};

}
#endif // PEER_H_
