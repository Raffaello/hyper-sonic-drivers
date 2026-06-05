#include <format>
#include <algorithm>
#include <cassert>
#include <HyperSonicDrivers/audio/midi/types.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/MidiDriver_ADLIB.hpp>
#include <HyperSonicDrivers/drivers/midi/scummvm/AdLibChannel.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::drivers::midi::scummvm
{
using hardware::opl::OplType;
using utils::logD;
using utils::logW;

constexpr const uint8_t g_operator1Offsets[9] = {
    0, 1, 2, 8, 9, 10, 16, 17, 18};

constexpr const uint8_t g_operator2Offsets[9] = {
    3, 4, 5, 11, 12, 13, 19, 20, 21};

constexpr const AdLibSetParams g_setParamTable[] = {
    // reg,  offs, val1, val2 ?
    {0x40, 0, 63,  63}, // level
    {0xE0, 2, 0,   0 }, // unused
    {0x40, 6, 192, 0 }, // level key scaling
    {0x20, 0, 15,  0 }, // modulator frequency multiple
    {0x60, 4, 240, 15}, // attack rate
    {0x60, 0, 15,  15}, // decay rate
    {0x80, 4, 240, 15}, // sustain level
    {0x80, 0, 15,  15}, // release rate
    {0xE0, 0, 3,   0 }, // waveformSelect select
    {0x20, 7, 128, 0 }, // amp mod
    {0x20, 6, 64,  0 }, // vib
    {0x20, 5, 32,  0 }, // eg typ
    {0x20, 4, 16,  0 }, // ksr
    {0xC0, 0, 1,   0 }, // decay alg
    {0xC0, 1, 14,  0 }  // feedback
};

constexpr const uint8_t g_paramTable1[16] = {
    29, 28, 27, 0, 3, 4, 7, 8, 13, 16, 17, 20, 21, 30, 31, 0};

constexpr const uint16_t g_maxValTable[16] = {
    0x2FF, 0x1F, 0x7, 0x3F, 0x0F, 0x0F, 0x0F, 0x3, 0x3F, 0x0F, 0x0F, 0x0F, 0x3, 0x3E, 0x1F, 0};

constexpr const uint16_t g_numStepsTable[] = {
    1, 2, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 18, 21, 24, 30, 36, 50, 64, 82, 100, 136, 160, 192, 240, 276, 340, 460, 600, 860, 1200, 1600};

constexpr const uint8_t g_noteFrequencies[] = {
    90, 91, 92, 92, 93, 94, 94, 95, 96, 96, 97, 98, 98, 99, 100, 101, 101, 102, 103, 104, 104, 105, 106, 107, 107, 108, 109, 110, 111, 111, 112, 113, 114, 115, 115, 116, 117, 118, 119, 120, 121, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 157, 158, 159, 160, 161, 162, 163, 165, 166, 167, 168, 169, 171, 172, 173, 174, 176, 177, 178, 180, 181, 182, 184, 185, 186, 188, 189, 190, 192, 193, 194, 196, 197, 199, 200, 202, 203, 205, 206, 208, 209, 211, 212, 214, 215, 217, 218, 220, 222, 223, 225, 226, 228, 230, 231, 233, 235, 236, 238, 240, 242, 243, 245, 247, 249, 251, 252, 254};

constexpr AdLibChannel* toAdlibPart(IMidiChannel* mc)
{
    return dynamic_cast<AdLibChannel*>(mc);
}

static AdLibChannel* toAdlibPart(const std::unique_ptr<IMidiChannel>& ap)
{
    return toAdlibPart(ap.get());
}

MidiDriver_ADLIB::MidiDriver_ADLIB(const std::shared_ptr<devices::Opl>& opl) : m_opl([&opl] {
            if (opl == nullptr)
                utils::throwLogC<std::runtime_error>("Device is null ptr");

            return opl->getOpl(); }()),
                                                                               m_opl3Mode(m_opl->type != OplType::OPL2)
{
    using audio::midi::MIDI_MAX_CHANNELS;
    using audio::midi::MIDI_PERCUSSION_CHANNEL;

    std::ranges::fill(_curNotTable, 0);
    for (int i = 0; i < MIDI_PERCUSSION_CHANNEL; ++i)
        m_channels[i] = std::make_unique<AdLibChannel>(static_cast<uint8_t>(i));

    m_channels[MIDI_PERCUSSION_CHANNEL] = std::make_unique<AdLibPercussionChannel>();
    for (int i = MIDI_PERCUSSION_CHANNEL + 1; i < MIDI_MAX_CHANNELS; i++)
        m_channels[i] = std::make_unique<AdLibChannel>(static_cast<uint8_t>(i));

    std::ranges::fill(_channelTable2, 0);
    m_percussion = dynamic_cast<AdLibPercussionChannel*>(m_channels[MIDI_PERCUSSION_CHANNEL].get());
}

MidiDriver_ADLIB::~MidiDriver_ADLIB()
{
    if (m_isOpen)
        MidiDriver_ADLIB::close();
}

bool MidiDriver_ADLIB::open(
    const audio::mixer::eChannelGroup group,
    const uint8_t                     volume,
    const uint8_t                     pan)
{
    if (m_isOpen)
        return true;

    m_isOpen = true;

    for (size_t i = 0; i != m_voices.size(); i++)
    {
        AdLibVoice* voice = &m_voices[i];
        voice->slot       = static_cast<uint8_t>(i);
        voice->_s11a.s10  = &voice->_s10b;
        voice->_s11b.s10  = &voice->_s10a;
    }

    m_opl->init();

    _regCache = (uint8_t*) calloc(256, 1);

    adlibWrite(8, 0x40);
    adlibWrite(0xBD, 0x00);
    if (!m_opl3Mode)
    {
        adlibWrite(1, 0x20);
        createLookupTable();
    }
    else
    {
        _regCacheSecondary = (uint8_t*) calloc(256, 1);
        adlibWriteSecondary(5, 1);
    }

    hardware::TimerCallBack cb = std::bind_front(&MidiDriver_ADLIB::onCallback, this);
    auto                    p  = std::make_shared<hardware::TimerCallBack>(cb);
    m_opl->start(p, group, volume, pan);

    return true;
}

void MidiDriver_ADLIB::close()
{
    if (!m_isOpen)
        return;
    m_isOpen = false;

    // Stop the OPL timer
    m_opl->stop();

    for (auto& v : m_voices)
    {
        if (v.getChannel() != nullptr)
            mcOff(&v);
    }

    free(_regCache);
    free(_regCacheSecondary);
}

uint32_t MidiDriver_ADLIB::property(int prop, uint32_t param)
{
    switch (prop)
    {
    case PROP_OLD_ADLIB:    // Older games used a different operator volume algorithm
        _scummSmallHeader = (param > 0);
        if (_scummSmallHeader)
        {
            _timerIncrease  = 473;
            _timerThreshold = 1000;
        }
        else
        {
            _timerIncrease  = 0xD69;
            _timerThreshold = 0x411B;
        }
        return 1;

    case PROP_SCUMM_OPL3:    // Sam&Max OPL3 support.
        m_opl3Mode = (param > 0);
        return 1;

    default:
        break;
    }

    return 0;
}

void MidiDriver_ADLIB::setPitchBendRange(uint8_t channel, unsigned int range)
{
    // Not supported in OPL3 mode.
    if (m_opl3Mode)
        return;

    AdLibChannel* part    = toAdlibPart(m_channels[channel]);
    part->pitchBendFactor = range;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        adlibNoteOn(voice->slot, voice->getNote() /* + part->_transposeEff*/, (part->pitch * part->pitchBendFactor >> 6) + part->detuneEff);
    }
}

