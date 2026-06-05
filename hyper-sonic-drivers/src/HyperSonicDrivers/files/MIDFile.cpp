#include <string>
#include <memory>
#include <cstring>
#include <algorithm>
#include <format>
#include <functional>
#include <cassert>
#include <HyperSonicDrivers/audio/midi/MIDIEvent.hpp>
#include <HyperSonicDrivers/audio/midi/MIDITrack.hpp>
#include <HyperSonicDrivers/files/MIDFile.hpp>
#include <HyperSonicDrivers/utils/algorithms.hpp>
#include <HyperSonicDrivers/utils/endianness.hpp>
#include <HyperSonicDrivers/utils/ILogger.hpp>

namespace HyperSonicDrivers::files
{
using audio::midi::MIDI_FORMAT;
using utils::logE;
using utils::logW;
using utils::throwLogC;

// TODO consider to "join" with IFFFile / or put common functions altogether
//      as it is similar to just the sub_header_chunk of IFF file.
constexpr const char* MIDI_HEADER = "MThd";
constexpr const char* MIDI_TRACK  = "MTrk";

MIDFile::MIDFile(const std::string& filename) : File(filename),
                                                _midi(nullptr)
{
    read_header();
    check_format();

    // Read Tracks
    for (int i = 0; i < _midi->numTracks; i++)
    {
        read_track();
    }
}

MIDFile::~MIDFile() = default;

std::shared_ptr<audio::MIDI> MIDFile::getOriginalMIDI() const noexcept
{
    return _midi;
}

std::shared_ptr<audio::MIDI> MIDFile::getMIDI() const
{
    using audio::midi::MIDI_EVENT_TYPES_HIGH;
    using audio::midi::MIDI_META_EVENT;
    using audio::midi::MIDI_META_EVENT_TYPES_LOW;
    using audio::midi::MIDI_META_EVENT_VAL;
    using audio::midi::MIDIEvent;

    if (_midi->format == audio::midi::MIDI_FORMAT::SINGLE_TRACK)
        return getOriginalMIDI();
    if (_midi->format == audio::midi::MIDI_FORMAT::MULTI_TRACK)
        throw std::runtime_error("MIDI MULTI_TRACK not supported yet");

    auto midi = std::make_shared<audio::MIDI>(audio::midi::MIDI_FORMAT::SINGLE_TRACK, 1, _midi->division);

    typedef struct midi_tuple_t
    {
        MIDIEvent e;
        uint32_t  abs_time = 0;
        uint16_t  track    = 0;
    } midi_tuple_t;

    using VecTuple = std::vector<midi_tuple_t>;
    VecTuple events_tuple;
    // count total events
    size_t totalEvents = 0;
    for (uint16_t n = 0; n < _midi->numTracks; n++)
    {
        totalEvents += _midi->getTrack(n).getEvents().size();
    }
    events_tuple.reserve(totalEvents);

    uint32_t          abs_time         = 0;
    constexpr uint8_t meta_event_val   = static_cast<uint8_t>(MIDI_META_EVENT_VAL::META);
    constexpr uint8_t end_of_track_val = static_cast<uint8_t>(MIDI_META_EVENT::END_OF_TRACK);
    midi_tuple_t      last_end_of_track;
    last_end_of_track.abs_time = 0;
    // 1. with absolute time just copy all the events as they are into 1 single track
    //    order of tracks matter
    //    only 1 delta time different from zero on same abs_time
    //    keep the delta_time different from 0 from track zero.
    for (uint16_t n = 0; n < _midi->numTracks; n++)
    {
        abs_time = 0;
        for (const auto& te : _midi->getTrack(n).getEvents())
        {
            // if it is a end_of_track skip it, it will be added later
            if (te.type.val == meta_event_val && te.data[0] == end_of_track_val)
            {
                if (abs_time > last_end_of_track.abs_time)
                {
                    last_end_of_track.e        = te;
                    last_end_of_track.abs_time = abs_time + te.delta_time;
                    last_end_of_track.track    = n;
                }
                continue;
            }

            abs_time += te.delta_time;
            midi_tuple_t mt;
            mt.abs_time = abs_time;
            mt.e        = te;
            mt.track    = n;
            events_tuple.emplace_back(mt);
        }
    }

    // add the end of track
    // this should be equivalent to the last event of the longest track, last end of track
    // last_end_of_track.abs_time = abs_time
    events_tuple.emplace_back(last_end_of_track);

    // 2. then sort them by absolute time
    //    tie break level 1 on delta_time,
    //    tie break level 2 on track number
    //    tie break level 3 on event type
    std::sort(
        events_tuple.begin(),
        events_tuple.end(),
        [](const midi_tuple_t& e1, const midi_tuple_t& e2) {
            if (e1.abs_time == e2.abs_time)
            {
                if (e1.e.delta_time == e2.e.delta_time)
                {
                    if (e1.track == e2.track)
                        return e1.e.type.val < e2.e.type.val;
                    else
                        return e1.track < e2.track;
                }
                else
                    return e1.e.delta_time > e2.e.delta_time;
            }
            else
                return e1.abs_time < e2.abs_time;
        });

    // 3. recompute delta time from absolute time
    abs_time = 0;
    for (auto& e : events_tuple)
    {
        e.e.delta_time = e.abs_time - abs_time;
        if (e.abs_time > abs_time)
            abs_time = e.abs_time;
    }

    // 4. extract MIDITrack from events without abs_time
    std::vector<MIDIEvent> events;
    events.reserve(events_tuple.size());

    std::transform(events_tuple.begin(),
                   events_tuple.end(),
                   std::back_inserter(events),
                   std::bind(&VecTuple::value_type::e, std::placeholders::_1));

    audio::midi::MIDITrack single_track(events);
    midi->addTrack(single_track);
    assert(midi->getTrack().getEvents().size() == single_track.getEvents().size());

    return midi;
}

int MIDFile::decode_VLQ(uint32_t& out_value) const
{
    uint8_t buf[4] = {0, 0, 0, 0};
    uint8_t i      = 0;
    uint8_t v      = 0;

    do
    {
        buf[i++] = v = readU8();
        assertValid_(i <= 4);
    }
    while (v & 0x80);

    return utils::decode_VLQ(buf, out_value);
}

MIDFile::midi_chunk_t MIDFile::read_chunk() const noexcept
{
    midi_chunk_t chunk;

    read(&chunk, sizeof(midi_chunk_t));
    chunk.length = utils::swapBE32(chunk.length);

    return chunk;
}

void MIDFile::read_header() noexcept
{
    midi_chunk_t header = read_chunk();
    assertValid_(strncmp(header.id, MIDI_HEADER, sizeof(header.id)) == 0);
    assertValid_(header.length == 6);

    uint16_t format   = readBE16();
    uint16_t nTracks  = readBE16();
    uint16_t division = readBE16();

    _midi = std::make_shared<audio::MIDI>(static_cast<MIDI_FORMAT>(format), nTracks, division);
}

void MIDFile::check_format() const
{
    switch (_midi->format)
    {
    case MIDI_FORMAT::SINGLE_TRACK:
        assertValid_(_midi->numTracks == 1);
        break;
    case MIDI_FORMAT::SIMULTANEOUS_TRACK:
    case MIDI_FORMAT::MULTI_TRACK:
        assertValid_(_midi->numTracks >= 1);
        break;
    default:
        throw std::runtime_error("MIDFile: _format invalid");
    }
}

void MIDFile::read_track() const
{
    using audio::midi::MIDI_EVENT_type_u;
    using audio::midi::MIDI_EVENT_TYPES_HIGH;
    using audio::midi::MIDI_META_EVENT;
    using audio::midi::MIDI_META_EVENT_TYPES_LOW;
    using audio::midi::MIDIEvent;
    using audio::midi::MIDITrack;
    using audio::midi::TO_META;

    MIDITrack         track;
    bool              endTrack   = false;
    MIDI_EVENT_type_u lastStatus = {0};
    // Read Track
    midi_chunk_t chunk = read_chunk();
    assertValid_(strncmp(chunk.id, MIDI_TRACK, sizeof(chunk.id)) == 0);

    // events
    uint32_t offs          = 0;
    uint32_t abs_time      = 0;
    uint32_t prev_abs_time = 0;
    while (!endTrack)
    {
        // MTrck Event:
        MIDIEvent e;
        // delta time encoded in VRQ
        offs     += decode_VLQ(e.delta_time);
        abs_time += e.delta_time;
        if (prev_abs_time > abs_time)
        {
            // uint32_t abs_time overflow
            // it shouldn't happen in "small midi" files
            // but just a sanity check just in case...
            // to figure it out when it might happen.
            // in theory never for small midi files.
            throw std::runtime_error("MIDI file too long, absolute time overflowed");
        }

        e.type.val = readU8();

        if (e.type._.high < 0x8)
        {
            e.type = lastStatus;
            seek(-1, std::fstream::cur);
        }
        else
        {
            offs++;
        }

        switch (static_cast<MIDI_EVENT_TYPES_HIGH>(e.type._.high))
        {
        case MIDI_EVENT_TYPES_HIGH::META_SYSEX:
            // special event
            switch (static_cast<MIDI_META_EVENT_TYPES_LOW>(e.type._.low))
            {
            case MIDI_META_EVENT_TYPES_LOW::SYS_EX0:
                // sysEx-event
                logE("sysEx0 event not implemented yet");
                break;
            case MIDI_META_EVENT_TYPES_LOW::SYS_EX7:
                // sysEx-event
                logE("sysEx7 event not implemented yet");
                break;
            case MIDI_META_EVENT_TYPES_LOW::META:
            {
                // meta-event
                uint8_t type = readU8();
                offs++;
                assertValid_(type < 128);
                uint32_t length  = 0;
                offs            += decode_VLQ(length);
                e.data.reserve(length + 1);
                e.data.push_back(type);
                for (uint32_t j = 0; j < length; j++)
                {
                    e.data.push_back(readU8());
                    offs++;
                }

                if (MIDI_META_EVENT::END_OF_TRACK == TO_META(type))
                {
                    endTrack = true;
                }
                break;
            }
            default:
                throwLogC<std::runtime_error>(std::format("MIDFile sub-event {:#04x} not recognized", e.type.val));
                break;
            }
            break;
        case MIDI_EVENT_TYPES_HIGH::PROGRAM_CHANGE:
        case MIDI_EVENT_TYPES_HIGH::CHANNEL_AFTERTOUCH:
            e.data.reserve(1);
            e.data.push_back(readU8());
            offs++;
            break;
        case MIDI_EVENT_TYPES_HIGH::NOTE_OFF:
        case MIDI_EVENT_TYPES_HIGH::NOTE_ON:
        case MIDI_EVENT_TYPES_HIGH::AFTERTOUCH:
        case MIDI_EVENT_TYPES_HIGH::CONTROLLER:
        case MIDI_EVENT_TYPES_HIGH::PITCH_BEND:
            e.data.reserve(2);
            e.data.push_back(readU8());
            e.data.push_back(readU8());
            offs += 2;
            break;
        default:
            // using previous status
            if (lastStatus.val == 0)
            {
                throwLogC<std::runtime_error>(std::format("MIDFile: midi event {:#02x} not recognized {:#02x} - last status = {} (pos={}).",
                                                          e.type.val,
                                                          static_cast<unsigned int>(e.type._.high),
                                                          lastStatus.val,
                                                          static_cast<unsigned long>(tell()))
                                                  .c_str());
            }
            break;
        }

        e.data.shrink_to_fit();
        track.addEvent(e);
        lastStatus = e.type;
    }

    // sanity check
    if (offs != chunk.length)
    {
        logW(std::format("Filename '{}' track {} length mismatch real length {}", m_filename, chunk.length, offs));
    }

    track.lock();
    _midi->addTrack(track);
}
}    // namespace HyperSonicDrivers::files
