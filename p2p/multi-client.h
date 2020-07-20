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

#ifndef MULTI_CLIENT_H_
#define MULTI_CLIENT_H_

#include "iwd-client.h"
#include "connman-client.h"

namespace P2P {

class MultiClient : public Client, public Client::Observer {
    public:
        MultiClient(const Parameters &params, Client::Observer *observer = NULL) : Client(observer), iwd_(params, this), connman_(params, this) {}
        ~MultiClient() {}

        void set_parameters(const Parameters &params) override { iwd_.set_parameters(params); connman_.set_parameters(params); }

        bool is_available() const override { return iwd_.is_available() || connman_.is_available(); }

        void scan() override
        {
            if (iwd_.is_available())
                iwd_.scan();
            else
                connman_.scan();
        }

        void on_peer_added(Client *client, std::shared_ptr<P2P::Peer> peer) override
        {
            if (observer_)
                observer_->on_peer_added(this, peer);
        }

        void on_peer_removed(Client *client, std::shared_ptr<P2P::Peer> peer) override
        {
            if (observer_)
                observer_->on_peer_removed(this, peer);
        }

        void on_availability_changed(Client *client) override
        {
            /* No extra check needed if we assume only one service is available at any time */
            if (observer_)
                observer_->on_availability_changed(this);
        }
    protected:
        IWDClient iwd_;
        ConnmanClient connman_;
};

}
#endif // MULTI_CLIENT_H_