void MidiDriver_ADLIB::sysEx_customInstrument(uint8_t channel, uint32_t type, const uint8_t* instr)
{
    if (m_opl3Mode)
    {
        logW("Used in OPL3 mode, not supported");
        return;
    }

    auto part = getChannel(channel);

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

    if (part->isPercussion)
    {
        if (type == static_cast<uint32_t>('ADLP'))
            m_percussion->setCustomInstr(instr);
    }
    else
    {
        if (type == static_cast<uint32_t>('ADL '))
            part->setCustomInstr(instr);
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

void MidiDriver_ADLIB::adlibWrite(uint8_t reg, uint8_t value)
{
    if (_regCache[reg] == value)
        return;

    _regCache[reg] = value;
    m_opl->writeReg(reg, value);
}

void MidiDriver_ADLIB::adlibWriteSecondary(uint8_t reg, uint8_t value)
{
    assert(m_opl3Mode);

    if (_regCacheSecondary[reg] == value)
    {
        return;
    }
    _regCacheSecondary[reg] = value;

    m_opl->writeReg(reg | 0x100, value);
}

void MidiDriver_ADLIB::onCallback() noexcept
{
    // TODO: here has to call the midi parser/player to send the next event(s)
    // if (_adlibTimerProc)
    //    (*_adlibTimerProc)(_adlibTimerParam);

    _timerCounter += _timerIncrease;
    while (_timerCounter >= _timerThreshold)
    {
        _timerCounter -= _timerThreshold;
        // Sam&Max's OPL3 driver does not have any timer handling like this.
        if (m_opl3Mode)
            continue;

        for (auto& voice : m_voices)
        {
            if (voice.isFree())
                continue;

            if (voice.duration && (voice.duration -= 0x11) <= 0)
            {
                mcOff(&voice);
                return;
            }

            if (voice._s10a.active)
            {
                mcIncStuff(&voice, &voice._s10a, &voice._s11a);
            }
            if (voice._s10b.active)
            {
                mcIncStuff(&voice, &voice._s10b, &voice._s11b);
            }
        }
    }
}

void MidiDriver_ADLIB::noteOff(const uint8_t chan, const uint8_t note) noexcept
{
    auto    part  = getChannel(chan);
    uint8_t note_ = note;
    if (part->isPercussion)
    {
        note_ = m_percussion->getNote(note_);
    }

    partKeyOff(part, note_);
    logD(std::format("noteOff {} {}", chan, note_));
}

void MidiDriver_ADLIB::noteOn(const uint8_t chan, const uint8_t note, const uint8_t vol) noexcept
{
    auto part = getChannel(chan);

    if (part->isPercussion)
    {
        const AdLibInstrument* inst = nullptr;
        const AdLibInstrument* sec  = nullptr;

        // The custom instruments have priority over the default mapping
        // We do not support custom instruments in OPL3 mode though.
        if (!m_opl3Mode)
        {
            inst = m_percussion->getCustomInstrument(note);
        }

        if (inst == nullptr)
        {
            // Use the default GM to FM mapping as a fallback
            const uint8_t key = g_gmPercussionInstrumentMap[note];
            if (key != 0xFF)
            {
                if (!m_opl3Mode)
                {
                    inst = &g_gmPercussionInstruments[key];
                }
                else
                {
                    inst = &g_gmPercussionInstrumentsOPL3[key][0];
                    sec  = &g_gmPercussionInstrumentsOPL3[key][1];
                }
            }
        }

        if (!inst)
        {
            logD(std::format("No instrument FM definition for GM percussion key {:d}", note));
            return;
        }

        partKeyOn(part, inst, note, vol, sec, part->pan);
    }
    else
    {
        partKeyOn(part, part->getInstr(), note, vol, part->getInstrSecondary(), part->pan);
    }

    logD(std::format("noteOn {} {}", note, vol));
}

void MidiDriver_ADLIB::controller(const uint8_t chan, const audio::midi::MIDI_EVENT_CONTROLLER_TYPES ctrl_type, uint8_t value) noexcept
{
    // TODO: mostly duplicate code, it should bring into the parent class i guess...
    switch (ctrl_type)
    {
        using enum audio::midi::MIDI_EVENT_CONTROLLER_TYPES;
    case BANK_SELECT_MSB:
    case BANK_SELECT_LSB:
        // Bank select. Not supported
        break;
    case MODULATION_WHEEL:
        ctrl_modulationWheel(chan, value);
        break;
    case CHANNEL_VOLUME:
        ctrl_volume(chan, value);
        break;
    case PAN:
        ctrl_panPosition(chan, value);
        break;
    case GENERAL_PURPOSE_CONTROLLER_1:
        ctrl_pitchBendFactor(chan, value);
        break;
    case GENERAL_PURPOSE_CONTROLLER_2:
        ctrl_detune(chan, value);
        break;
    case GENERAL_PURPOSE_CONTROLLER_3:
        ctrl_priority(chan, value);
        break;
    case SUSTAIN:
        ctrl_sustain(chan, value);
        break;
    case REVERB:
        // Effects level. Not supported.
        ctrl_reverb(chan, value);
        break;
    case CHORUS:
        // Chorus level. Not supported.
        ctrl_chorus(chan, value);
        break;
    case RESET_ALL_CONTROLLERS:
        // reset all controllers
        ctrl_modulationWheel(chan, 0);
        ctrl_pitchBendFactor(chan, 0);
        ctrl_detune(chan, 0);
        ctrl_sustain(chan, 0);
        break;
    case ALL_NOTES_OFF:
        ctrl_allNotesOff();
        break;
    default:
        logW(std::format("Unknown control change message {:d} {:d}", static_cast<int>(ctrl_type), value));
    }
}

void MidiDriver_ADLIB::programChange(const uint8_t chan, const uint8_t program) noexcept
{
    if (program > 127)
        return;

    auto part     = getChannel(chan);
    part->program = program;
    if (!part->isPercussion)
        part->setInstr(m_opl3Mode);
}

void MidiDriver_ADLIB::pitchBend(const uint8_t chan, const uint16_t bend) noexcept
{
    auto part = getChannel(chan);

    part->pitch = bend;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        if (!m_opl3Mode)
        {
            adlibNoteOn(voice->slot, voice->getNote() /* + _transposeEff*/, (part->pitch * part->pitchBendFactor >> 6) + part->detuneEff);
        }
        else
        {
            adlibNoteOn(voice->slot, voice->getNote(), part->pitch >> 1);
        }
    }
}

void MidiDriver_ADLIB::sysEx(const uint8_t* msg, uint16_t length) noexcept
{
}

void MidiDriver_ADLIB::ctrl_modulationWheel(const uint8_t chan, const uint8_t value) noexcept
{
    auto part = getChannel(chan);

    if (part->isPercussion)
        return;

    part->modulation = value;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        if (voice->_s10a.active && voice->_s11a.flag0x40)
            voice->_s10a.modWheel = part->modulation >> 2;
        if (voice->_s10b.active && voice->_s11b.flag0x40)
            voice->_s10b.modWheel = part->modulation >> 2;
    }
}

