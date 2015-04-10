/*
 * This file is part of Wireless Display Software for Linux OS
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

#include "cap_negotiation_state.h"

#include "libwds/common/rtsp_status_code.h"
#include "libwds/parser/audiocodecs.h"
#include "libwds/parser/clientrtpports.h"
#include "libwds/parser/getparameter.h"
#include "libwds/parser/payload.h"
#include "libwds/parser/presentationurl.h"
#include "libwds/parser/reply.h"
#include "libwds/parser/setparameter.h"
#include "libwds/parser/videoformats.h"
#include "libwds/public/media_manager.h"

namespace wds {
namespace source {

class M3Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;

 private:
  std::unique_ptr<Message> CreateMessage() override;
  bool HandleReply(Reply* reply) override;
};

class M4Handler final : public SequencedMessageSender {
 public:
  using SequencedMessageSender::SequencedMessageSender;

 private:
  std::unique_ptr<Message> CreateMessage() override;
  bool HandleReply(Reply* reply) override;
};

std::unique_ptr<Message> M3Handler::CreateMessage() {
  GetParameter* get_param = new GetParameter("rtsp://localhost/wfd1.0");
  get_param->header().set_cseq(send_cseq_++);
  // todo: get data from environment (e.g. if video / audio / hdcp required)
  // and request only needed parameters
  std::vector<std::string> props;
  props.push_back("wfd_video_formats");
  props.push_back("wfd_audio_codecs");
  props.push_back("wfd_client_rtp_ports");
  get_param->set_payload(std::unique_ptr<Payload>(new Payload(props)));
  return std::unique_ptr<Message>(get_param);
}

bool M3Handler::HandleReply(Reply* reply) {
  if (reply->response_code() != RTSP_OK)
    return false;

  SourceMediaManager* source_manager = ToSourceMediaManager(manager_);
  auto prop = reply->payload().get_property(WFD_CLIENT_RTP_PORTS);
  auto ports = static_cast<ClientRtpPorts*>(prop.get());
  if (!ports){
    WDS_ERROR("Failed to obtain RTP ports from source.");
    return false;
  }
  source_manager->SetSinkRtpPorts(ports->rtp_port_0(), ports->rtp_port_1());

  auto video_formats = static_cast<VideoFormats*>(
      reply->payload().get_property(WFD_VIDEO_FORMATS).get());
  if (!video_formats) {
    WDS_ERROR("Failed to obtain WFD_VIDEO_FORMATS property");
    return false;
  }

  auto audio_codecs = static_cast<AudioCodecs*>(
      reply->payload().get_property(WFD_AUDIO_CODECS).get());
  if (!audio_codecs) {
    WDS_ERROR("Failed to obtain WFD_AUDIO_CODECS property");
    return false;
  }

  if (!source_manager->InitOptimalVideoFormat(
      video_formats->GetNativeFormat(),
      video_formats->GetSelectableH264Formats())) {
    WDS_ERROR("Failed to initalize optimal video format.");
    return false;
  }

  if (!source_manager->InitOptimalAudioFormat(audio_codecs->audio_codecs())) {
    WDS_ERROR("Failed to initalize optimal audio format.");
    return false;
  }

  return true;
}

std::unique_ptr<Message> M4Handler::CreateMessage() {
  SetParameter* set_param = new SetParameter("rtsp://localhost/wfd1.0");
  set_param->header().set_cseq(send_cseq_++);
  SourceMediaManager* source_manager = ToSourceMediaManager(manager_);
  const auto& ports = source_manager->GetSinkRtpPorts();
  set_param->payload().add_property(
      std::shared_ptr<Property>(new ClientRtpPorts(ports.first, ports.second)));
  std::string presentation_Url_1 = "rtsp://" + sender_->GetLocalIPAddress() + "/wfd1.0/streamid=0";
  set_param->payload().add_property(
      std::shared_ptr<Property>(new PresentationUrl(presentation_Url_1, "")));
  set_param->payload().add_property(
      std::shared_ptr<VideoFormats>(new VideoFormats(
          NativeVideoFormat(),  // Should be all zeros.
          false,
          {source_manager->GetOptimalVideoFormat()})));
  set_param->payload().add_property(
      std::shared_ptr<AudioCodecs>(new AudioCodecs({source_manager->GetOptimalAudioFormat()})));

  return std::unique_ptr<Message>(set_param);
}

bool M4Handler::HandleReply(Reply* reply) {
  return (reply->response_code() == RTSP_OK);
}

CapNegotiationState::CapNegotiationState(const InitParams &init_params)
  : MessageSequenceHandler(init_params) {
  AddSequencedHandler(make_ptr(new M3Handler(init_params)));
  AddSequencedHandler(make_ptr(new M4Handler(init_params)));
}

CapNegotiationState::~CapNegotiationState() {
}

}  // namespace source
}  // namespace wds
