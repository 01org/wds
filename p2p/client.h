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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <memory>

#include "peer.h"

namespace P2P {

class Client {
    public:
        class Observer {
            public:
                virtual void on_peer_added(Client *client, std::shared_ptr<P2P::Peer> peer) {}
                virtual void on_peer_removed(Client *client, std::shared_ptr<P2P::Peer> peer) {}
                virtual void on_availability_changed(Client *client) {}

            protected:
                virtual ~Observer() {}
        };

        virtual void set_parameters(const Parameters &params) = 0;
        virtual void set_observer(Observer* observer) {
            observer_ = observer;
        }

        virtual bool is_available() const = 0;
        /* TODO error / finished handling */
        virtual void scan() = 0;

    protected:
        Client(Observer *observer = NULL) : observer_(observer) {}

        Observer* observer_;
        std::map<std::string, std::shared_ptr<P2P::Peer>> peers_;
};

}
#endif // CLIENT_H_