void MidiDriver_ADLIB::ctrl_volume(const uint8_t chan, const uint8_t value) noexcept
{
    auto part = getChannel(chan);

    part->volume = value;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        if (!m_opl3Mode)
        {
            adlibSetParam(voice->slot, 0, g_volumeTable[g_volumeLookupTable[voice->vol2][part->volume >> 2]]);
            if (voice->twoChan)
            {
                adlibSetParam(voice->slot, 13, g_volumeTable[g_volumeLookupTable[voice->vol1][part->volume >> 2]]);
            }
        }
        else
        {
            adlibSetParam(voice->slot, 0, g_volumeTable[((voice->vol2 + 1) * part->volume) >> 7], true);
            adlibSetParam(voice->slot, 0, g_volumeTable[((voice->secVol2 + 1) * part->volume) >> 7], false);
            if (voice->twoChan)
            {
                adlibSetParam(voice->slot, 13, g_volumeTable[((voice->vol1 + 1) * part->volume) >> 7], true);
            }
            if (voice->secTwoChan)
            {
                adlibSetParam(voice->slot, 13, g_volumeTable[((voice->secVol1 + 1) * part->volume) >> 7], false);
            }
        }
    }
}

void MidiDriver_ADLIB::ctrl_panPosition(const uint8_t chan, uint8_t value) noexcept
{
    auto part = getChannel(chan);
    part->pan = value;
}

