#include <gameplay/GameplayModule.hpp>

#include <protobuf/LogFrame.pb.h>
#include <protobuf/RadioRx.pb.h>
#include <protobuf/RadioTx.pb.h>
#include <protobuf/messages_robocup_ssl_detection.pb.h>
#include <protobuf/messages_robocup_ssl_geometry.pb.h>
#include <protobuf/messages_robocup_ssl_wrapper.pb.h>

#include <Robot.hpp>
#include "Network.hpp"
#include "NetworkRadio.hpp"
#include "PacketConvert.hpp"
#include "Radio.hpp"
#include "RadioNode.hpp"
#include "SimRadio.hpp"

RadioNode::RadioNode(Context* context, bool simulation, bool blueTeam)
    : _context(context),
      _was_blue(blueTeam) {
    _lastRadioRxTime = RJ::Time(std::chrono::microseconds(RJ::timestamp()));
    _radio = simulation
                 ? static_cast<Radio*>(new SimRadio(_context, blueTeam))
                 : static_cast<Radio*>(new NetworkRadio(NetworkRadioServerPort));
}

bool RadioNode::isOpen() { return _radio->isOpen(); }

RJ::Time RadioNode::getLastRadioRxTime() { return _lastRadioRxTime; }

Radio* RadioNode::getRadio() { return _radio; }

void RadioNode::run() {
    if (!_radio) {
        return;
    }

    // Check if the team has changed, and switch teams if necessary.
    if (_context->game_state.blueTeam != _was_blue) {
        _radio->switchTeam(_context->game_state.blueTeam);
        _was_blue = _context->game_state.blueTeam;
    }

    // Read radio reverse packets
    _radio->receive();

    while (_radio->hasReversePackets()) {
        Packet::RadioRx rx = _radio->popReversePacket();
        _context->state.logFrame->add_radio_rx()->CopyFrom(rx);

        _lastRadioRxTime = RJ::Time(std::chrono::microseconds(rx.timestamp()));

        // Store this packet in the appropriate robot
        unsigned int board = rx.robot_id();
        if (board < Num_Shells) {
            // We have to copy because the RX packet will survive past this
            // frame but LogFrame will not (the RadioRx in LogFrame will be
            // reused).
            _context->state.self[board]->setRadioRx(rx);
            _context->state.self[board]->radioRxUpdated();
        }
    }

    construct_tx_proto((*_context->state.logFrame->mutable_radio_tx()),
                       _context->robot_intents, _context->motion_setpoints);
    _radio->send(*_context->state.logFrame->mutable_radio_tx());
}
