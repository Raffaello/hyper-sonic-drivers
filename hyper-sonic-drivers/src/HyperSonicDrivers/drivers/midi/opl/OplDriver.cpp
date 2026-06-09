#include <algorithm>
#include <format>
#include <cassert>
#include <HyperSonicDrivers/drivers/midi/opl/OplDriver.hpp>
#include <HyperSonicDrivers/drivers/midi/opl/OplChannel.hpp>
#include <HyperSonicDrivers/hardware/opl/OplType.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::drivers::midi::opl
{
using hardware::opl::opl2_num_channels;
using hardware::opl::opl3_num_channels;

using hardware::opl::OplType;
using utils::logW;
using utils::logE;
using utils::logC;

OplDriver::OplDriver(const std::shared_ptr<devices::Opl>& opl)
    : m_opl([&opl] {
          if (opl == nullptr)
              utils::throwLogC<std::runtime_error>("OPL device is null");
          return opl->getOpl();
      }()),
      m_opl3_mode(m_opl->type == OplType::OPL3),
      m_oplNumChannels(m_opl3_mode ? opl3_num_channels : opl2_num_channels)
{
    m_oplWriter = std::make_unique<drivers::opl::OplWriter>(m_opl, m_opl3_mode);

    m_voices.resize(m_oplNumChannels);
    for (uint8_t i = 0; i < m_oplNumChannels; ++i)
    {
        m_voices[i] = std::make_unique<OplVoice>(i, m_oplWriter.get());
        m_voicesFreeIndex.push_back(i);
    }

    for (uint8_t i = 0; i < audio::midi::MIDI_MAX_CHANNELS; ++i)
    {
        m_channels[i] = std::make_unique<OplChannel>(i);
    }
}

OplDriver::~OplDriver()
{
    OplDriver::close();
}

bool OplDriver::open(const audio::mixer::eChannelGroup group,
                     const uint8_t                     volume,
                     const uint8_t                     pan)
{
    if (isOpen())
        return true;

    if (!m_oplWriter->init())
    {
        logE("Can't initialize OPL Emulator chip.");
        return false;
    }

    m_group  = group;
    m_volume = volume;
    m_pan    = pan;

    // TODO: here the acquire should be done.

    hardware::TimerCallBack cb = std::bind_front(&OplDriver::onCallback, this);
    setCallback(cb, hardware::opl::default_opl_callback_freq);
    return true;
}

void OplDriver::close()
{
    if (isOpen())
    {
        m_opl->stop();
        m_isOpen = false;
    }

    // TODO: here the release should be done.
}

void OplDriver::onCallback() noexcept
{
    // TODO: here has to call the midi player/parser to send the next events.
    // and update the internal ticks/timer to keep tracks of the deltas


    // TODO: here could process midi events,
    //       enqueued in send method
    //  if queue empty do nothing
    // must keep track of the last time it was called
    // and update is internal timer with the midi-event delta.

    // NOTE changing this onTimer will effect the current MIDDriver using a thread.
    //      but the same logic of the thread will be performed here.

    [[maybe_unused]] int i = 0;
}

void OplDriver::pause() const noexcept
{
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        const uint8_t i = *it;
        if (m_opl3_mode)
            m_oplWriter->writeValue(0xC0, i, m_voices[i]->getInstrument()->feedback);
        m_voices[i]->pause();
    }
}

void OplDriver::resume() const noexcept
{
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        const uint8_t i = *it;
        m_voices[i]->resume();
    }
}

void OplDriver::setCallback(hardware::TimerCallBack callback, int timerFrequency)
{
    auto p = std::make_shared<hardware::TimerCallBack>(callback);
    m_opl->start(p, m_group, m_volume, m_pan, timerFrequency);
    m_isOpen = true;
}

void OplDriver::noteOff(const uint8_t chan, const uint8_t note) noexcept
{
    const uint8_t sustain = m_channels[chan]->sustain;

    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end();)
    {
        if (m_voices[*it]->getChannelNum() == chan && m_voices[*it]->noteOff(note, sustain))
        {
            m_voicesFreeIndex.push_back(*it);
            it = m_voicesInUseIndex.erase(it);
        }
        else
            ++it;
    }
}