void MidiDriver_ADLIB::ctrl_pitchBendFactor(const uint8_t chan, const uint8_t value) noexcept
{
    auto part = getChannel(chan);

    if (part->isPercussion)
        return;

    if (m_opl3Mode)
        return;

    part->pitchBendFactor = value;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        adlibNoteOn(voice->slot, voice->getNote() /* + _transposeEff*/, (part->pitch * part->pitchBendFactor >> 6) + part->detuneEff);
    }
}

void MidiDriver_ADLIB::ctrl_detune(const uint8_t chan, const uint8_t value) noexcept
{
    auto part = getChannel(chan);

    if (part->isPercussion)
        return;

    // Sam&Max's OPL3 driver uses this for a completely different purpose. It
    // is related to voice allocation. We ignore this for now.
    // TODO: We probably need to look how the interpreter side of Sam&Max's
    // iMuse version handles all this too. Implementing the driver side here
    // would be not that hard.
    if (m_opl3Mode)
    {
        //_maxNotes = value;
        return;
    }

    part->detuneEff = value;
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        adlibNoteOn(voice->slot, voice->getNote() /* + _transposeEff*/, (part->pitch * part->pitchBendFactor >> 6) + part->detuneEff);
    }
}

void MidiDriver_ADLIB::ctrl_priority(const uint8_t chan, const uint8_t value) noexcept
{
    auto part = getChannel(chan);
    if (part->isPercussion)
        return;

    part->priority = value;
}

void MidiDriver_ADLIB::ctrl_sustain(const uint8_t chan, uint8_t value) noexcept
{
    auto part = getChannel(chan);

    if (part->isPercussion)
        return;

    part->sustain = value;
    if (value != 0)
    {
        for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
        {
            if (voice->isSustain())
                mcOff(voice);
        }
    }
}

void MidiDriver_ADLIB::ctrl_reverb(const uint8_t chan, uint8_t value) noexcept
{
    // Not Supported
}

void MidiDriver_ADLIB::ctrl_chorus(const uint8_t chan, uint8_t value) noexcept
{
    // Not Supported
}

void MidiDriver_ADLIB::ctrl_allNotesOff() noexcept
{
    for (AdLibVoice v : m_voices)
        mcOff(&v);
}

// void MidiDriver_ADLIB::setTimerCallback(void* timerParam, /*Common::TimerManager::TimerProc*/ void* timerProc) {
//     _adlibTimerProc = timerProc;
//     _adlibTimerParam = timerParam;
// }

void MidiDriver_ADLIB::mcOff(AdLibVoice* voice)
{
    AdLibVoice* tmp;

    adlibKeyOff(voice->slot);

    tmp = voice->prev;

    if (voice->next)
        voice->next->prev = tmp;
    if (tmp)
        tmp->next = voice->next;
    else
        toAdlibPart(voice->getChannel())->voice = voice->next;

    voice->setChannel(nullptr);
    voice->setFree(true);
}

void MidiDriver_ADLIB::mcIncStuff(AdLibVoice* voice, Struct10* s10, Struct11* s11)
{
    const AdLibChannel* part = toAdlibPart(voice->getChannel());
    const uint8_t       code = struct10OnTimer(s10, s11);

    if (code & 1)
    {
        switch (s11->param)
        {
        case 0:
            voice->vol2 = s10->startValue + s11->modifyVal;
            if (!_scummSmallHeader)
            {
                adlibSetParam(voice->slot, 0, g_volumeTable[g_volumeLookupTable[voice->vol2][part->volume >> 2]]);
            }
            else
            {
                adlibSetParam(voice->slot, 0, voice->vol2);
            }
            break;
        case 13:
            voice->vol1 = s10->startValue + s11->modifyVal;
            if (voice->twoChan && !_scummSmallHeader)
            {
                adlibSetParam(voice->slot, 13, g_volumeTable[g_volumeLookupTable[voice->vol1][part->volume >> 2]]);
            }
            else
            {
                adlibSetParam(voice->slot, 13, voice->vol1);
            }
            break;
        case 30:
            s11->s10->modWheel = (char) s11->modifyVal;
            break;
        case 31:
            s11->s10->unk3 = (char) s11->modifyVal;
            break;
        default:
            adlibSetParam(voice->slot, s11->param, s10->startValue + s11->modifyVal);
            break;
        }
    }

    if (code & 2 && s11->flag0x10)
        adlibKeyOnOff(voice->slot);
}

void MidiDriver_ADLIB::adlibKeyOff(int chan)
{
    uint8_t reg = chan + 0xB0;
    adlibWrite(reg, adlibGetRegValue(reg) & ~0x20);
    if (m_opl3Mode)
    {
        adlibWriteSecondary(reg, adlibGetRegValueSecondary(reg) & ~0x20);
    }
}

uint8_t MidiDriver_ADLIB::struct10OnTimer(Struct10* s10, Struct11* s11)
{
    uint8_t result = 0;
    int     i;

    if (s10->count && (s10->count -= 17) <= 0)
    {
        s10->active = 0;
        return 0;
    }

    i                    = s10->curVal + s10->speedHi;
    s10->speedLoCounter += s10->speedLo;
    if (s10->speedLoCounter >= s10->speedLoMax)
    {
        s10->speedLoCounter -= s10->speedLoMax;
        i                   += s10->direction;
    }
    if (s10->curVal != i || s10->modWheel != s10->modWheelLast)
    {
        s10->curVal       = i;
        s10->modWheelLast = s10->modWheel;
        i                 = lookupVolume(i, s10->modWheelLast);
        if (i != s11->modifyVal)
        {
            s11->modifyVal = i;
            result         = 1;
        }
    }

    if (!--s10->numSteps)
    {
        s10->active++;
        if (s10->active > 4)
        {
            if (s10->loop)
            {
                s10->active  = 1;
                result      |= 2;
                struct10Setup(s10);
            }
            else
            {
                s10->active = 0;
            }
        }
        else
        {
            struct10Setup(s10);
        }
    }

    return result;
}

void MidiDriver_ADLIB::adlibSetParam(int channel, uint8_t param, int value, bool primary)
{
    const AdLibSetParams* as;
    uint8_t               reg;

    assert(channel >= 0 && channel < 9);
    assert(!m_opl3Mode || (param == 0 || param == 13));

    if (param <= 12)
    {
        reg = g_operator2Offsets[channel];
    }
    else if (param <= 25)
    {
        param -= 13;
        reg    = g_operator1Offsets[channel];
    }
    else if (param <= 27)
    {
        param -= 13;
        reg    = channel;
    }
    else if (param == 28 || param == 29)
    {
        if (param == 28)
            value -= 15;
        else
            value -= 383;
        value                   *= 16;
        _channelTable2[channel]  = value;
        adlibPlayNote(channel, _curNotTable[channel] + value);
        return;
    }
    else
    {
        return;
    }

    as = &g_setParamTable[param];
    if (as->inversion)
        value = as->inversion - value;
    reg += as->registerBase;
    if (primary)
    {
        adlibWrite(reg, (adlibGetRegValue(reg) & ~as->mask) | (((uint8_t) value) << as->shift));
    }
    else
    {
        adlibWriteSecondary(reg, (adlibGetRegValueSecondary(reg) & ~as->mask) | (((uint8_t) value) << as->shift));
    }
}

void MidiDriver_ADLIB::adlibKeyOnOff(int channel)
{
    assert(!m_opl3Mode);

    uint8_t val;
    uint8_t reg = channel + 0xB0;
    assert(channel >= 0 && channel < 9);

    val = adlibGetRegValue(reg);
    adlibWrite(reg, val & ~0x20);
    adlibWrite(reg, val | 0x20);
}

void MidiDriver_ADLIB::struct10Setup(Struct10* s10)
{
    int     b, c, d, e, f, g, h;
    uint8_t t;

    b = s10->unk3;
    f = s10->active - 1;

    t = s10->tableA[f];
    e = g_numStepsTable[g_volumeLookupTable[t & 64][b]];
    if (t & 0x80)
    {
        e = randomNr(e);
    }
    if (e == 0)
        e++;

    s10->numSteps = s10->speedLoMax = e;

    if (f != 2)
    {
        c = s10->maxValue;
        g = s10->startValue;
        t = s10->tableB[f];
        d = lookupVolume(c, (t & 64) - 31);
        if (t & 0x80)
        {
            d = randomNr(d);
        }
        if (d + g > c)
        {
            h = c - g;
        }
        else
        {
            h = d;
            if (d + g < 0)
                h = -g;
        }
        h -= s10->curVal;
    }
    else
    {
        h = 0;
    }

    s10->speedHi = h / e;
    if (h < 0)
    {
        h              = -h;
        s10->direction = -1;
    }
    else
    {
        s10->direction = 1;
    }

    s10->speedLo        = h % e;
    s10->speedLoCounter = 0;
}