void OplDriver::noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept
{
    using audio::opl::banks::OP2Bank;

    const bool isPercussion = m_channels[chan]->isPercussion;
    int8_t     freeSlot     = getFreeOplVoiceIndex(!isPercussion);
    if (freeSlot != -1)
    {
        const uint8_t instr_index = isPercussion ? OP2Bank::getPercussionIndex(note) : m_channels[chan]->program;

        const auto instr = m_op2Bank->getInstrumentPtr(instr_index);

        utils::logD(std::format("channel#={} - midi_ch={}", chan, m_channels[chan]->channel));
        allocateVoice(freeSlot, chan, note, vol, instr, false);

        if (m_opl3_mode && OP2Bank::supportOpl3(instr))
        {
            freeSlot = getFreeOplVoiceIndex(true);
            if (freeSlot != -1)
                allocateVoice(freeSlot, chan, note, vol, instr, true);
        }
    }
    else
    {
        logC(std::format("NO FREE CHANNEL? midi-ch={} - playingVoices={} -- free={}", chan, m_voicesInUseIndex.size(), m_voicesFreeIndex.size()));
    }
}

void OplDriver::pitchBend(const uint8_t chan, const uint16_t bend) noexcept
{
    m_channels[chan]->pitch = bend;
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->getChannelNum() == chan)
            m_voices[*it]->pitchBend(bend);
    }
}

void OplDriver::sysEx(const uint8_t* msg, uint16_t length) noexcept
{
    // TODO ...
}

void OplDriver::ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept
{
    m_channels[chan]->modulation = value;

    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->getChannelNum() == chan)
            m_voices[*it]->ctrl_modulationWheel(value);
    }
}

void OplDriver::ctrl_volume(const uint8_t chan, const uint8_t value) noexcept
{
    m_channels[chan]->volume = value;
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->getChannelNum() == chan)
            m_voices[*it]->ctrl_volume(value);
    }
}

void OplDriver::ctrl_panPosition(const uint8_t chan, const uint8_t value) noexcept
{
    if (!m_opl3_mode)
        return;

    m_channels[chan]->pan = value;
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->getChannelNum() == chan)
            m_voices[*it]->ctrl_panPosition(value);
    }
}

void OplDriver::ctrl_sustain(const uint8_t chan, const uint8_t value) noexcept
{
    m_channels[chan]->sustain = value;
    if (value < opl_sustain_threshold)
        releaseSustain(chan);
}

void OplDriver::ctrl_allNotesOff() noexcept
{
    m_oplWriter->stopAll();
}

void OplDriver::releaseSustain(const uint8_t channel) const noexcept
{
    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->getChannelNum() == channel)
            m_voices[*it]->releaseSustain();
    }
}

uint8_t OplDriver::releaseVoice(const uint8_t slot, const bool forced)
{
    assert(slot >= 0 && slot < m_oplNumChannels);

    return m_voices[slot]->release(forced);
}

int OplDriver::allocateVoice(const uint8_t slot, const uint8_t channel, const uint8_t note, const uint8_t volume, const audio::opl::banks::Op2BankInstrument_t* instrument, const bool secondary)
{
    return m_voices[slot]->allocate(
        m_channels[channel].get(), note, volume, instrument, secondary);
}

int8_t OplDriver::getFreeOplVoiceIndex(const bool force)
{
    // TODO flag = 0, 1, 2, 3 ???  (using 2 bits)
    // 0 :=> search a free channel, and if not find try to kill a 2nd voice or oldest
    // 1 :=> search a free channel only.
    // 2 :=> like 0 but won't kill the oldest channel
    // 3 :=> is like 1 as it return when bit 0 is 1
    // -----
    // used only 0 and 2, so bit 1 only:
    // it can be a bool !kill_oldest_channel

    assert(m_voicesFreeIndex.size() + m_voicesInUseIndex.size() == m_oplNumChannels);

    if (!m_voicesFreeIndex.empty())
    {
        const uint8_t i = m_voicesFreeIndex.front();
        m_voicesFreeIndex.pop_front();
        m_voicesInUseIndex.push_back(i);
        return i;
    }

    for (auto it = m_voicesInUseIndex.begin(); it != m_voicesInUseIndex.end(); ++it)
    {
        if (m_voices[*it]->isSecondary())
        {
            uint8_t i = releaseVoice(*it, true);
            m_voicesInUseIndex.erase(it);
            m_voicesInUseIndex.push_back(i);
            return i;
        }
    }

    if (force)
    {
        uint8_t i = releaseVoice(m_voicesInUseIndex.front(), true);
        m_voicesInUseIndex.pop_front();
        m_voicesInUseIndex.push_back(i);
        return i;
    }

    return -1;
}
}    // namespace HyperSonicDrivers::drivers::midi::opl