void MidiDriver_ADLIB::adlibPlayNote(int channel, int note)
{
    uint8_t old, oct, notex;
    int     note2;
    int     i;

    note2 = (note >> 7) - 4;
    note2 = (note2 < 128) ? note2 : 0;

    oct = (note2 / 12);
    if (oct > 7)
        oct = 7 << 2;
    else
        oct <<= 2;
    notex = note2 % 12 + 3;

    old = adlibGetRegValue(channel + 0xB0);
    if (old & 0x20)
    {
        old &= ~0x20;
        if (oct > old)
        {
            if (notex < 6)
            {
                notex += 12;
                oct   -= 4;
            }
        }
        else if (oct < old)
        {
            if (notex > 11)
            {
                notex -= 12;
                oct   += 4;
            }
        }
    }

    i = (notex << 3) + ((note >> 4) & 0x7);
    adlibWrite(channel + 0xA0, g_noteFrequencies[i]);
    adlibWrite(channel + 0xB0, oct | 0x20);
}

int MidiDriver_ADLIB::randomNr(int a)
{
    static uint8_t _randSeed = 1;
    if (_randSeed & 1)
    {
        _randSeed >>= 1;
        _randSeed  ^= 0xB8;
    }
    else
    {
        _randSeed >>= 1;
    }
    return _randSeed * a >> 8;
}

void MidiDriver_ADLIB::partKeyOff(AdLibChannel* part, uint8_t note)
{
    for (AdLibVoice* voice = part->voice; voice; voice = voice->next)
    {
        if (voice->getNote() == note)
        {
            if (part->sustain)
                voice->setWaitForPedal(true);
            else
                mcOff(voice);
        }
    }
}

AdLibChannel* MidiDriver_ADLIB::getChannel(const uint8_t channel) const noexcept
{
    return toAdlibPart(m_channels[channel]);
}

void MidiDriver_ADLIB::partKeyOn(AdLibChannel* part, const AdLibInstrument* instr, uint8_t note, uint8_t velocity, const AdLibInstrument* second, uint8_t pan)
{
    AdLibVoice* voice;

    voice = allocateVoice(part->priority);
    if (!voice)
        return;

    linkMc(part, voice);
    mcKeyOn(voice, instr, note, velocity, second, pan);
}

AdLibVoice* MidiDriver_ADLIB::allocateVoice(uint8_t pri)
{
    AdLibVoice* ac   = nullptr;
    AdLibVoice* best = nullptr;

    for (int i = 0; i < 9; i++)
    {
        if (++_voiceIndex >= 9)
            _voiceIndex = 0;

        ac = &m_voices[_voiceIndex];
        if (ac->getChannel() == nullptr)
            return ac;

        if (ac->next == nullptr)
        {
            const auto priEff_ = toAdlibPart(ac->getChannel())->priority;
            if (priEff_ <= pri)
            {
                pri  = priEff_;
                best = ac;
            }
        }
    }

    /* SCUMM V3 games don't have note priorities, first comes wins. */
    if (_scummSmallHeader)
        return nullptr;

    if (best)
        mcOff(best);
    return best;
}

void MidiDriver_ADLIB::linkMc(AdLibChannel* part, AdLibVoice* voice)
{
    voice->setFree(false);
    voice->setChannel(part);
    voice->next = part->voice;
    part->voice = voice;
    voice->prev = nullptr;

    if (voice->next != nullptr)
        voice->next->prev = voice;
}

void MidiDriver_ADLIB::mcKeyOn(AdLibVoice* voice, const AdLibInstrument* instr, uint8_t note, uint8_t velocity, const AdLibInstrument* second, uint8_t pan)
{
    const AdLibChannel* part = toAdlibPart(voice->getChannel());
    uint8_t             vol1, vol2;
    uint8_t             secVol1 = 0, secVol2 = 0;

    voice->twoChan = instr->feedback & 1;
    voice->setNote(note);
    voice->setWaitForPedal(false);
    voice->duration = instr->duration;
    if (voice->duration != 0)
        voice->duration *= 63;

    if (!_scummSmallHeader)
    {
        if (m_opl3Mode)
            vol1 = (instr->modScalingOutputLevel & 0x3F) + (velocity * ((instr->modWaveformSelect >> 3) + 1)) / 64;
        else
            vol1 = (instr->modScalingOutputLevel & 0x3F) + g_volumeLookupTable[velocity >> 1][instr->modWaveformSelect >> 2];
    }
    else
    {
        vol1 = 0x3f - (instr->modScalingOutputLevel & 0x3F);
    }

    if (vol1 > 0x3F)
        vol1 = 0x3F;

    voice->vol1 = vol1;

    if (!_scummSmallHeader)
    {
        if (m_opl3Mode)
            vol2 = (instr->carScalingOutputLevel & 0x3F) + (velocity * ((instr->carWaveformSelect >> 3) + 1)) / 64;
        else
            vol2 = (instr->carScalingOutputLevel & 0x3F) + g_volumeLookupTable[velocity >> 1][instr->carWaveformSelect >> 2];
    }
    else
    {
        vol2 = 0x3f - (instr->carScalingOutputLevel & 0x3F);
    }

    if (vol2 > 0x3F)
        vol2 = 0x3F;
    voice->vol2 = vol2;

    if (m_opl3Mode)
    {
        voice->secTwoChan = second->feedback & 1;
        secVol1           = (second->modScalingOutputLevel & 0x3F) + (velocity * ((second->modWaveformSelect >> 3) + 1)) / 64;
        if (secVol1 > 0x3F)
        {
            secVol1 = 0x3F;
        }
        voice->secVol1 = secVol1;
        secVol2        = (second->carScalingOutputLevel & 0x3F) + (velocity * ((second->carWaveformSelect >> 3) + 1)) / 64;
        if (secVol2 > 0x3F)
        {
            secVol2 = 0x3F;
        }
        voice->secVol2 = secVol2;
    }

    if (!_scummSmallHeader)
    {
        if (!m_opl3Mode)
        {
            int c = part->volume >> 2;
            vol2  = g_volumeTable[g_volumeLookupTable[vol2][c]];
            if (voice->twoChan)
                vol1 = g_volumeTable[g_volumeLookupTable[vol1][c]];
        }
        else
        {
            vol2    = g_volumeTable[((vol2 + 1) * part->volume) >> 7];
            secVol2 = g_volumeTable[((secVol2 + 1) * part->volume) >> 7];
            if (voice->twoChan)
                vol1 = g_volumeTable[((vol1 + 1) * part->volume) >> 7];
            if (voice->secTwoChan)
                secVol1 = g_volumeTable[((secVol1 + 1) * part->volume) >> 7];
        }
    }

    adlibSetupChannel(voice->slot, instr, vol1, vol2);
    if (!m_opl3Mode)
    {
        adlibNoteOnEx(voice->slot, /*part->_transposeEff + */ note, part->detuneEff + (part->pitch * part->pitchBendFactor >> 6));

        // TODO/NOTE: it looks dead code to me, didn't find anything that is changing flagsA value in the code.
        if (instr->flagsA & 0x80)
        {
            mcInitStuff(voice, &voice->_s10a, &voice->_s11a, instr->flagsA, &instr->extraA);
        }
        else
        {
            voice->_s10a.active = 0;
        }

        if (instr->flagsB & 0x80)
        {
            mcInitStuff(voice, &voice->_s10b, &voice->_s11b, instr->flagsB, &instr->extraB);
        }
        else
        {
            voice->_s10b.active = 0;
        }
    }
    else
    {
        adlibSetupChannelSecondary(voice->slot, second, secVol1, secVol2, pan);
        adlibNoteOnEx(voice->slot, note, part->pitch >> 1);
    }
}

void MidiDriver_ADLIB::adlibSetupChannel(int chan, const AdLibInstrument* instr, uint8_t vol1, uint8_t vol2)
{
    assert(chan >= 0 && chan < 9);

    uint8_t channel = g_operator1Offsets[chan];
    adlibWrite(channel + 0x20, instr->modCharacteristic);
    adlibWrite(channel + 0x40, (instr->modScalingOutputLevel | 0x3F) - vol1);
    adlibWrite(channel + 0x60, 0xff & (~instr->modAttackDecay));
    adlibWrite(channel + 0x80, 0xff & (~instr->modSustainRelease));
    adlibWrite(channel + 0xE0, instr->modWaveformSelect);

    channel = g_operator2Offsets[chan];
    adlibWrite(channel + 0x20, instr->carCharacteristic);
    adlibWrite(channel + 0x40, (instr->carScalingOutputLevel | 0x3F) - vol2);
    adlibWrite(channel + 0x60, 0xff & (~instr->carAttackDecay));
    adlibWrite(channel + 0x80, 0xff & (~instr->carSustainRelease));
    adlibWrite(channel + 0xE0, instr->carWaveformSelect);

    adlibWrite((uint8_t) chan + 0xC0, instr->feedback | (m_opl3Mode ? 0x30 : 0));
}

void MidiDriver_ADLIB::adlibSetupChannelSecondary(int chan, const AdLibInstrument* instr, uint8_t vol1, uint8_t vol2, uint8_t pan)
{
    assert(chan >= 0 && chan < 9);
    assert(m_opl3Mode);

    uint8_t channel = g_operator1Offsets[chan];
    adlibWriteSecondary(channel + 0x20, instr->modCharacteristic);
    adlibWriteSecondary(channel + 0x40, (instr->modScalingOutputLevel | 0x3F) - vol1);
    adlibWriteSecondary(channel + 0x60, 0xff & (~instr->modAttackDecay));
    adlibWriteSecondary(channel + 0x80, 0xff & (~instr->modSustainRelease));
    adlibWriteSecondary(channel + 0xE0, instr->modWaveformSelect);

    channel = g_operator2Offsets[chan];
    adlibWriteSecondary(channel + 0x20, instr->carCharacteristic);
    adlibWriteSecondary(channel + 0x40, (instr->carScalingOutputLevel | 0x3F) - vol2);
    adlibWriteSecondary(channel + 0x60, 0xff & (~instr->carAttackDecay));
    adlibWriteSecondary(channel + 0x80, 0xff & (~instr->carSustainRelease));
    adlibWriteSecondary(channel + 0xE0, instr->carWaveformSelect);

    // The original uses the following (strange) behavior:
#if 0
        if (instr->feedback | (pan > 64)) {
            adlibWriteSecondary((uint8_t)chan + 0xC0, 0x20);
        }
        else {
            adlibWriteSecondary((uint8_t)chan + 0xC0, 0x10);
        }
#else
    adlibWriteSecondary((uint8_t) chan + 0xC0, instr->feedback | ((pan > 64) ? 0x20 : 0x10));
#endif
}

uint8_t MidiDriver_ADLIB::adlibGetRegValue(uint8_t reg) const noexcept
{
    return _regCache[reg];
}

uint8_t MidiDriver_ADLIB::adlibGetRegValueSecondary(uint8_t reg) const noexcept
{
    return _regCacheSecondary[reg];
}

void MidiDriver_ADLIB::mcInitStuff(AdLibVoice* voice, Struct10* s10, Struct11* s11, uint8_t flags, const InstrumentExtra* ie)
{
    const AdLibChannel* part = toAdlibPart(voice->getChannel());
    s11->modifyVal           = 0;
    s11->flag0x40            = flags & 0x40;
    s10->loop                = flags & 0x20;
    s11->flag0x10            = flags & 0x10;
    s11->param               = g_paramTable1[flags & 0xF];
    s10->maxValue            = g_maxValTable[flags & 0xF];
    s10->unk3                = 31;
    if (s11->flag0x40)
    {
        s10->modWheel = part->modulation >> 2;
    }
    else
    {
        s10->modWheel = 31;
    }

    switch (s11->param)
    {
    case 0:
        s10->startValue = voice->vol2;
        break;
    case 13:
        s10->startValue = voice->vol1;
        break;
    case 30:
        s10->startValue    = 31;
        s11->s10->modWheel = 0;
        break;
    case 31:
        s10->startValue = 0;
        s11->s10->unk3  = 0;
        break;
    default:
        s10->startValue = adlibGetRegValueParam(voice->slot, s11->param);
    }

    struct10Init(s10, ie);
}

void MidiDriver_ADLIB::struct10Init(Struct10* s10, const InstrumentExtra* ie)
{
    s10->active = 1;
    if (!_scummSmallHeader)
    {
        s10->curVal = 0;
    }
    else
    {
        s10->curVal     = s10->startValue;
        s10->startValue = 0;
    }
    s10->modWheelLast = 31;
    s10->count        = ie->a;
    if (s10->count)
        s10->count *= 63;
    s10->tableA[0] = ie->b;
    s10->tableA[1] = ie->d;
    s10->tableA[2] = ie->f;
    s10->tableA[3] = ie->g;

    s10->tableB[0] = ie->c;
    s10->tableB[1] = ie->e;
    s10->tableB[2] = 0;
    s10->tableB[3] = ie->h;

    struct10Setup(s10);
}

int MidiDriver_ADLIB::adlibGetRegValueParam(int chan, uint8_t param)
{
    const AdLibSetParams* as;
    uint8_t               val;
    uint8_t               channel;

    assert(chan >= 0 && chan < 9);

    if (param <= 12)
    {
        channel = g_operator2Offsets[chan];
    }
    else if (param <= 25)
    {
        param   -= 13;
        channel  = g_operator1Offsets[chan];
    }
    else if (param <= 27)
    {
        param   -= 13;
        channel  = chan;
    }
    else if (param == 28)
    {
        return 0xF;
    }
    else if (param == 29)
    {
        return 0x17F;
    }
    else
    {
        return 0;
    }

    as    = &g_setParamTable[param];
    val   = adlibGetRegValue(channel + as->registerBase);
    val  &= as->mask;
    val >>= as->shift;
    if (as->inversion)
        val = as->inversion - val;

    return val;
}

void MidiDriver_ADLIB::adlibNoteOn(int chan, uint8_t note, int mod)
{
    if (m_opl3Mode)
    {
        adlibNoteOnEx(chan, note, mod);
        return;
    }

    assert(chan >= 0 && chan < 9);
    int code           = (note << 7) + mod;
    _curNotTable[chan] = code;
    adlibPlayNote(chan, (int16_t) _channelTable2[chan] + code);
}

void MidiDriver_ADLIB::adlibNoteOnEx(int chan, uint8_t note, int mod)
{
    assert(chan >= 0 && chan < 9);

    if (m_opl3Mode)
    {
        const int noteAdjusted = note + (mod >> 8) - 7;
        const int pitchAdjust  = (mod >> 5) & 7;

        adlibWrite(0xA0 + chan, g_noteFrequencies[(noteAdjusted % 12) * 8 + pitchAdjust + 6 * 8]);
        adlibWriteSecondary(0xA0 + chan, g_noteFrequencies[(noteAdjusted % 12) * 8 + pitchAdjust + 6 * 8]);
        adlibWrite(0xB0 + chan, (std::clamp(noteAdjusted / 12, 0, 7) << 2) | 0x20);
        adlibWriteSecondary(0xB0 + chan, (std::clamp(noteAdjusted / 12, 0, 7) << 2) | 0x20);
    }
    else
    {
        int code             = (note << 7) + mod;
        _curNotTable[chan]   = code;
        _channelTable2[chan] = 0;
        adlibPlayNote(chan, code);
    }
}
}    // namespace HyperSonicDrivers::drivers::midi::scummvm
